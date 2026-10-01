#include <Arduino.h>
#include <esp_system.h>
#include <SPI.h>
#include <Wire.h>
#include "BuzzerManager.h"
#include "DisplayManager.h"
#include "DashboardPublisher.h"
#include "DriveCommandHandler.h"
#include "ImuManager.h"
#include "MotorController.h"
#include "ManualOverridePolicy.h"
#include "ManualMotionGate.h"
#include "NavigationManager.h"
#include "NavigationTrace.h"
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
bool handleManualControl();
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
NavigationTrace navTrace;
DashboardPublisher dashboardPublisher(dashboard, display, dashboardData);
String lastControlMode = "MANUAL";
String manualMotorDisplayState = "";
String currentServoPosition = "FRONT";
NavigationManager navigation(motors,
                             sensors,
                             imu,
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
bool criticalStopRequiresManualRearm = false;
bool manualForwardBlockedByUltrasonic = false;
bool manualMovementActive = false;
ManualOverridePolicy manualOverride;
bool allowSlowOutputsThisPass = true;
unsigned long lastServiceLoopStartedMs = 0;

const char* resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "POWER_ON";
    case ESP_RST_SW: return "SOFTWARE";
    case ESP_RST_PANIC: return "PANIC";
    case ESP_RST_INT_WDT: return "INTERRUPT_WATCHDOG";
    case ESP_RST_TASK_WDT: return "TASK_WATCHDOG";
    case ESP_RST_WDT: return "WATCHDOG";
    case ESP_RST_DEEPSLEEP: return "DEEP_SLEEP";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    default: return "OTHER";
  }
}

void stopMotors() {
  motors.stop();
  manualMovementActive = false;
}

void revokeManualOverride(const char* reason) {
  if (!manualOverride.enabled()) return;
  manualOverride.revoke();
  dashboardData.manualFlameGasOverride = false;
  dashboard.setManualOverrideState(false);
  dashboard.invalidateControlSession();
  dashboardData.manualMotionInhibitReason = reason;
  Serial.printf("[Safety] Manual flame/gas override revoked: %s\n", reason);
}

bool onManualOverrideAction(bool enable) {
  stopMotors();
  dashboard.setPendingCommand("");
  if (!enable) {
    revokeManualOverride("OVERRIDE_OFF");
    return true;
  }
  sensors.updateControlSensors();
  safety.update(sensors.snapshot());
  const SensorSnapshot& overrideSnapshot = sensors.snapshot();
  if (!overrideSnapshot.flameSampleValid) return false;
  const bool accepted = manualOverride.enable(
      dashboard.getControlMode() == "MANUAL",
      safety.isAutoRecoverableEnvironmentalHazard(),
      safety.hasNonRecoverableCriticalHazard(),
      dashboard.sessionAlive(), true);
  if (!accepted) return false;
  dashboardData.manualFlameGasOverride = true;
  dashboard.setManualOverrideState(true);
  dashboardData.manualMotionInhibitReason = "AWAIT_FRESH_DIRECTION";
  Serial.println("[Safety] Manual flame/gas override enabled; rear clearance unverified.");
  return true;
}

void onDashboardCommandAccepted(const String& command,
                                unsigned long sequence,
                                unsigned long acceptedMs) {
  (void)acceptedMs;
  if (command == "STOP" || command == "RELEASE") {
    // MotorController::stop() writes PWM duty zero before changing GPIO state.
    // This callback runs inside /cmd before logging or formatting a response.
    stopMotors();
    if (command == "STOP") revokeManualOverride("OPERATOR_STOP");
    else manualOverride.movementStopped();
    manualMotorDisplayState = "STOP";
    dashboardData.motorState = "STOP";
    dashboardData.navigationDecision = command == "STOP" ? "OPERATOR_STOP" : "MANUAL_RELEASE_STOP";
    dashboardData.manualMotionInhibitReason = command == "STOP" ? "OPERATOR_STOP" : "MANUAL_RELEASE";
    dashboard.markCommandApplied("STOP", sequence,
                                 command == "STOP" ? "OPERATOR_STOP" : "MANUAL_RELEASE");
    if (command == "STOP" && dashboard.getControlMode() == "AUTO") {
      navigation.notifyOperatorStop();
    }
    return;
  }

  if (dashboard.getControlMode() != "MANUAL" ||
      manualSensorHeadOperation.state != ManualSensorHeadState::Idle ||
      currentServoPosition != "FRONT") {
    return;
  }

  // Refresh only control-critical inputs before applying motion. DHT, IMU,
  // OLED, nRF and serial work remain outside the HTTP handler.
  sensors.updateControlSensors();
  safety.update(sensors.snapshot());
  handleManualControl();
}

void onDashboardModeAccepted(const String& mode) {
  stopMotors();
  revokeManualOverride("MODE_CHANGE");
  navigation.cancelForModeChange();
  manualMotorDisplayState = mode == "MANUAL" ? "STOP" : "";
  dashboardData.motorState = "STOP";
  dashboardData.navigationDecision = "MODE_CHANGE_STOP";
  dashboard.markStopApplied("MODE_CHANGE");
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
  if (snapshot.flameSampleValid && snapshot.flameDetected) mask |= 0x01;
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
  dashboardData.gasCritical = safety.isGasCritical();
  dashboardData.flameCritical = safety.isFlameCritical();
  dashboardData.temperatureCritical = safety.isTemperatureCritical();
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
  dashboardData.radioEnabled = RobotConfig::ENABLE_NRF24;
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
  dashboardData.freeHeap = ESP.getFreeHeap();
  dashboardData.minimumFreeHeap = ESP.getMinFreeHeap();
  dashboardData.psramSize = ESP.getPsramSize();
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
                                           navigationStatus,
                                           allowSlowOutputsThisPass);
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

bool manualFrontTurnVeto(const SensorSnapshot& snapshot, unsigned long now) {
  return snapshot.distanceFacingFront && snapshot.distanceValid &&
      snapshot.ultrasonicState == UltrasonicResultState::DistanceValid &&
      snapshot.distanceSampleMs != 0 &&
      now - snapshot.distanceSampleMs <= RobotConfig::ULTRASONIC_STALE_MS &&
      snapshot.distanceCm <= RobotConfig::NEAR_OBSTACLE_CM;
}

bool serviceLoop() {
  const unsigned long serviceStartedMs = millis();
  if (lastServiceLoopStartedMs != 0) {
    const unsigned long gap = serviceStartedMs - lastServiceLoopStartedMs;
    if (gap > dashboardData.maximumServiceLoopGapMs) {
      dashboardData.maximumServiceLoopGapMs = gap;
    }
  }
  lastServiceLoopStartedMs = serviceStartedMs;
  dashboard.loop();
  const bool controlActivity = dashboard.consumeControlActivity();
  const bool controlPending = dashboard.getPendingCommand().length() > 0;
  const bool allowBackgroundWork = !controlActivity && !controlPending;
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
  if (allowBackgroundWork) {
    sensors.update();
  } else {
    sensors.updateControlSensors();
  }
  safety.update(sensors.snapshot());
  if (dashboard.getControlMode() == "MANUAL") {
    const MotorState motion=motors.getState();
    const bool nearFront=manualFrontTurnVeto(sensors.snapshot(), millis());
    const bool leftTurning=motion == MotorState::LeftTurn || motion == MotorState::SpinLeft;
    const bool rightTurning=motion == MotorState::RightTurn || motion == MotorState::SpinRight;
    if ((leftTurning && (safety.isLeftIrObstacleActive() || nearFront)) ||
        (rightTurning && (safety.isRightIrObstacleActive() || nearFront))) {
      stopMotors();
      manualMotorDisplayState="STOP";
      dashboard.setPendingCommand("");
      dashboardData.navigationDecision="MANUAL_TURN_OBSTACLE_STOP";
      dashboardData.manualMotionInhibitReason="TURN_OBSTACLE";
      dashboard.markStopApplied("TURN_OBSTACLE");
    }
  }
  const bool hadOverride = manualOverride.enabled();
  const bool sessionLive = dashboard.sessionAlive();
  const SensorSnapshot& guardSnapshot = sensors.snapshot();
  const bool sensorFault = !guardSnapshot.flameSampleValid;
  manualOverride.tick(millis(), dashboard.getControlMode() == "MANUAL",
                      safety.isAutoRecoverableEnvironmentalHazard(),
                      safety.hasNonRecoverableCriticalHazard() || sensorFault,
                      sessionLive);
  if (hadOverride && !manualOverride.enabled()) {
    const char* reason = !sessionLive ? "CONTROL_SESSION_LOST" :
        safety.hasNonRecoverableCriticalHazard() ? "TEMPERATURE_CRITICAL" :
        sensorFault ? "SENSOR_FAULT" : "HAZARD_CLEARED";
    stopMotors();
    dashboard.setPendingCommand("");
    dashboardData.manualFlameGasOverride = false;
    dashboard.setManualOverrideState(false);
    dashboard.invalidateControlSession();
    dashboardData.manualMotionInhibitReason = reason;
    dashboard.markStopApplied(reason);
  }
  const bool hazardExempt = manualOverride.exemptsFlameGas(
      dashboard.getControlMode() == "MANUAL", sessionLive);
  if (safety.blocksMotion(hazardExempt)) {
    if (!criticalStopLatched) {
      criticalStopLatched = true;
      criticalStopModeCommandCounter = dashboard.modeCommandCounter();
    }
    if (safety.hasNonRecoverableCriticalHazard()) {
      criticalStopRequiresManualRearm = true;
    }
    if (motors.getState() != MotorState::Stopped) stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "SAFETY_STOP";
    dashboard.markStopApplied("CRITICAL_SAFETY");
  }
  dashboard.updateBackground();
  if (allowBackgroundWork) {
    imu.update();
    radio.update(currentEnvironmentalAlarmMask());
  }
  bool manualDeadmanTriggered = false;
  if (dashboard.getControlMode() == "MANUAL" &&
      manualMovementActive &&
      motors.getState() != MotorState::Stopped &&
      millis() - dashboard.lastCommandReceivedMs() >= RobotConfig::MANUAL_COMMAND_TIMEOUT_MS) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    manualOverride.movementStopped();
    dashboardData.navigationDecision = "MANUAL_DEADMAN_STOP";
    dashboardData.manualMotionInhibitReason = "MANUAL_DEADMAN";
    dashboard.markStopApplied("MANUAL_DEADMAN");
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
    dashboard.markStopApplied("MANUAL_SIDE_OBSTACLE");
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
  allowSlowOutputsThisPass = allowBackgroundWork;
  publishCurrentTelemetry(safetyChanged);
  navTrace.service(dashboardData);
  allowSlowOutputsThisPass = true;
  return safetyChanged;
}
void refreshManualForwardSonarGuard(const SensorSnapshot& snapshot,
                                    unsigned long now) {
  ManualSonarEvidence evidence = ManualSonarEvidence::Unknown;
  if (snapshot.distanceFacingFront && snapshot.distanceSampleMs != 0 &&
      now - snapshot.distanceSampleMs <= RobotConfig::ULTRASONIC_STALE_MS) {
    if (snapshot.ultrasonicState == UltrasonicResultState::DistanceValid &&
        snapshot.distanceValid && isfinite(snapshot.distanceCm) &&
        snapshot.distanceCm > 0.0f) {
      evidence = ManualSonarEvidence::DistanceValid;
    } else if (snapshot.ultrasonicState == UltrasonicResultState::ClearNoEcho) {
      evidence = ManualSonarEvidence::ClearNoEcho;
    }
  }
  manualForwardBlockedByUltrasonic = manualForwardSonarBlocked(
      manualForwardBlockedByUltrasonic, evidence, snapshot.distanceCm,
      RobotConfig::MANUAL_FORWARD_STOP_CM,
      RobotConfig::MANUAL_FORWARD_CLEAR_CM);
}
bool handleManualControl() {
  String command = dashboard.getPendingCommand();
  const unsigned long commandSequence = dashboard.pendingCommandSequence();

  if (command.length() == 0) {
    return false;
  }

  if (command != "STOP" && command != "RELEASE" &&
      millis() - dashboard.lastCommandReceivedMs() >= RobotConfig::MANUAL_COMMAND_TIMEOUT_MS) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "MANUAL_STALE_COMMAND_STOP";
    dashboard.markCommandApplied("STOP", commandSequence, "MANUAL_DEADMAN");
    syncDashboardState("MANUAL");
    return false;
  }

  if (safety.blocksMotion(manualOverride.exemptsFlameGas(
          dashboard.getControlMode() == "MANUAL", dashboard.sessionAlive())) &&
      command != "STOP" && command != "RELEASE") {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "SAFETY_STOP";
    dashboard.markCommandApplied("STOP", commandSequence, "SAFETY_INTERLOCK");
    syncDashboardState("MANUAL");
    return false;
  }

  if (command == "RELEASE") {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "MANUAL_RELEASE_STOP";
    dashboardData.manualMotionInhibitReason = "MANUAL_RELEASE";
    dashboard.markCommandApplied("STOP", commandSequence, "MANUAL_RELEASE");
    syncDashboardState("MANUAL");
    return true;
  }

  ManualGateInput gate;
  gate.manualMode = dashboard.getControlMode() == "MANUAL";
  gate.direction = command == "FORWARD" ? ManualDirection::Forward :
      command == "BACKWARD" ? ManualDirection::Backward :
      command == "LEFT" ? ManualDirection::Left : ManualDirection::Right;
  gate.flameGasCritical = safety.isAutoRecoverableEnvironmentalHazard();
  gate.temperatureCritical = safety.hasNonRecoverableCriticalHazard();
  gate.overrideEnabled = manualOverride.enabled();
  gate.sessionAlive = dashboard.sessionAlive();
  const SensorSnapshot& gateSnapshot = sensors.snapshot();
  gate.sensorFault = manualOverride.enabled() && !gateSnapshot.flameSampleValid;
  refreshManualForwardSonarGuard(gateSnapshot, millis());
  gate.forwardObstacle = manualForwardBlockedByUltrasonic ||
      safety.blocksManualForwardMotion(true);
  // Front IR and a fresh side inspection can veto a known obstruction.
  // The operator supervises the remaining unsensed turn sweep.
  const bool freshSideInspection = manualInspectionState.ready &&
      manualInspectionState.obstacle &&
      gateSnapshot.distanceSampleMs != 0 &&
      millis() - gateSnapshot.distanceSampleMs <= RobotConfig::NAVIGATION_SCAN_SET_MAX_AGE_MS;
  const bool nearFront=manualFrontTurnVeto(gateSnapshot, millis());
  gate.leftTurnObstacle = safety.isLeftIrObstacleActive() || nearFront ||
      (freshSideInspection && manualInspectionState.orientation == ManualSensorHeadCommand::Left);
  gate.rightTurnObstacle = safety.isRightIrObstacleActive() || nearFront ||
      (freshSideInspection && manualInspectionState.orientation == ManualSensorHeadCommand::Right);
  const ManualInhibit inhibit = manualMotionGate(gate);
  if (inhibit != ManualInhibit::None) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = manualInhibitName(inhibit);
    dashboardData.manualMotionInhibitReason = manualInhibitName(inhibit);
    dashboard.markCommandApplied("STOP", commandSequence, manualInhibitName(inhibit));
    syncDashboardState("MANUAL");
    return false;
  }

  if (command == "FORWARD" && manualForwardBlockedByUltrasonic) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "MANUAL_FORWARD_HOLD";
    dashboard.markCommandApplied("STOP", commandSequence, "ULTRASONIC_HOLD");
    syncDashboardState("MANUAL");
    return false;
  }

  if (command == "FORWARD" && safety.blocksManualForwardMotion(
          manualOverride.exemptsFlameGas(true, dashboard.sessionAlive()))) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = safety.obstacleDecision(sensors.snapshot()) == "ULTRASONIC_BLOCK"
      ? "FORWARD_BLOCKED_ULTRASONIC"
      : "FORWARD_BLOCKED_BOTH_IR";
    dashboard.markCommandApplied("STOP", commandSequence, "OBSTACLE_INTERLOCK");
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
    if (command == currentManualState && motors.getState() != MotorState::Stopped) {
      dashboard.markCommandApplied(command, commandSequence, "HELD_REFRESH");
      return true;
    }
    dashboard.markCommandApplied("STOP", commandSequence, "COMMAND_REJECTED");
    return false;
  }

  if (criticalStopLatched && !safety.isCritical()) {
    criticalStopLatched = false;
  }
  manualMotorDisplayState = result.motorDisplayState;
  dashboardData.navigationDecision = result.navigationDecision;
  dashboardData.manualMotionInhibitReason =
      command == "BACKWARD" ? "REAR_CLEARANCE_NOT_SENSOR_VERIFIED" :
      (command == "LEFT" || command == "RIGHT") ? "TURN_OPERATOR_SUPERVISED" : "NONE";
  dashboard.markCommandApplied(command,
                               commandSequence,
                               command == "STOP" ? "OPERATOR_STOP" : "NONE");
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

  refreshManualForwardSonarGuard(snapshot, now);
  if (snapshot.distanceSampleSequence != lastManualGuardSequence &&
      snapshot.distanceFacingFront) {
    lastManualGuardSequence = snapshot.distanceSampleSequence;
    if (snapshot.ultrasonicState == UltrasonicResultState::DistanceValid &&
        snapshot.distanceValid && manualForwardBlockedByUltrasonic &&
        motors.getState() == MotorState::Forward) {
      stopMotors();
      manualMotorDisplayState = "STOP";
      dashboard.setPendingCommand("");
      dashboardData.navigationDecision = "MANUAL_OBSTACLE_STOP";
      dashboard.markStopApplied("ULTRASONIC_NEAR");
      syncDashboardState("MANUAL");
      Serial.println("Manual fresh HC-SR04 forward stop.");
    }
  }

  if (motors.getState() == MotorState::Forward &&
      safety.blocksManualForwardMotion(manualOverride.exemptsFlameGas(
          true, dashboard.sessionAlive()))) {
    stopMotors();
    manualMotorDisplayState = "STOP";
    dashboard.setPendingCommand("");
    dashboardData.navigationDecision = "FORWARD_BLOCKED_NEAR";
    dashboard.markStopApplied("NEAR_OBSTACLE");
    syncDashboardState("MANUAL");
    Serial.println("Manual near obstacle stop.");
  }
}

void setup() {
  Serial.begin(115200);
  navTrace.begin(esp_random());
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

  dashboardData.resetReason = resetReasonName(esp_reset_reason());
  dashboard.setControlCallbacks(onDashboardCommandAccepted, onDashboardModeAccepted);
  dashboard.setOverrideCallback(onManualOverrideAction);
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

  if (safety.blocksMotion(manualOverride.exemptsFlameGas(
          controlMode == "MANUAL", dashboard.sessionAlive()))) {
    if (controlMode == "AUTO" &&
        safety.isAutoRecoverableEnvironmentalHazard() &&
        !criticalStopRequiresManualRearm) {
      navigation.updateAuto();
    }
    publishCurrentTelemetry(true);
    delay(1);
    return;
  }

  if (criticalStopLatched) {
    if (dashboard.modeCommandCounter() != criticalStopModeCommandCounter) {
      criticalStopLatched = false;
      criticalStopRequiresManualRearm = false;
      stopMotors();
      dashboardData.navigationDecision = controlMode == "AUTO" ? "AUTO_REARMED" : "MANUAL_CONTROL";
      dashboard.setNavigationDecision(dashboardData.navigationDecision);
    } else if (controlMode == "AUTO") {
      if (!criticalStopRequiresManualRearm && navigation.environmentalRecoveryInProgress()) {
        navigation.updateAuto();
        if (navigation.consumeEnvironmentalRecoveryCompleted()) {
          criticalStopLatched = false;
          dashboardData.navigationDecision = "ENVIRONMENTAL_RECOVERY_COMPLETE";
          dashboard.setNavigationDecision(dashboardData.navigationDecision);
        } else {
          publishCurrentTelemetry(true);
          delay(1);
          return;
        }
      }
    }
    if (criticalStopLatched && controlMode == "AUTO") {
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
