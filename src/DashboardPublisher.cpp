#include "DashboardPublisher.h"
#include "RobotConfig.h"

namespace {
  const char* ultrasonicStateName(UltrasonicResultState state) {
    switch (state) {
      case UltrasonicResultState::DistanceValid: return "DISTANCE_VALID";
      case UltrasonicResultState::ClearNoEcho: return "CLEAR_NO_ECHO";
      case UltrasonicResultState::SensorFault: return "SENSOR_FAULT";
      case UltrasonicResultState::Stale: return "STALE";
    }
    return "STALE";
  }

  const char* flameStateName(FlameState state) {
    switch (state) {
      case FlameState::Clear: return "CLEAR";
      case FlameState::Elevated: return "ELEVATED";
      case FlameState::PossibleFlame: return "POSSIBLE_FLAME";
      case FlameState::FlameDetected: return "FLAME_DETECTED";
    }
    return "CLEAR";
  }
}

DashboardPublisher::DashboardPublisher(WebDashboard& webDashboard, DisplayManager& displayManager, DashboardData& dashboardData)
  : dashboard(webDashboard),
    display(displayManager),
    data(dashboardData),
    hasPublishedData(false),
    lastSerialLogMs(0) {
}

void DashboardPublisher::syncState(const String& controlMode,
                                   const String& turnMode,
                                   const String& motorStateName,
                                   const String& manualMotorDisplayState) {
  data.controlMode = controlMode;
  data.turnMode = turnMode;
  if (controlMode == "MANUAL" && manualMotorDisplayState.length() > 0) {
    data.motorState = manualMotorDisplayState;
  } else {
    data.motorState = motorStateName;
  }

  dashboard.setData(data);
  cachePublishedDashboardData();
}

void DashboardPublisher::publishSensorReadings(const String& servoPos,
                                               int servoAngle,
                                               const SensorSnapshot& snapshot,
                                               bool leftObstacleFiltered,
                                               bool rightObstacleFiltered,
                                               int speedPercent,
                                               const String& obstacleDecision,
                                               const String& gasStatus,
                                               const String& safetyState,
                                               const String& alarmReason,
                                               bool nearObstacleHazard,
                                               const String& nearObstacleSource,
                                               bool ultrasonicWarning,
                                               const String& alarmSoundState,
                                               const String& buzzerState,
                                               const String& controlMode,
                                               const String& turnMode,
                                               const String& motorStateName,
                                               const String& manualMotorDisplayState,
                                               const String& navigationStatus,
                                               bool allowSlowOutputs) {
  data.temperature = snapshot.temperature;
  data.temperatureValid = snapshot.temperatureValid;
  data.humidity = snapshot.humidity;
  data.humidityValid = snapshot.humidityValid;
  data.dhtErrorCount = snapshot.dhtErrorCount;
  data.gasValue = snapshot.gasRaw;
  data.gasFilteredValue = snapshot.gasFiltered;
  data.mq2Warmup = snapshot.mq2Warmup;
  data.mq2WarmupAgeMs = snapshot.mq2WarmupAgeMs;
  data.gasStatus = gasStatus;
  data.frontDistance = snapshot.distanceCm;
  data.distanceValid = snapshot.distanceValid;
  data.ultrasonicState = ultrasonicStateName(snapshot.ultrasonicState);
  data.distanceSampleSequence = snapshot.distanceSampleSequence;
  data.ultrasonicInvalidCount = snapshot.ultrasonicInvalidCount;
  data.ultrasonicFaultCount = snapshot.ultrasonicFaultCount;
  data.servoAngle = servoAngle;
  data.scanOrientation = servoPos;
  data.flameDetected = snapshot.flameDetected;
  data.flameState = flameStateName(snapshot.flameState);
  data.flameDigitalLevel = snapshot.flameDigitalLevel;
  data.flameRawActive = snapshot.flameRawActive;
  data.flameSampleValid = snapshot.flameSampleValid;
  data.leftIrRaw = snapshot.leftIrRaw;
  data.rightIrRaw = snapshot.rightIrRaw;
  data.leftObstacle = leftObstacleFiltered;
  data.rightObstacle = rightObstacleFiltered;
  data.irObstacleDetected = leftObstacleFiltered || rightObstacleFiltered;
  data.speedPercent = speedPercent;
  data.obstacleDecision = obstacleDecision;
  data.nearObstacleHazard = nearObstacleHazard;
  data.nearObstacleSource = nearObstacleSource;
  data.ultrasonicWarning = ultrasonicWarning;
  data.safetyState = safetyState;
  data.alarmReason = alarmReason;
  data.alarmSoundState = alarmSoundState;
  data.buzzerState = buzzerState;
  data.controlMode = controlMode;
  data.turnMode = turnMode;
  data.uptimeMs = millis();
  if (controlMode == "MANUAL" && manualMotorDisplayState.length() > 0) {
    data.motorState = manualMotorDisplayState;
  } else {
    data.motorState = motorStateName;
  }

  if (allowSlowOutputs) {
    display.updateDashboard(data);
  }

  publishIfChanged();
  if (allowSlowOutputs) {
    logSensorReadings(servoPos, snapshot, leftObstacleFiltered, rightObstacleFiltered, motorStateName);
  }
}

bool DashboardPublisher::floatChanged(float current, float previous, float threshold) const {
  if (isnan(current) && isnan(previous)) {
    return false;
  }
  if (isnan(current) || isnan(previous)) {
    return true;
  }
  return fabs(current - previous) >= threshold;
}

bool DashboardPublisher::dashboardDataChanged(const DashboardData& current, const DashboardData& previous) const {
  return floatChanged(current.temperature, previous.temperature, 0.1f) ||
         floatChanged(current.humidity, previous.humidity, 0.1f) ||
         current.temperatureValid != previous.temperatureValid ||
         current.humidityValid != previous.humidityValid ||
         current.dhtErrorCount != previous.dhtErrorCount ||
         floatChanged(current.frontDistance, previous.frontDistance, 0.5f) ||
         abs(current.gasValue - previous.gasValue) >= 5 ||
         abs(current.gasFilteredValue - previous.gasFilteredValue) >= 5 ||
         current.distanceValid != previous.distanceValid ||
         current.ultrasonicState != previous.ultrasonicState ||
         current.distanceSampleSequence != previous.distanceSampleSequence ||
         current.ultrasonicInvalidCount != previous.ultrasonicInvalidCount ||
         current.ultrasonicFaultCount != previous.ultrasonicFaultCount ||
         current.servoAngle != previous.servoAngle ||
         current.scanOrientation != previous.scanOrientation ||
         current.manualInspectionObstacle != previous.manualInspectionObstacle ||
         current.manualInspectionOrientation != previous.manualInspectionOrientation ||
         current.mq2Warmup != previous.mq2Warmup ||
         current.flameDetected != previous.flameDetected ||
         current.flameState != previous.flameState ||
         current.flameDigitalLevel != previous.flameDigitalLevel ||
         current.flameRawActive != previous.flameRawActive ||
         current.flameSampleValid != previous.flameSampleValid ||
         current.leftIrRaw != previous.leftIrRaw ||
         current.rightIrRaw != previous.rightIrRaw ||
         current.leftObstacle != previous.leftObstacle ||
         current.rightObstacle != previous.rightObstacle ||
         current.irObstacleDetected != previous.irObstacleDetected ||
         current.nearObstacleHazard != previous.nearObstacleHazard ||
         current.nearObstacleSource != previous.nearObstacleSource ||
         current.ultrasonicWarning != previous.ultrasonicWarning ||
         current.gasStatus != previous.gasStatus ||
         current.safetyState != previous.safetyState ||
         current.alarmReason != previous.alarmReason ||
         current.alarmSoundState != previous.alarmSoundState ||
         current.buzzerState != previous.buzzerState ||
         current.motorState != previous.motorState ||
         current.speedPercent != previous.speedPercent ||
         current.obstacleDecision != previous.obstacleDecision ||
         current.navigationDecision != previous.navigationDecision ||
         current.autoNavigationState != previous.autoNavigationState ||
         current.autoIrZoneState != previous.autoIrZoneState ||
         current.autoAvoidanceSource != previous.autoAvoidanceSource ||
         current.autoPreferredPath != previous.autoPreferredPath ||
         current.autoObstacleRecoveryAttempts != previous.autoObstacleRecoveryAttempts ||
         current.autoBlockedRetryCount != previous.autoBlockedRetryCount ||
         floatChanged(current.autoRelativeTurnDeg, previous.autoRelativeTurnDeg, 0.5f) ||
         current.autoSelectedPath != previous.autoSelectedPath ||
         current.autoTurnCompletionReason != previous.autoTurnCompletionReason ||
         current.autoScanLeftState != previous.autoScanLeftState ||
         current.autoScanFrontState != previous.autoScanFrontState ||
         current.autoScanRightState != previous.autoScanRightState ||
         floatChanged(current.autoScanLeftDistanceCm, previous.autoScanLeftDistanceCm, 0.5f) ||
         floatChanged(current.autoScanFrontDistanceCm, previous.autoScanFrontDistanceCm, 0.5f) ||
         floatChanged(current.autoScanRightDistanceCm, previous.autoScanRightDistanceCm, 0.5f) ||
         current.autoScanLeftTrusted != previous.autoScanLeftTrusted ||
         current.autoScanFrontTrusted != previous.autoScanFrontTrusted ||
         current.autoScanRightTrusted != previous.autoScanRightTrusted ||
         current.autoHazardRecoveryState != previous.autoHazardRecoveryState ||
         current.autoHazardClearRemainingMs != previous.autoHazardClearRemainingMs ||
         current.autoStuckState != previous.autoStuckState ||
         current.autoStuckRecoveryAttempts != previous.autoStuckRecoveryAttempts ||
         current.autoStuckLastReason != previous.autoStuckLastReason ||
         current.controlMode != previous.controlMode ||
         current.turnMode != previous.turnMode ||
         current.softApIp != previous.softApIp ||
         current.localHostname != previous.localHostname ||
         current.mdnsActive != previous.mdnsActive ||
         current.staConfigured != previous.staConfigured ||
         current.staSsid != previous.staSsid ||
         current.staStatus != previous.staStatus ||
         current.staConnected != previous.staConnected ||
         current.staIp != previous.staIp ||
         current.staRssi != previous.staRssi ||
         current.staSignalQuality != previous.staSignalQuality ||
         current.softApChannel != previous.softApChannel ||
         current.softApClients != previous.softApClients ||
         current.freeHeap != previous.freeHeap ||
         current.minimumFreeHeap != previous.minimumFreeHeap ||
         current.maximumServiceLoopGapMs != previous.maximumServiceLoopGapMs ||
         current.imuInitialized != previous.imuInitialized ||
         current.imuHealthy != previous.imuHealthy ||
         current.imuHealthState != previous.imuHealthState ||
         current.imuErrorCount != previous.imuErrorCount ||
         current.imuConsecutiveFailures != previous.imuConsecutiveFailures ||
         floatChanged(current.accelX, previous.accelX, 0.1f) ||
         floatChanged(current.accelY, previous.accelY, 0.1f) ||
         floatChanged(current.accelZ, previous.accelZ, 0.1f) ||
         floatChanged(current.gyroX, previous.gyroX, 0.05f) ||
         floatChanged(current.gyroY, previous.gyroY, 0.05f) ||
         floatChanged(current.gyroZ, previous.gyroZ, 0.05f) ||
         floatChanged(current.roll, previous.roll, 0.2f) ||
         floatChanged(current.pitch, previous.pitch, 0.2f) ||
         current.motionState != previous.motionState ||
         current.radioInitialized != previous.radioInitialized ||
         current.radioEnabled != previous.radioEnabled ||
         current.radioChipDetected != previous.radioChipDetected ||
         current.radioListening != previous.radioListening ||
         current.radioTxAttempts != previous.radioTxAttempts ||
         current.radioTxSuccess != previous.radioTxSuccess ||
         current.radioTxFailure != previous.radioTxFailure ||
         current.radioRxPackets != previous.radioRxPackets ||
         current.radioPeer != previous.radioPeer ||
         current.displayInitialized != previous.displayInitialized;
}

void DashboardPublisher::cachePublishedDashboardData() {
  lastPublishedData = data;
  hasPublishedData = true;
}

void DashboardPublisher::publishIfChanged() {
  if (!hasPublishedData || dashboardDataChanged(data, lastPublishedData)) {
    dashboard.setData(data);
    cachePublishedDashboardData();
  }
}

void DashboardPublisher::logSensorReadings(const String& servoPos,
                                           const SensorSnapshot& snapshot,
                                           bool leftObstacleFiltered,
                                           bool rightObstacleFiltered,
                                           const String& motorStateName) {
  unsigned long now = millis();
  if (lastSerialLogMs != 0 && now - lastSerialLogMs < RobotConfig::SERIAL_LOG_INTERVAL_MS) {
    return;
  }
  lastSerialLogMs = now;
  Serial.println("----------------------");
  Serial.print("Servo   : "); Serial.println(servoPos);
  Serial.print("Distance: "); Serial.println(snapshot.distanceCm);
  Serial.print("Left IR : raw="); Serial.print(snapshot.leftIrRaw);
  Serial.print(" filtered="); Serial.println(leftObstacleFiltered ? "BLOCK" : "CLEAR");
  Serial.print("Right IR: raw="); Serial.print(snapshot.rightIrRaw);
  Serial.print(" filtered="); Serial.println(rightObstacleFiltered ? "BLOCK" : "CLEAR");
  Serial.print("Environment: temp=");
  if (snapshot.temperatureValid) Serial.print(snapshot.temperature, 1); else Serial.print("INVALID");
  Serial.print(" hum=");
  if (snapshot.humidityValid) Serial.print(snapshot.humidity, 1); else Serial.print("INVALID");
  Serial.print(" gas="); Serial.print(snapshot.gasFiltered);
  Serial.print(" state="); Serial.print(data.gasStatus);
  Serial.print(" flame_do="); Serial.print(snapshot.flameDigitalLevel == LOW ? "LOW" : "HIGH");
  Serial.print(" raw_active="); Serial.print(snapshot.flameRawActive ? "YES" : "NO");
  Serial.print(" state="); Serial.print(flameStateName(snapshot.flameState));
  Serial.print(" sample_valid="); Serial.println(snapshot.flameSampleValid ? "YES" : "NO");
  Serial.print("Motor   : "); Serial.println(motorStateName);
}
