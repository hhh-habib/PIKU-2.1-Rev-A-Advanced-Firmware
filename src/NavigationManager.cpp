#include "NavigationManager.h"
#include "RobotConfig.h"

NavigationManager::NavigationManager(MotorController& motors,
                                     SensorManager& sensors,
                                     ServoScanner& servoScanner,
                                     SafetyManager& safety,
                                     WebDashboard& dashboard,
                                     DashboardData& dashboardData,
                                     String& currentServoPosition,
                                     ServiceCallback serviceCallback,
                                     PublishCallback publishCallback,
                                     WarningCallback warningCallback)
  : _motors(motors),
    _sensors(sensors),
    _servoScanner(servoScanner),
    _safety(safety),
    _dashboard(dashboard),
    _dashboardData(dashboardData),
    _currentServoPosition(currentServoPosition),
    _serviceCallback(serviceCallback),
    _publishCallback(publishCallback),
    _warningCallback(warningCallback),
    _state(NavigationState::Idle),
    _lastNearRecoveryMs(0),
    _nearRecoveryLatched(false),
    _operatorStopLatched(false),
    _operatorStopModeCommandCounter(0) {
}

void NavigationManager::updateAuto() {
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return;
  }
  resetRecoveryLatchIfClear();

  _motors.stop();

  setState(NavigationState::ScanFront, "SENSE_FRONT");
  float front = -1.0f;
  bool frontTrusted = acquireFreshFrontDistance(front);
  const SensorSnapshot& snapshot = _sensors.snapshot();

  if (abortIfOperatorRequested() || abortIfCritical()) {
    return;
  }

  if (!frontTrusted) {
    Serial.println("Ultrasonic sensor fault. Holding position.");
    setState(NavigationState::InvalidStop, "ULTRASONIC_INVALID_STOP");
    _motors.stop();
    responsiveDelay(250);
  } else if (_safety.isNearObstacleHazardActive()) {
    runNearObstacleRecovery();
  } else if (snapshot.ultrasonicState == UltrasonicResultState::DistanceValid &&
             front <= RobotConfig::AUTO_OBSTACLE_CM) {
    runObstacleAvoidance();
  } else if (!_safety.isCritical()) {
    driveForwardWithFreshGuard();
  } else {
    setState(NavigationState::InvalidStop, "ULTRASONIC_INVALID_STOP");
    _motors.stop();
    responsiveDelay(250);
  }
}

void NavigationManager::resetRecoveryLatchIfClear() {
  if (!_safety.isNearObstacleHazardActive()) {
    _nearRecoveryLatched = false;
  }
}

NavigationState NavigationManager::state() const {
  return _state;
}

const char* NavigationManager::stateName() const {
  switch (_state) {
    case NavigationState::Idle: return "Idle";
    case NavigationState::ScanFront: return "ScanFront";
    case NavigationState::Forward: return "Forward";
    case NavigationState::ObstacleReverse: return "ObstacleReverse";
    case NavigationState::ScanLeft: return "ScanLeft";
    case NavigationState::ScanRight: return "ScanRight";
    case NavigationState::TurnLeft: return "TurnLeft";
    case NavigationState::TurnRight: return "TurnRight";
    case NavigationState::InvalidStop: return "InvalidStop";
    case NavigationState::NearObstacleHold: return "NearObstacleHold";
    case NavigationState::NearObstacleRecovery: return "NearObstacleRecovery";
    case NavigationState::NearObstacleReverse: return "NearObstacleReverse";
    case NavigationState::NearObstacleScan: return "NearObstacleScan";
    case NavigationState::NearObstacleTurn: return "NearObstacleTurn";
    case NavigationState::NearObstacleCooldown: return "NearObstacleCooldown";
    case NavigationState::OperatorAbort: return "OperatorAbort";
    case NavigationState::CriticalAbort: return "CriticalAbort";
  }
  return "Unknown";
}

void NavigationManager::setState(NavigationState state, const String& decision) {
  _state = state;
  _dashboardData.navigationDecision = decision;
  _dashboard.setNavigationDecision(decision);
}

bool NavigationManager::responsiveDelay(unsigned long durationMs) {
  unsigned long startMs = millis();
  while (millis() - startMs < durationMs) {
    _sensors.setDistanceFacingFront(_currentServoPosition == "FRONT");
    if (_serviceCallback != nullptr) {
      _serviceCallback();
    }
    if (abortIfOperatorRequested() || abortIfCritical()) {
      return true;
    }
    delay(1);
  }
  return false;
}

float NavigationManager::scanAt(int angle, const String& name) {
  _motors.stop();
  _currentServoPosition = name;
  _sensors.setDistanceFacingFront(name == "FRONT");
  if (responsiveDelay(250)) {
    return -1;
  }

  if (angle == RobotConfig::CENTER_SCAN) {
    _servoScanner.center();
  } else if (angle == RobotConfig::LEFT_SCAN) {
    _servoScanner.lookLeft();
  } else if (angle == RobotConfig::RIGHT_SCAN) {
    _servoScanner.lookRight();
  } else {
    _servoScanner.writeAngle(angle);
  }
  if (responsiveDelay(700)) {
    _servoScanner.detach();
    return -1;
  }

  unsigned long previousSequence = _sensors.distanceSampleSequence();
  float distance = readAndPublishDistance(name);
  const SensorSnapshot& snapshot = _sensors.snapshot();

  _servoScanner.detach();
  if (responsiveDelay(250)) {
    return -1.0f;
  }

  if (snapshot.distanceSampleSequence <= previousSequence) {
    return -1.0f;
  }
  if (snapshot.ultrasonicState == UltrasonicResultState::ClearNoEcho) {
    return RobotConfig::ULTRASONIC_MAX_CM;
  }
  return distance;
}

float NavigationManager::readAndPublishDistance(const String& servoPos) {
  _currentServoPosition = servoPos;
  bool facingFront = servoPos == "FRONT";
  _sensors.setDistanceFacingFront(facingFront);
  unsigned long targetSample = _sensors.requestDistanceSample(facingFront);
  unsigned long startMs = millis();
  while (_sensors.distanceSampleSequence() < targetSample &&
         millis() - startMs < RobotConfig::ULTRASONIC_SAMPLE_TIMEOUT_MS) {
    if (_serviceCallback != nullptr) {
      _serviceCallback();
    }
    if (_safety.isCritical()) {
      break;
    }
    if (abortIfOperatorRequested()) {
      break;
    }
    delay(1);
  }

  _safety.update(_sensors.snapshot());
  if (_publishCallback != nullptr) {
    _publishCallback(true);
  }
  return _sensors.snapshot().distanceCm;
}

bool NavigationManager::acquireFreshFrontDistance(float& distanceCm) {
  _motors.stop();
  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.center();
  if (responsiveDelay(120)) {
    _servoScanner.detach();
    distanceCm = -1.0f;
    return false;
  }

  unsigned long previousSequence = _sensors.distanceSampleSequence();
  distanceCm = readAndPublishDistance("FRONT");
  const SensorSnapshot& snapshot = _sensors.snapshot();
  if (snapshot.distanceSampleSequence <= previousSequence) {
    return false;
  }
  if (!snapshot.distanceFacingFront) {
    return false;
  }
  if (snapshot.ultrasonicState == UltrasonicResultState::DistanceValid) {
    return true;
  }
  if (snapshot.ultrasonicState == UltrasonicResultState::ClearNoEcho ||
      snapshot.ultrasonicState == UltrasonicResultState::SensorFault) {
    distanceCm = RobotConfig::ULTRASONIC_MAX_CM;
    return true;
  }
  return false;
}

bool NavigationManager::driveForwardWithFreshGuard() {
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return true;
  }
  Serial.println("Decision: FORWARD_STEP");
  setState(NavigationState::Forward, "FORWARD_STEP");
  _motors.forward();
  unsigned long lastSampleRequestMs = 0;
  unsigned long stepStartedMs = millis();

  while (_dashboard.getControlMode() == "AUTO" &&
         millis() - stepStartedMs < RobotConfig::AUTO_FORWARD_STEP_MS) {
    if (abortIfOperatorRequested() || abortIfCritical()) {
      _motors.stop();
      return true;
    }
    if (_safety.isNearObstacleHazardActive()) {
      _motors.stop();
      runNearObstacleRecovery();
      return true;
    }

    unsigned long now = millis();
    if (lastSampleRequestMs == 0 ||
        now - lastSampleRequestMs >= RobotConfig::ULTRASONIC_GUARD_INTERVAL_MS) {
      lastSampleRequestMs = now;
      unsigned long previousSequence = _sensors.distanceSampleSequence();
      float front = readAndPublishDistance("FRONT");
      const SensorSnapshot& snapshot = _sensors.snapshot();

      if (abortIfOperatorRequested() || abortIfCritical()) {
        _motors.stop();
        return true;
      }
      if (snapshot.distanceSampleSequence <= previousSequence ||
          !snapshot.distanceFacingFront) {
        Serial.println("Ultrasonic sensor fault while moving forward. Holding position.");
        setState(NavigationState::InvalidStop, "ULTRASONIC_INVALID_STOP");
        _motors.stop();
        return true;
      }
      if (_safety.isNearObstacleHazardActive()) {
        _motors.stop();
        runNearObstacleRecovery();
        return true;
      }
      if (snapshot.ultrasonicState == UltrasonicResultState::DistanceValid &&
          front <= RobotConfig::AUTO_OBSTACLE_CM) {
        Serial.println("AUTO obstacle threshold crossed. Avoiding.");
        _motors.stop();
        runObstacleAvoidance();
        return true;
      }
      setState(NavigationState::Forward, "FORWARD_STEP");
      _motors.forward();
    }

    if (_serviceCallback != nullptr) {
      _serviceCallback();
    }
    if (abortIfOperatorRequested() || abortIfCritical()) {
      return true;
    }
    delay(1);
  }

  if (abortIfOperatorRequested() || abortIfCritical()) {
    return true;
  }
  _motors.stop();
  setState(NavigationState::ScanFront, "STOP_SETTLE");
  responsiveDelay(RobotConfig::AUTO_STOP_SETTLE_MS);
  return true;
}

void NavigationManager::runObstacleAvoidance() {
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return;
  }

  Serial.println("Obstacle detected. Moving backward.");
  setState(NavigationState::ObstacleReverse, "BACKWARD_RECOVERY");
  _motors.stop();
  if (responsiveDelay(50)) return;
  _motors.backward();
  if (responsiveDelay(RobotConfig::AUTO_OBSTACLE_REVERSE_MS)) return;
  _motors.stop();
  if (responsiveDelay(250)) return;
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return;
  }

  Serial.println("Scanning left, center, and right.");
  _state = NavigationState::ScanLeft;
  float left = scanAt(RobotConfig::LEFT_SCAN, "LEFT");
  if (abortIfOperatorRequested() || abortIfCritical()) return;
  _state = NavigationState::ScanFront;
  float center = scanAt(RobotConfig::CENTER_SCAN, "FRONT");
  if (abortIfOperatorRequested() || abortIfCritical()) return;
  _state = NavigationState::ScanRight;
  float right = scanAt(RobotConfig::RIGHT_SCAN, "RIGHT");

  if (abortIfOperatorRequested() || abortIfCritical()) {
    return;
  }

  _servoScanner.center();
  if (responsiveDelay(400)) return;
  _servoScanner.detach();
  choosePathAndTurn(left, center, right, false);
  responsiveDelay(300);
}

void NavigationManager::choosePathAndTurn(float left, float center, float right, bool nearRecovery) {
  const bool leftSafe = left > RobotConfig::AUTO_OBSTACLE_CM;
  const bool centerSafe = center > RobotConfig::AUTO_OBSTACLE_CM;
  const bool rightSafe = right > RobotConfig::AUTO_OBSTACLE_CM;

  if (!leftSafe && !centerSafe && !rightSafe) {
    Serial.println("No scanned path is safely above the obstacle threshold. Holding position.");
    setState(NavigationState::InvalidStop, nearRecovery ? "NEAR_NO_SAFE_PATH_STOP" : "NO_SAFE_PATH_STOP");
    _motors.stop();
    return;
  }

  if (centerSafe && (!leftSafe || center >= left) && (!rightSafe || center >= right)) {
    Serial.println("Decision: CENTER CLEAR");
    setState(NavigationState::Forward, nearRecovery ? "NEAR_CENTER_CLEAR" : "CENTER_CLEAR");
    _motors.stop();
    return;
  }

  if (leftSafe && (!rightSafe || left > right)) {
    Serial.println("Decision: LEFT");
    setState(nearRecovery ? NavigationState::NearObstacleTurn : NavigationState::TurnLeft,
             nearRecovery ? "NEAR_TURN_LEFT" : "LEFT");
    _motors.leftTurn();
    if (responsiveDelay(170)) return;
    _motors.stop();
  } else {
    Serial.println("Decision: RIGHT");
    setState(nearRecovery ? NavigationState::NearObstacleTurn : NavigationState::TurnRight,
             nearRecovery ? "NEAR_TURN_RIGHT" : "RIGHT");
    _motors.rightTurn();
    if (responsiveDelay(170)) return;
    _motors.stop();
  }
}

bool NavigationManager::runNearObstacleRecovery() {
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return true;
  }
  // Preserve the debounced IR side that triggered recovery. The sensors can
  // clear while reversing or scanning, but the first avoidance choice should
  // still turn away from the detected side.
  const bool leftIrTriggered = _safety.isLeftIrObstacleActive();
  const bool rightIrTriggered = _safety.isRightIrObstacleActive();
  unsigned long now = millis();
  if (_nearRecoveryLatched && _safety.isNearObstacleHazardActive()) {
    _motors.stop();
    setState(NavigationState::NearObstacleHold, "NEAR_OBSTACLE_HOLD");
    if (_warningCallback != nullptr) {
      _warningCallback();
    }
    responsiveDelay(100);
    return true;
  }

  if (_lastNearRecoveryMs != 0 && now - _lastNearRecoveryMs < RobotConfig::IR_RECOVERY_COOLDOWN_MS) {
    _motors.stop();
    setState(NavigationState::NearObstacleCooldown, "NEAR_RECOVERY_COOLDOWN");
    responsiveDelay(100);
    return true;
  }

  _nearRecoveryLatched = true;
  _lastNearRecoveryMs = now;
  _motors.stop();
  if (_warningCallback != nullptr) {
    _warningCallback();
  }
  setState(NavigationState::NearObstacleRecovery, "NEAR_OBSTACLE_RECOVERY");
  if (_publishCallback != nullptr) {
    _publishCallback(true);
  }
  if (responsiveDelay(RobotConfig::IR_RECOVERY_SETTLE_MS)) return true;
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return true;
  }

  // BOTH IR means hold first and inspect all three directions before choosing.
  if (!(leftIrTriggered && rightIrTriggered)) {
    setState(NavigationState::NearObstacleReverse, "NEAR_REVERSE");
    _motors.backward();
    if (responsiveDelay(RobotConfig::IR_RECOVERY_REVERSE_MS)) return true;
    _motors.stop();
    if (responsiveDelay(250)) return true;
  }
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return true;
  }

  _state = NavigationState::NearObstacleScan;
  float left = scanAt(RobotConfig::LEFT_SCAN, "LEFT");
  if (abortIfOperatorRequested() || abortIfCritical()) return true;
  _state = NavigationState::NearObstacleScan;
  float center = scanAt(RobotConfig::CENTER_SCAN, "FRONT");
  if (abortIfOperatorRequested() || abortIfCritical()) return true;
  _state = NavigationState::NearObstacleScan;
  float right = scanAt(RobotConfig::RIGHT_SCAN, "RIGHT");
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return true;
  }

  _servoScanner.center();
  if (responsiveDelay(400)) return true;
  _servoScanner.detach();

  const bool leftSafe = left > RobotConfig::AUTO_OBSTACLE_CM;
  const bool rightSafe = right > RobotConfig::AUTO_OBSTACLE_CM;
  if (leftIrTriggered && !rightIrTriggered && rightSafe) {
    setState(NavigationState::NearObstacleTurn, "LEFT_IR_TURN_RIGHT");
    _motors.rightTurn();
    if (responsiveDelay(170)) return true;
    _motors.stop();
  } else if (rightIrTriggered && !leftIrTriggered && leftSafe) {
    setState(NavigationState::NearObstacleTurn, "RIGHT_IR_TURN_LEFT");
    _motors.leftTurn();
    if (responsiveDelay(170)) return true;
    _motors.stop();
  } else {
    choosePathAndTurn(left, center, right, true);
  }

  if (responsiveDelay(300)) return true;
  _safety.update(_sensors.snapshot());
  resetRecoveryLatchIfClear();
  return true;
}
bool NavigationManager::abortIfOperatorRequested() {
  unsigned long modeCounter = _dashboard.modeCommandCounter();
  if (_operatorStopLatched && modeCounter != _operatorStopModeCommandCounter) {
    _operatorStopLatched = false;
  }

  if (_dashboard.getPendingCommand() == "STOP") {
    _dashboard.setPendingCommand("");
    _operatorStopLatched = true;
    _operatorStopModeCommandCounter = modeCounter;
    _motors.stop();
    setState(NavigationState::OperatorAbort, "AUTO_OPERATOR_STOP");
    if (_publishCallback != nullptr) {
      _publishCallback(true);
    }
    return true;
  }

  if (_dashboard.getControlMode() != "AUTO") {
    _motors.stop();
    setState(NavigationState::OperatorAbort, "AUTO_MODE_CHANGE");
    if (_publishCallback != nullptr) {
      _publishCallback(true);
    }
    return true;
  }

  if (_operatorStopLatched) {
    _motors.stop();
    setState(NavigationState::OperatorAbort, "AUTO_OPERATOR_STOP");
    return true;
  }

  return false;
}

bool NavigationManager::abortIfCritical() {
  if (!_safety.isCritical()) {
    return false;
  }

  _motors.stop();
  setState(NavigationState::CriticalAbort, "SAFETY_STOP");
  if (_publishCallback != nullptr) {
    _publishCallback(true);
  }
  return true;
}
