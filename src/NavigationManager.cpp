#include "NavigationManager.h"
#include "RobotConfig.h"

NavigationManager::NavigationManager(MotorController& motors,
                                     SensorManager& sensors,
                                     ImuManager& imu,
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
    _imu(imu),
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
  UltrasonicSample front;
  bool frontTrusted = acquireFreshFrontDistance(front);

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
  } else if (front.state == UltrasonicResultState::DistanceValid &&
             front.distanceCm <= RobotConfig::AUTO_OBSTACLE_CM) {
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

NavigationManager::NavigationScanSample NavigationManager::scanAt(int angle, const char* name) {
  NavigationScanSample result;
  result.angle = angle;
  result.direction = name;
  _motors.stop();
  _currentServoPosition = name;
  _sensors.setDistanceFacingFront(strcmp(name, "FRONT") == 0);
  if (responsiveDelay(250)) {
    return result;
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
    return result;
  }

  unsigned long previousSequence = _sensors.distanceSampleSequence();
  result.ultrasonic = readAndPublishDistance(name);
  result.completed = result.ultrasonic.sequence > previousSequence &&
    result.ultrasonic.sampleMs != 0 &&
    millis() - result.ultrasonic.sampleMs <= RobotConfig::ULTRASONIC_STALE_MS;

  _servoScanner.detach();
  if (responsiveDelay(250)) {
    result.completed = false;
  }
  return result;
}

UltrasonicSample NavigationManager::readAndPublishDistance(const char* servoPos) {
  _currentServoPosition = servoPos;
  bool facingFront = strcmp(servoPos, "FRONT") == 0;
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

  // Capture the complete result before telemetry callbacks or cooperative waits
  // can advance SensorManager to a different sample.
  UltrasonicSample result = _sensors.latestUltrasonicSample();
  _safety.update(_sensors.snapshot());
  if (_publishCallback != nullptr) {
    _publishCallback(true);
  }
  return result;
}

bool NavigationManager::acquireFreshFrontDistance(UltrasonicSample& sample) {
  _motors.stop();
  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.center();
  if (responsiveDelay(120)) {
    _servoScanner.detach();
    return false;
  }

  unsigned long previousSequence = _sensors.distanceSampleSequence();
  sample = readAndPublishDistance("FRONT");
  if (sample.sequence <= previousSequence || sample.sampleMs == 0 ||
      millis() - sample.sampleMs > RobotConfig::ULTRASONIC_STALE_MS) {
    return false;
  }
  if (!sample.facingFront) {
    return false;
  }
  return (sample.state == UltrasonicResultState::DistanceValid && sample.valid) ||
         sample.state == UltrasonicResultState::ClearNoEcho;
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
      UltrasonicSample front = readAndPublishDistance("FRONT");

      if (abortIfOperatorRequested() || abortIfCritical()) {
        _motors.stop();
        return true;
      }
      if (front.sequence <= previousSequence || !front.facingFront ||
          front.sampleMs == 0 || millis() - front.sampleMs > RobotConfig::ULTRASONIC_STALE_MS ||
          front.state == UltrasonicResultState::SensorFault ||
          front.state == UltrasonicResultState::Stale) {
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
      if (front.state == UltrasonicResultState::DistanceValid && front.valid &&
          front.distanceCm <= RobotConfig::AUTO_OBSTACLE_CM) {
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

bool NavigationManager::executeAvoidanceTurn(bool turnLeft, bool nearRecovery) {
  const unsigned long turnStartedMs = millis();
  const ImuSnapshot& initial = _imu.snapshot();
  bool usingGyro = initial.initialized && initial.healthy &&
    isfinite(initial.gyroZ) && initial.lastSuccessMs != 0 &&
    turnStartedMs - initial.lastSuccessMs <= RobotConfig::IMU_STALE_MS;
  const float gyroBiasRadPerSecond = usingGyro ? initial.gyroZ : 0.0f;
  unsigned long lastIntegratedSampleMs = usingGyro ? initial.sampleMs : 0;
  float relativeTurnDeg = 0.0f;

  if (turnLeft) {
    _motors.leftTurn();
  } else {
    _motors.rightTurn();
  }

  if (!usingGyro) {
    Serial.println("[AUTO TURN] IMU unavailable at turn start; using timed fallback");
  }

  while (millis() - turnStartedMs < RobotConfig::AUTO_AVOIDANCE_TURN_TIMEOUT_MS) {
    if (_serviceCallback != nullptr) {
      _serviceCallback();
    }
    if (abortIfOperatorRequested() || abortIfCritical()) {
      _motors.stop();
      return true;
    }

    const SensorSnapshot& controlSensors = _sensors.snapshot();
    if (controlSensors.ultrasonicState == UltrasonicResultState::SensorFault ||
        controlSensors.ultrasonicState == UltrasonicResultState::Stale) {
      _motors.stop();
      setState(NavigationState::InvalidStop, "TURN_ULTRASONIC_INVALID_STOP");
      return true;
    }
    if (!nearRecovery && _safety.isNearObstacleHazardActive()) {
      _motors.stop();
      setState(NavigationState::NearObstacleHold, "TURN_NEAR_OBSTACLE_STOP");
      if (_warningCallback != nullptr) {
        _warningCallback();
      }
      return true;
    }

    const unsigned long now = millis();
    if (usingGyro) {
      const ImuSnapshot& sample = _imu.snapshot();
      const bool sampleUsable = sample.initialized && sample.healthy &&
        isfinite(sample.gyroZ) && sample.lastSuccessMs != 0 &&
        now - sample.lastSuccessMs <= RobotConfig::IMU_STALE_MS;
      if (!sampleUsable) {
        usingGyro = false;
        Serial.println("[AUTO TURN] IMU became invalid/stale; finishing with timed fallback");
      } else if (sample.sampleMs != lastIntegratedSampleMs) {
        const unsigned long sampleDeltaMs = sample.sampleMs - lastIntegratedSampleMs;
        lastIntegratedSampleMs = sample.sampleMs;
        if (sampleDeltaMs == 0 || sampleDeltaMs > RobotConfig::IMU_STALE_MS) {
          usingGyro = false;
          Serial.println("[AUTO TURN] IMU sample gap exceeded limit; finishing with timed fallback");
        } else {
          const float relativeRateDegPerSecond =
            fabsf((sample.gyroZ - gyroBiasRadPerSecond) * RAD_TO_DEG);
          if (relativeRateDegPerSecond >= RobotConfig::AUTO_AVOIDANCE_GYRO_DEADBAND_DPS) {
            relativeTurnDeg += relativeRateDegPerSecond *
              (static_cast<float>(sampleDeltaMs) / 1000.0f);
          }
          // Stop just beyond the nominal target by the configured tolerance.
          // The one-way completion latch prevents target-boundary chatter.
          if (relativeTurnDeg >= RobotConfig::AUTO_AVOIDANCE_TURN_TARGET_DEG +
              RobotConfig::AUTO_AVOIDANCE_TURN_TOLERANCE_DEG) {
            _motors.stop();
            setState(_state, turnLeft
              ? (nearRecovery ? "NEAR_LEFT_RELATIVE_TURN_COMPLETE" : "LEFT_RELATIVE_TURN_COMPLETE")
              : (nearRecovery ? "NEAR_RIGHT_RELATIVE_TURN_COMPLETE" : "RIGHT_RELATIVE_TURN_COMPLETE"));
            Serial.print("[AUTO TURN] Relative gyro-Z angle complete: ");
            Serial.print(relativeTurnDeg, 1);
            Serial.println(" deg");
            return false;
          }
        }
      }
    }

    if (!usingGyro &&
        now - turnStartedMs >= RobotConfig::AUTO_AVOIDANCE_TIMED_FALLBACK_MS) {
      _motors.stop();
      setState(_state, turnLeft
        ? (nearRecovery ? "NEAR_LEFT_TIMED_TURN_FALLBACK" : "LEFT_TIMED_TURN_FALLBACK")
        : (nearRecovery ? "NEAR_RIGHT_TIMED_TURN_FALLBACK" : "RIGHT_TIMED_TURN_FALLBACK"));
      return false;
    }
    delay(1);
  }

  _motors.stop();
  setState(NavigationState::InvalidStop,
           turnLeft ? "LEFT_TURN_TIMEOUT_STOP" : "RIGHT_TURN_TIMEOUT_STOP");
  Serial.print("[AUTO TURN] Hard timeout at relative angle ");
  Serial.print(relativeTurnDeg, 1);
  Serial.println(" deg");
  return false;
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
  NavigationScanSample left = scanAt(RobotConfig::LEFT_SCAN, "LEFT");
  if (abortIfOperatorRequested() || abortIfCritical()) return;
  _state = NavigationState::ScanFront;
  NavigationScanSample center = scanAt(RobotConfig::CENTER_SCAN, "FRONT");
  if (abortIfOperatorRequested() || abortIfCritical()) return;
  _state = NavigationState::ScanRight;
  NavigationScanSample right = scanAt(RobotConfig::RIGHT_SCAN, "RIGHT");

  if (abortIfOperatorRequested() || abortIfCritical()) {
    return;
  }

  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.center();
  if (responsiveDelay(400)) return;
  _servoScanner.detach();
  choosePathAndTurn(left, center, right, false);
  responsiveDelay(300);
}

bool NavigationManager::scanSampleTrusted(const NavigationScanSample& sample) const {
  if (!sample.completed || sample.ultrasonic.sampleMs == 0 ||
      millis() - sample.ultrasonic.sampleMs > RobotConfig::NAVIGATION_SCAN_SET_MAX_AGE_MS) {
    return false;
  }
  return (sample.ultrasonic.state == UltrasonicResultState::DistanceValid &&
          sample.ultrasonic.valid) ||
         sample.ultrasonic.state == UltrasonicResultState::ClearNoEcho;
}

float NavigationManager::scanPathDistance(const NavigationScanSample& sample) const {
  return sample.ultrasonic.state == UltrasonicResultState::ClearNoEcho
    ? RobotConfig::ULTRASONIC_MAX_CM
    : sample.ultrasonic.distanceCm;
}

void NavigationManager::choosePathAndTurn(const NavigationScanSample& leftSample,
                                          const NavigationScanSample& centerSample,
                                          const NavigationScanSample& rightSample,
                                          bool nearRecovery) {
  if (!scanSampleTrusted(leftSample) || !scanSampleTrusted(centerSample) ||
      !scanSampleTrusted(rightSample)) {
    Serial.println("Ultrasonic scan fault or stale sample. Holding position.");
    setState(NavigationState::InvalidStop, "ULTRASONIC_SCAN_FAULT_STOP");
    _motors.stop();
    return;
  }
  const float left = scanPathDistance(leftSample);
  const float center = scanPathDistance(centerSample);
  const float right = scanPathDistance(rightSample);
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
    if (executeAvoidanceTurn(true, nearRecovery)) return;
  } else {
    Serial.println("Decision: RIGHT");
    setState(nearRecovery ? NavigationState::NearObstacleTurn : NavigationState::TurnRight,
             nearRecovery ? "NEAR_TURN_RIGHT" : "RIGHT");
    if (executeAvoidanceTurn(false, nearRecovery)) return;
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
  NavigationScanSample left = scanAt(RobotConfig::LEFT_SCAN, "LEFT");
  if (abortIfOperatorRequested() || abortIfCritical()) return true;
  _state = NavigationState::NearObstacleScan;
  NavigationScanSample center = scanAt(RobotConfig::CENTER_SCAN, "FRONT");
  if (abortIfOperatorRequested() || abortIfCritical()) return true;
  _state = NavigationState::NearObstacleScan;
  NavigationScanSample right = scanAt(RobotConfig::RIGHT_SCAN, "RIGHT");
  if (abortIfOperatorRequested() || abortIfCritical()) {
    return true;
  }

  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.center();
  if (responsiveDelay(400)) return true;
  _servoScanner.detach();

  if (!scanSampleTrusted(left) || !scanSampleTrusted(center) || !scanSampleTrusted(right)) {
    setState(NavigationState::InvalidStop, "ULTRASONIC_SCAN_FAULT_STOP");
    _motors.stop();
    return true;
  }
  const bool leftSafe = scanPathDistance(left) > RobotConfig::AUTO_OBSTACLE_CM;
  const bool rightSafe = scanPathDistance(right) > RobotConfig::AUTO_OBSTACLE_CM;
  if (leftIrTriggered && !rightIrTriggered && rightSafe) {
    setState(NavigationState::NearObstacleTurn, "LEFT_IR_TURN_RIGHT");
    if (executeAvoidanceTurn(false, true)) return true;
  } else if (rightIrTriggered && !leftIrTriggered && leftSafe) {
    setState(NavigationState::NearObstacleTurn, "RIGHT_IR_TURN_LEFT");
    if (executeAvoidanceTurn(true, true)) return true;
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
