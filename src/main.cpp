#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "BuzzerManager.h"
#include "DisplayManager.h"
#include "DashboardPublisher.h"
#include "DriveCommandHandler.h"
#include "ImuManager.h"
#include "MotorController.h"
#include "NavigationManager.h"
#include "PinConfig.h"
#include "RobotConfig.h"
#include "RadioManager.h"
#include "SafetyManager.h"
#include "SensorManager.h"
#include "ServoScanner.h"
#include "Secrets.h"
#include "WebDashboard.h"
#include "WiFiConfig.h"

bool serviceLoop();
void publishCurrentTelemetry(bool force = false);
void triggerObstacleWarning();

namespace {
  static constexpr unsigned long MANUAL_SENSOR_HEAD_SETTLE_MS = 700;

  enum class ManualSensorHeadState {
    Idle,
    Settling,
    Sampling
  };

  struct ManualSensorHeadOperation {
    ManualSensorHeadState state = ManualSensorHeadState::Idle;
    ManualSensorHeadCommand command = ManualSensorHeadCommand::None;
    unsigned long stateStartedMs = 0;
    unsigned long targetSampleSequence = 0;
  };

  struct ManualInspectionState {
    bool ready = false;
    bool obstacle = false;
    ManualSensorHeadCommand orientation = ManualSensorHeadCommand::None;
    unsigned long lastEvaluatedSequence = 0;
  };

  ManualSensorHeadOperation manualSensorHeadOperation;
  ManualInspectionState manualInspectionState;
  int manualLogicalInspectionAngle = RobotConfig::CENTER_SCAN;
}

MotorController motors(PinConfig::RIGHT_IN1,
                       PinConfig::RIGHT_IN2,
                       PinConfig::LEFT_IN1,
                       PinConfig::LEFT_IN2,
                       PinConfig::MOTOR_ENABLE_PWM);

DisplayManager display;
DriveCommandHandler driveCommands(motors);
SensorManager sensors;
ImuManager imu;
RadioManager radio;
ServoScanner servoScanner(PinConfig::SERVO_PIN);
BuzzerManager buzzer(PinConfig::BUZZER_PIN);
SafetyManager safety;
WebDashboard dashboard;
DashboardData dashboardData;
DashboardPublisher dashboardPublisher(dashboard, display, dashboardData);
String lastControlMode = "MANUAL";
String manualMotorDisplayState = "";
String currentServoPosition = "FRONT";
NavigationManager navigation(motors,
                             sensors,
                             servoScanner,
                             safety,
                             dashboard,
                             dashboardData,
                             currentServoPosition,
                             serviceLoop,
                             publishCurrentTelemetry,
                             triggerObstacleWarning);

unsigned long lastDashboardPublishMs = 0;
unsigned long obstacleWarningUntilMs = 0;
unsigned long criticalStopModeCommandCounter = 0;
unsigned long lastManualGuardRequestMs = 0;
unsigned long lastManualGuardSequence = 0;
float lastManualFrontDistance = -1;
bool criticalStopLatched = false;
bool manualForwardBlockedByUltrasonic = false;
bool manualMovementActive = false;

void stopMotors() {
  motors.stop();
  manualMovementActive = false;
}

void forward() {
  motors.forward();
}

void backward() {
  motors.backward();
}

void leftTurn() {
  motors.leftTurn();
}

void rightTurn() {
  motors.rightTurn();
}

void spinLeft() {
  motors.spinLeft();
}

void spinRight() {
  motors.spinRight();
}

void responsiveDelay(unsigned long durationMs) {
  unsigned long startMs = millis();
  while (millis() - startMs < durationMs) {
    sensors.setDistanceFacingFront(currentServoPosition == "FRONT");
    serviceLoop();
    delay(1);
  }
}

String getGasStatus(const SensorSnapshot& snapshot) {
  if (!RobotConfig::HAS_MQ2) {
    return "DISABLED";
  }
  if (snapshot.mq2Warmup) {
    return "WARMING";
  }
  if (snapshot.gasFiltered >= RobotConfig::GAS_HIGH_RELATIVE_THRESHOLD) {
    return "HIGH_RELATIVE";
  }
  if (snapshot.gasFiltered >= RobotConfig::GAS_ELEVATED_RELATIVE_THRESHOLD) {
    return "ELEVATED_RELATIVE";
  }
  return "LOW_RELATIVE";
}

uint8_t currentEnvironmentalAlarmMask() {
  const SensorSnapshot& snapshot = sensors.snapshot();
  uint8_t mask = 0;
  if (snapshot.flameClassificationValid &&
      snapshot.flameState == FlameState::FlameDetected) mask |= 0x01;
  if (!snapshot.mq2Warmup && snapshot.gasFiltered >= RobotConfig::GAS_HIGH_RELATIVE_THRESHOLD) mask |= 0x02;
  if (snapshot.temperatureValid && snapshot.temperature >= RobotConfig::TEMP_CRITICAL_C) mask |= 0x04;
  return mask;
}

unsigned long sampleAge(unsigned long sampleMs, unsigned long now) {
  return sampleMs == 0 ? 0 : now - sampleMs;
}

int logicalDashboardScanAngle(const String& orientation) {
  // The installed servo is mechanically mirrored. Dashboard geometry uses
  // 0 degrees on the left, 90 front, and 180 on the right.
  if (orientation == "LEFT") return RobotConfig::RIGHT_SCAN;
  if (orientation == "RIGHT") return RobotConfig::LEFT_SCAN;
  return RobotConfig::CENTER_SCAN;
}

void holdManualServoForward() {
  servoScanner.center();
}

void releaseManualServo() {
  servoScanner.detach();
}

void syncDashboardState(const String& controlMode) {
  dashboardPublisher.syncState(controlMode, dashboard.getTurnMode(), motors.getStateName(), manualMotorDisplayState);
}

void publishCurrentTelemetry(bool force) {
  unsigned long now = millis();
  if (!force && lastDashboardPublishMs != 0 &&
      now - lastDashboardPublishMs < RobotConfig::DASHBOARD_PUBLISH_INTERVAL_MS) {
    return;
  }

  const SensorSnapshot& snapshot = sensors.snapshot();
  const ImuSnapshot& imuState = imu.snapshot();
  const RadioSnapshot& radioState = radio.snapshot();
  String controlMode = dashboard.getControlMode();
  String navigationStatus = dashboardData.navigationDecision.length() > 0 ? dashboardData.navigationDecision : motors.getStateName();
  int publishedServoAngle = controlMode == "MANUAL"
    ? manualLogicalInspectionAngle
    : logicalDashboardScanAngle(currentServoPosition);
  dashboardData.temperatureValid = snapshot.temperatureValid;
  dashboardData.humidityValid = snapshot.humidityValid;
  dashboardData.environmentSampleAgeMs = sampleAge(snapshot.environmentLastSuccessMs, now);
  dashboardData.distanceSampleAgeMs = sampleAge(snapshot.distanceSampleMs, now);
  dashboardData.mq2SampleAgeMs = sampleAge(snapshot.mq2SampleMs, now);
  dashboardData.flameSampleAgeMs = sampleAge(snapshot.flameSampleMs, now);
  dashboardData.imuInitialized = imuState.initialized;
  dashboardData.imuHealthy = imuState.healthy;
  dashboardData.imuHealthState = imuState.healthState;
  dashboardData.imuErrorCount = imuState.errorCount;
  dashboardData.imuConsecutiveFailures = imuState.consecutiveFailures;
  dashboardData.accelX = imuState.accelX;
  dashboardData.accelY = imuState.accelY;
  dashboardData.accelZ = imuState.accelZ;
  dashboardData.gyroX = imuState.gyroX;
  dashboardData.gyroY = imuState.gyroY;
  dashboardData.gyroZ = imuState.gyroZ;
  dashboardData.roll = imuState.roll;
  dashboardData.pitch = imuState.pitch;
  dashboardData.imuTemperature = imuState.temperature;
  dashboardData.motionState = imuState.motionState;
  dashboardData.imuSampleAgeMs = sampleAge(imuState.lastSuccessMs, now);
  dashboardData.radioInitialized = radioState.initialized;
  dashboardData.radioChipDetected = radioState.chipDetected;
  dashboardData.radioListening = radioState.listening;
  dashboardData.radioChannel = radioState.channel;
  dashboardData.radioDataRate = radio.dataRateName();
  dashboardData.radioPaLevel = radio.paLevelName();
  dashboardData.radioTxAttempts = radioState.txAttempts;
  dashboardData.radioTxSuccess = radioState.txSuccess;
  dashboardData.radioTxFailure = radioState.txFailure;
  dashboardData.radioRxPackets = radioState.rxPackets;
  dashboardData.radioHardware = radio.hardwareStateName();
  dashboardData.radioPeer = radio.peerStateName();
  dashboardData.radioHasTx = radioState.lastTxMs != 0;
  dashboardData.radioHasRx = radioState.lastRxMs != 0;
  dashboardData.radioLastTxAgeMs = sampleAge(radioState.lastTxMs, now);
  dashboardData.radioLastRxAgeMs = sampleAge(radioState.lastRxMs, now);
  dashboardData.displayInitialized = display.initialized();
  dashboardPublisher.publishSensorReadings(currentServoPosition,
                                           publishedServoAngle,
                                           snapshot,
                                           safety.isLeftIrObstacleActive(),
                                           safety.isRightIrObstacleActive(),
                                           motors.getSpeedPercent(),
                                           safety.obstacleDecision(snapshot),
                                           getGasStatus(snapshot),
                                           safety.stateName(),
                                           safety.alarmReason(),
                                           safety.isNearObstacleHazardActive(),
                                           safety.nearObstacleSource(),
                                           safety.isUltrasonicWarningActive(),
                                           buzzer.soundStateName(),
                                           buzzer.modeName(),
                                           controlMode,
                                           dashboard.getTurnMode(),
                                           motors.getStateName(),
                                           manualMotorDisplayState,
                                           navigationStatus);
  lastDashboardPublishMs = now;
}

const char* manualSensorHeadOrientation(ManualSensorHeadCommand command) {
  switch (command) {
    case ManualSensorHeadCommand::Left: return "LEFT";
    case ManualSensorHeadCommand::Front: return "FRONT";
    case ManualSensorHeadCommand::Right: return "RIGHT";
    case ManualSensorHeadCommand::None: return "NONE";
  }
  return "NONE";
}

int logicalManualInspectionAngle(ManualSensorHeadCommand command) {
  switch (command) {
    case ManualSensorHeadCommand::Left: return RobotConfig::RIGHT_SCAN;
    case ManualSensorHeadCommand::Front: return RobotConfig::CENTER_SCAN;
    case ManualSensorHeadCommand::Right: return RobotConfig::LEFT_SCAN;
    case ManualSensorHeadCommand::None: return RobotConfig::CENTER_SCAN;
  }
  return RobotConfig::CENTER_SCAN;
}

int physicalManualServoTargetAngle(ManualSensorHeadCommand command) {
  switch (command) {
    case ManualSensorHeadCommand::Left: return RobotConfig::LEFT_SCAN;
    case ManualSensorHeadCommand::Front: return RobotConfig::CENTER_SCAN;
    case ManualSensorHeadCommand::Right: return RobotConfig::RIGHT_SCAN;
    case ManualSensorHeadCommand::None: return RobotConfig::CENTER_SCAN;
  }
  return RobotConfig::CENTER_SCAN;
}

bool clearManualInspectionState() {
  bool changed = manualInspectionState.ready ||
                 manualInspectionState.obstacle ||
                 manualInspectionState.orientation != ManualSensorHeadCommand::None ||
                 dashboardData.manualInspectionObstacle ||
                 dashboardData.manualInspectionOrientation != "NONE";
  manualInspectionState = ManualInspectionState();
  manualInspectionState.lastEvaluatedSequence = sensors.distanceSampleSequence();
  dashboardData.manualInspectionObstacle = false;
  dashboardData.manualInspectionOrientation = "NONE";
  return changed;
}

bool updateManualInspectionWarning() {
  const char* orientation = manualSensorHeadOrientation(manualInspectionState.orientation);
  if (dashboard.getControlMode() != "MANUAL" ||
      !manualInspectionState.ready ||
      manualSensorHeadOperation.state != ManualSensorHeadState::Idle ||
      manualInspectionState.orientation == ManualSensorHeadCommand::None ||
      currentServoPosition != orientation) {
    return clearManualInspectionState();
  }

  const SensorSnapshot& snapshot = sensors.snapshot();
  if (snapshot.distanceSampleSequence == manualInspectionState.lastEvaluatedSequence) {
    return false;
  }
  manualInspectionState.lastEvaluatedSequence = snapshot.distanceSampleSequence;

  bool orientationMatches =
    manualInspectionState.orientation == ManualSensorHeadCommand::Front
      ? snapshot.distanceFacingFront
      : !snapshot.distanceFacingFront;
  bool obstacle = orientationMatches &&
                  snapshot.distanceValid &&
                  snapshot.ultrasonicState == UltrasonicResultState::DistanceValid &&
                  snapshot.distanceCm <= RobotConfig::NEAR_OBSTACLE_CM;
  bool changed = obstacle != manualInspectionState.obstacle;
  manualInspectionState.obstacle = obstacle;
  dashboardData.manualInspectionObstacle = obstacle;
  dashboardData.manualInspectionOrientation = orientation;
  return changed;
}

void confirmManualInspectionReady(ManualSensorHeadCommand command, unsigned long completedSequence) {
  manualInspectionState.ready = true;
  manualInspectionState.obstacle = false;
  manualInspectionState.orientation = command;
  manualInspectionState.lastEvaluatedSequence = completedSequence > 0 ? completedSequence - 1 : 0;
  dashboardData.manualInspectionObstacle = false;
  dashboardData.manualInspectionOrientation = manualSensorHeadOrientation(command);
  updateManualInspectionWarning();
}

void beginManualSensorHeadOperation(ManualSensorHeadCommand command, bool clearPendingDriveCommand) {
  stopMotors();
  manualMotorDisplayState = "STOP";
  // A moving head has no confirmed inspection direction; discard any prior side warning.
  clearManualInspectionState();
  if (clearPendingDriveCommand) {
    dashboard.setPendingCommand("");
  }

  const char* orientation = manualSensorHeadOrientation(command);
  currentServoPosition = orientation;
  manualLogicalInspectionAngle = logicalManualInspectionAngle(command);
  sensors.setDistanceFacingFront(command == ManualSensorHeadCommand::Front);

  switch (command) {
    case ManualSensorHeadCommand::Left:
      servoScanner.writeAngle(physicalManualServoTargetAngle(command));
      dashboardData.navigationDecision = "MANUAL_LOOK_LEFT";
      break;
    case ManualSensorHeadCommand::Front:
      servoScanner.center();
      dashboardData.navigationDecision = clearPendingDriveCommand ? "MANUAL_LOOK_FRONT" : "MANUAL_DRIVE_RECENTER";
      break;
    case ManualSensorHeadCommand::Right:
      servoScanner.writeAngle(physicalManualServoTargetAngle(command));
      dashboardData.navigationDecision = "MANUAL_LOOK_RIGHT";
      break;
    case ManualSensorHeadCommand::None:
      return;
  }

  manualSensorHeadOperation.command = command;
  manualSensorHeadOperation.state = ManualSensorHeadState::Settling;
  manualSensorHeadOperation.stateStartedMs = millis();
  manualSensorHeadOperation.targetSampleSequence = 0;
  syncDashboardState("MANUAL");
  publishCurrentTelemetry(true);
}

bool updateManualSensorHeadOperation() {
  if (manualSensorHeadOperation.state == ManualSensorHeadState::Idle) {
    ManualSensorHeadCommand command = dashboard.consumeManualSensorHeadCommand();
    if (command == ManualSensorHeadCommand::None) {
      return false;
    }
    beginManualSensorHeadOperation(command, true);
    return true;
  }

  if (manualSensorHeadOperation.state == ManualSensorHeadState::Settling) {
    if (millis() - manualSensorHeadOperation.stateStartedMs < MANUAL_SENSOR_HEAD_SETTLE_MS ||
        sensors.isDistanceSampleInProgress()) {
      return true;
    }

    bool facingFront = manualSensorHeadOperation.command == ManualSensorHeadCommand::Front;
    manualSensorHeadOperation.targetSampleSequence = sensors.requestDistanceSample(facingFront);
    manualSensorHeadOperation.state = ManualSensorHeadState::Sampling;
    dashboardData.navigationDecision = "MANUAL_SENSOR_SAMPLE";
    syncDashboardState("MANUAL");
    return true;
  }

  if (sensors.distanceSampleSequence() < manualSensorHeadOperation.targetSampleSequence) {
    return true;
  }

  ManualSensorHeadCommand completedCommand = manualSensorHeadOperation.command;
  unsigned long completedSequence = manualSensorHeadOperation.targetSampleSequence;
  servoScanner.detach();
  manualSensorHeadOperation = ManualSensorHeadOperation();

  ManualSensorHeadCommand nextCommand = dashboard.consumeManualSensorHeadCommand();
  if (nextCommand != ManualSensorHeadCommand::None) {
    beginManualSensorHeadOperation(nextCommand, true);
    return true;
  }

  confirmManualInspectionReady(completedCommand, completedSequence);
  dashboardData.navigationDecision = "MANUAL_SENSOR_READY";
  syncDashboardState("MANUAL");
  publishCurrentTelemetry(true);
  return false;
}

bool serviceLoop() {
  dashboard.loop();
  // Apply the operator's fail-safe STOP before sensor, TFT, or telemetry work.
  // Leave the command pending so the existing Manual/AUTO handlers retain
  // their state updates and AUTO stop-latch behavior.
  if (dashboard.getPendingCommand() == "STOP" &&
      motors.getState() != MotorState::Stopped) {
    stopMotors();
    manualMotorDisplayState = "STOP";
  }
  int requestedSpeedPercent = 0;
  const bool speedChanged = dashboard.consumeSpeedPercent(requestedSpeedPercent);
  if (speedChanged) {
    motors.setSpeedPercent(requestedSpeedPercent);
  }
  sensors.update();
  safety.update(sensors.snapshot());
  if (safety.isCritical()) {
    if (!criticalStopLatched) {
      criticalStopLatched = true;
      criticalStopModeCommandCounter = dashboard.modeCommandCounter();
    }
    if (motors.getState() != MotorState::Stopped) stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "SAFETY_STOP";
  }
  imu.update();
  radio.update(currentEnvironmentalAlarmMask());
  bool manualDeadmanTriggered = false;
  if (dashboard.getControlMode() == "MANUAL" &&
      manualMovementActive &&
      motors.getState() != MotorState::Stopped &&
      millis() - dashboard.lastCommandReceivedMs() >= RobotConfig::MANUAL_COMMAND_TIMEOUT_MS) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "MANUAL_DEADMAN_STOP";
    syncDashboardState("MANUAL");
    manualDeadmanTriggered = true;
    Serial.println("Manual command timeout. Motors stopped.");
  }
  bool manualInspectionChanged = updateManualInspectionWarning();
  bool safetyChanged = safety.stateChanged() || manualInspectionChanged || manualDeadmanTriggered || speedChanged;

  buzzer.setEnabled(dashboard.isAlarmEnabled());
  buzzer.setMuted(dashboard.isAlarmMuted());
  if (dashboard.consumeAlarmTestRequest()) {
    buzzer.requestTest();
  }

  bool sideInspectionObstacle =
    manualInspectionState.obstacle &&
    manualInspectionState.orientation != ManualSensorHeadCommand::Front;
  if (sideInspectionObstacle) {
    if (motors.getState() != MotorState::Stopped) {
      stopMotors();
    }
    manualMotorDisplayState = "STOP";
    // Preserve a directional request so Manual mode can recenter and revalidate FRONT.
    dashboardData.navigationDecision = "MANUAL_INSPECTION_STOP";
  }

  if (safety.isCritical()) {
    buzzer.setMode(BuzzerMode::Critical);
  } else if (safety.isNearObstacleHazardActive() || millis() < obstacleWarningUntilMs) {
    buzzer.setMode(BuzzerMode::Hazard);
  } else if (manualInspectionState.obstacle) {
    // Manual inspection is audible but never enters SafetyManager navigation decisions.
    buzzer.setMode(BuzzerMode::Hazard);
  } else if (safety.isCaution()) {
    buzzer.setMode(BuzzerMode::Caution);
  } else {
    buzzer.setMode(BuzzerMode::Off);
  }
  buzzer.update();
  publishCurrentTelemetry(safetyChanged);
  return safetyChanged;
}
bool handleManualControl() {
  String command = dashboard.getPendingCommand();

  if (command.length() == 0) {
    return false;
  }

  if (command != "STOP" &&
      millis() - dashboard.lastCommandReceivedMs() >= RobotConfig::MANUAL_COMMAND_TIMEOUT_MS) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "MANUAL_STALE_COMMAND_STOP";
    syncDashboardState("MANUAL");
    return false;
  }

  if (safety.blocksMotion() && command != "STOP") {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "SAFETY_STOP";
    syncDashboardState("MANUAL");
    return false;
  }

  if (command == "FORWARD" && manualForwardBlockedByUltrasonic) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "MANUAL_FORWARD_HOLD";
    syncDashboardState("MANUAL");
    return false;
  }

  if (command == "FORWARD" && safety.blocksManualForwardMotion()) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = safety.obstacleDecision(sensors.snapshot()) == "ULTRASONIC_BLOCK"
      ? "FORWARD_BLOCKED_ULTRASONIC"
      : "FORWARD_BLOCKED_BOTH_IR";
    syncDashboardState("MANUAL");
    return false;
  }

  String currentManualState = manualMotorDisplayState.length() > 0 ? manualMotorDisplayState : motors.getStateName();
  if (command != "STOP") {
    manualMovementActive = true;
  }
  DriveCommandResult result = driveCommands.handleCommand(command, currentManualState, dashboard.getTurnMode());

  dashboard.setPendingCommand("");
  if (command == "STOP") {
    manualMovementActive = false;
  }
  if (!result.executed) {
    return false;
  }

  if (criticalStopLatched && !safety.isCritical()) {
    criticalStopLatched = false;
  }
  manualMotorDisplayState = result.motorDisplayState;
  dashboardData.navigationDecision = result.navigationDecision;
  syncDashboardState("MANUAL");
  return true;
}
void triggerObstacleWarning() {
  obstacleWarningUntilMs = millis() + RobotConfig::OBSTACLE_WARNING_MS;
}

void updateManualMode() {
  sensors.setDistanceFacingFront(currentServoPosition == "FRONT");
  bool sensorHeadBusy = updateManualSensorHeadOperation();
  if (!sensorHeadBusy) {
    String pendingDriveCommand = dashboard.getPendingCommand();
    if (pendingDriveCommand.length() > 0 &&
        pendingDriveCommand != "STOP" &&
        currentServoPosition != "FRONT") {
      // Preserve the existing front-facing drive guard before motion resumes.
      beginManualSensorHeadOperation(ManualSensorHeadCommand::Front, false);
      sensorHeadBusy = true;
    } else {
      handleManualControl();
    }
  }

  const SensorSnapshot& snapshot = sensors.snapshot();
  if (snapshot.distanceFacingFront) {
    lastManualFrontDistance = snapshot.distanceCm;
  }
  unsigned long now = millis();

  if (!sensorHeadBusy &&
      currentServoPosition == "FRONT" &&
      !sensors.isDistanceSampleInProgress() &&
      (lastManualGuardRequestMs == 0 ||
       now - lastManualGuardRequestMs >= RobotConfig::ULTRASONIC_GUARD_INTERVAL_MS)) {
    lastManualGuardRequestMs = now;
    sensors.requestDistanceSample(true);
  }

  if (snapshot.distanceSampleSequence != lastManualGuardSequence &&
      snapshot.distanceFacingFront) {
    lastManualGuardSequence = snapshot.distanceSampleSequence;
    if (snapshot.distanceValid) {
      lastManualFrontDistance = snapshot.distanceCm;
      if (snapshot.distanceCm >= RobotConfig::MANUAL_FORWARD_CLEAR_CM) {
        manualForwardBlockedByUltrasonic = false;
      }
      if (motors.getState() == MotorState::Forward &&
          snapshot.distanceCm <= RobotConfig::MANUAL_FORWARD_STOP_CM) {
        manualForwardBlockedByUltrasonic = true;
        stopMotors();
        manualMotorDisplayState = "STOP";
        dashboard.setPendingCommand("");
        dashboardData.navigationDecision = "MANUAL_OBSTACLE_STOP";
        syncDashboardState("MANUAL");
        Serial.println("Manual fresh HC-SR04 forward stop.");
      }
    } else if (snapshot.ultrasonicState == UltrasonicResultState::ClearNoEcho) {
      manualForwardBlockedByUltrasonic = false;
    } else if (snapshot.ultrasonicState == UltrasonicResultState::SensorFault ||
               snapshot.ultrasonicState == UltrasonicResultState::Stale) {
      manualForwardBlockedByUltrasonic = true;
      if (motors.getState() == MotorState::Forward) {
        stopMotors();
        manualMotorDisplayState = "STOP";
        dashboard.setPendingCommand("");
        dashboardData.navigationDecision = "MANUAL_ULTRASONIC_FAULT_STOP";
        syncDashboardState("MANUAL");
      }
    }
  }

  if (motors.getState() == MotorState::Forward &&
      safety.blocksManualForwardMotion()) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "FORWARD_BLOCKED_NEAR";
    syncDashboardState("MANUAL");
    Serial.println("Manual near obstacle stop.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.print("[Boot] ");
  Serial.print(RobotConfig::PRODUCT_NAME);
  Serial.print(" firmware ");
  Serial.println(RobotConfig::FIRMWARE_VERSION);

  // Mandatory first actuator action: direction LOW and shared enable duty zero.
  motors.begin();
  stopMotors();
  manualMotorDisplayState = "STOP";
  Serial.println("[Boot] Motors initialized STOPPED (L298N PWM, default 60%)");

  buzzer.begin();
  sensors.begin();
  safety.begin();
  servoScanner.center();
  responsiveDelay(700);
  servoScanner.detach();

  Wire.begin(PinConfig::I2C_SDA, PinConfig::I2C_SCL);
  Wire.setClock(400000);
  imu.begin();

  pinMode(PinConfig::OLED_CS, OUTPUT);
  pinMode(PinConfig::NRF24_CSN, OUTPUT);
  digitalWrite(PinConfig::OLED_CS, HIGH);
  digitalWrite(PinConfig::NRF24_CSN, HIGH);
  SPI.begin(PinConfig::SPI_SCK, PinConfig::SPI_MISO, PinConfig::SPI_MOSI);
  display.begin();
  digitalWrite(PinConfig::OLED_CS, HIGH);
  digitalWrite(PinConfig::NRF24_CSN, HIGH);
  radio.begin();
  Serial.print("[Boot] Flash bytes: ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("[Boot] PSRAM bytes: ");
  Serial.println(ESP.getPsramSize());
  dashboardData.controlMode = "MANUAL";
  dashboardData.turnMode = "PIVOT";
  dashboardData.motorState = "STOP";
  dashboardData.speedPercent = motors.getSpeedPercent();
  dashboardData.obstacleDecision = "CLEAR";
  dashboardData.navigationDecision = "MANUAL_CONTROL";
  dashboardData.safetyState = "SAFE";
  dashboardData.alarmReason = "NONE";
  dashboardData.alarmSoundState = "NOT_INSTALLED";
  dashboardData.buzzerState = "NOT_INSTALLED";
  dashboardData.servoAngle = servoScanner.currentAngle();
  dashboardData.scanOrientation = currentServoPosition;
  dashboardData.displayInitialized = display.initialized();

  dashboard.begin(PIKU_SOFTAP_SSID,
                  PIKU_SOFTAP_PASSWORD,
                  WiFiConfig::HOME_WIFI_SSID,
                  WiFiConfig::HOME_WIFI_PASSWORD);
  dashboard.setControlMode("MANUAL");
  dashboard.setTurnMode("PIVOT");
  syncDashboardState("MANUAL");
  responsiveDelay(1000);
}

void loop() {
  serviceLoop();

  String controlMode = dashboard.getControlMode();
  if (controlMode != lastControlMode) {
    stopMotors();
    lastControlMode = controlMode;
    if (controlMode == "MANUAL") {
      manualSensorHeadOperation = ManualSensorHeadOperation();
      clearManualInspectionState();
      manualMotorDisplayState = "STOP";
      beginManualSensorHeadOperation(ManualSensorHeadCommand::Front, true);
    } else {
      manualSensorHeadOperation = ManualSensorHeadOperation();
      clearManualInspectionState();
      manualMotorDisplayState = "";
      releaseManualServo();
      navigation.resetRecoveryLatchIfClear();
    }
    syncDashboardState(controlMode);
  }

  if (safety.isCritical()) {
    publishCurrentTelemetry(true);
    delay(1);
    return;
  }

  if (criticalStopLatched) {
    if (dashboard.modeCommandCounter() != criticalStopModeCommandCounter) {
      criticalStopLatched = false;
      stopMotors();
      dashboardData.navigationDecision = controlMode == "AUTO" ? "AUTO_REARMED" : "MANUAL_CONTROL";
      dashboard.setNavigationDecision(dashboardData.navigationDecision);
    } else if (controlMode == "AUTO") {
      stopMotors();
      dashboardData.navigationDecision = "SAFETY_CLEARED_STOP";
      dashboard.setNavigationDecision(dashboardData.navigationDecision);
      publishCurrentTelemetry(true);
      delay(1);
      return;
    }
  }

  if (controlMode == "MANUAL") {
    updateManualMode();
    serviceLoop();
    return;
  }

  navigation.updateAuto();
}
