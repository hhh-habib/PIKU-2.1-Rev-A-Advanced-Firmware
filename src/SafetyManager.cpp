#include "SafetyManager.h"
#include "RobotConfig.h"

SafetyManager::SafetyManager()
  : _gasCritical(false),
    _gasCaution(false),
    _flameCritical(false),
    _leftIrObstacle(false),
    _rightIrObstacle(false),
    _tempCritical(false),
    _tempWarning(false),
    _ultrasonicCaution(false),
    _ultrasonicInvalid(true),
    _ultrasonicWarning(false),
    _ultrasonicFault(false),
    _ultrasonicStale(true),
    _nearObstacleHazard(false),
    _nearObstacleSource("NONE"),
    _lastCritical(false),
    _lastCaution(false),
    _lastNearObstacleHazard(false),
    _lastNearObstacleSource("NONE"),
    _lastReason("NONE"),
    _changed(true),
    _gasCriticalSince(0),
    _gasCriticalClearSince(0),
    _gasCautionSince(0),
    _gasCautionClearSince(0),
    _flameSince(0),
    _flameClearSince(0),
    _leftIrSince(0),
    _leftIrClearSince(0),
    _rightIrSince(0),
    _rightIrClearSince(0),
    _tempCriticalSince(0),
    _tempCriticalClearSince(0) {
}

void SafetyManager::begin() {
  _changed = true;
}

void SafetyManager::update(const SensorSnapshot& snapshot) {
  unsigned long now = millis();
  if (RobotConfig::HAS_MQ2) {
    updateGas(snapshot, now);
  } else {
    _gasCritical = false;
    _gasCaution = false;
  }
  if (RobotConfig::HAS_FLAME_SENSOR) {
    updateFlame(snapshot, now);
  } else {
    _flameCritical = false;
  }
  updateIr(snapshot.leftObstacleDetected, snapshot.rightObstacleDetected, now);
  if (RobotConfig::HAS_DHT22) {
    updateTemperature(snapshot.temperature, snapshot.temperatureValid, now);
  } else {
    _tempCritical = false;
    _tempWarning = false;
  }
  updateUltrasonic(snapshot);
  updateNearObstacle(snapshot);
  noteStateChange();
}

bool SafetyManager::isCritical() const {
  return _gasCritical || _flameCritical || _tempCritical;
}

bool SafetyManager::isAutoRecoverableEnvironmentalHazard() const {
  return _gasCritical || _flameCritical;
}

bool SafetyManager::hasNonRecoverableCriticalHazard() const {
  return _tempCritical;
}

bool SafetyManager::isCaution() const {
  return _gasCaution || _tempWarning || _ultrasonicWarning || _nearObstacleHazard;
}

bool SafetyManager::isIrObstacleActive() const {
  return _leftIrObstacle || _rightIrObstacle;
}

bool SafetyManager::isLeftIrObstacleActive() const {
  return _leftIrObstacle;
}

bool SafetyManager::isRightIrObstacleActive() const {
  return _rightIrObstacle;
}

bool SafetyManager::isNearObstacleHazardActive() const {
  return _nearObstacleHazard;
}

bool SafetyManager::isUltrasonicWarningActive() const {
  return _ultrasonicWarning;
}

bool SafetyManager::blocksMotion() const {
  return isCritical();
}

bool SafetyManager::blocksForwardMotion() const {
  return isCritical() || _nearObstacleHazard || _ultrasonicCaution || _ultrasonicWarning;
}

bool SafetyManager::blocksManualForwardMotion() const {
  // A single confirmed side IR informs avoidance but does not disable Manual forward.
  return isCritical() || _ultrasonicCaution || _ultrasonicWarning ||
         (_leftIrObstacle && _rightIrObstacle);
}

bool SafetyManager::stateChanged() {
  bool changed = _changed;
  _changed = false;
  return changed;
}

String SafetyManager::stateName() const {
  if (isCritical()) {
    return "CRITICAL";
  }
  if (isCaution()) {
    return "CAUTION";
  }
  return "SAFE";
}

String SafetyManager::alarmReason() const {
  if (_flameCritical) return "FLAME";
  if (_gasCritical) return "GAS_HIGH_RELATIVE";
  if (_tempCritical) return "TEMP_HIGH";
  if (_nearObstacleHazard) return "NEAR_OBSTACLE";
  if (_leftIrObstacle && _rightIrObstacle) return "BOTH_IR_OBSTACLES";
  if (_leftIrObstacle) return "LEFT_IR_OBSTACLE";
  if (_rightIrObstacle) return "RIGHT_IR_OBSTACLE";
  if (_ultrasonicFault) return "ULTRASONIC_SENSOR_FAULT";
  if (_ultrasonicStale) return "ULTRASONIC_STALE";
  if (_ultrasonicWarning) return "ULTRASONIC_WARNING";
  if (_gasCaution) return "GAS_ELEVATED_RELATIVE";
  if (_tempWarning) return "TEMP_WARN";
  return "NONE";
}

String SafetyManager::nearObstacleSource() const {
  return _nearObstacleSource;
}

String SafetyManager::obstacleDecision(const SensorSnapshot& snapshot) const {
  if (snapshot.distanceFacingFront &&
      snapshot.ultrasonicState == UltrasonicResultState::DistanceValid &&
      snapshot.distanceCm <= RobotConfig::OBSTACLE_CM) {
    return "ULTRASONIC_BLOCK";
  }
  if (_leftIrObstacle && _rightIrObstacle) return "BOTH_IR_BLOCK";
  if (_leftIrObstacle) return "LEFT_IR_BLOCK";
  if (_rightIrObstacle) return "RIGHT_IR_BLOCK";
  if (snapshot.distanceFacingFront &&
      snapshot.ultrasonicState != UltrasonicResultState::DistanceValid) {
    if (snapshot.ultrasonicState == UltrasonicResultState::ClearNoEcho) return "CLEAR_NO_ECHO";
    if (snapshot.ultrasonicState == UltrasonicResultState::SensorFault) return "ULTRASONIC_SENSOR_FAULT";
    return "ULTRASONIC_STALE";
  }
  return "CLEAR";
}
void SafetyManager::updateGas(const SensorSnapshot& snapshot, unsigned long now) {
  if (snapshot.mq2Warmup) {
    _gasCritical = false;
    _gasCaution = false;
    _gasCriticalSince = _gasCriticalClearSince = 0;
    _gasCautionSince = _gasCautionClearSince = 0;
    return;
  }

  if (!_gasCritical && snapshot.gasFiltered >= RobotConfig::GAS_HIGH_RELATIVE_THRESHOLD) {
    if (_gasCriticalSince == 0) _gasCriticalSince = now;
    if (now - _gasCriticalSince >= RobotConfig::GAS_DEBOUNCE_MS) _gasCritical = true;
  } else if (_gasCritical && snapshot.gasFiltered <= RobotConfig::GAS_HIGH_RELATIVE_CLEAR_THRESHOLD) {
    if (_gasCriticalClearSince == 0) _gasCriticalClearSince = now;
    if (now - _gasCriticalClearSince >= RobotConfig::GAS_CLEAR_DEBOUNCE_MS) _gasCritical = false;
  } else {
    _gasCriticalSince = 0;
    _gasCriticalClearSince = 0;
  }

  if (!_gasCaution && snapshot.gasFiltered >= RobotConfig::GAS_ELEVATED_RELATIVE_THRESHOLD) {
    if (_gasCautionSince == 0) _gasCautionSince = now;
    if (now - _gasCautionSince >= RobotConfig::GAS_DEBOUNCE_MS) _gasCaution = true;
  } else if (_gasCaution && snapshot.gasFiltered <= RobotConfig::GAS_ELEVATED_RELATIVE_CLEAR_THRESHOLD) {
    if (_gasCautionClearSince == 0) _gasCautionClearSince = now;
    if (now - _gasCautionClearSince >= RobotConfig::GAS_CLEAR_DEBOUNCE_MS) _gasCaution = false;
  } else {
    _gasCautionSince = 0;
    _gasCautionClearSince = 0;
  }
}

void SafetyManager::updateFlame(const SensorSnapshot& snapshot, unsigned long now) {
  const bool detected = snapshot.flameSampleValid && snapshot.flameDetected;
  if (detected) {
    _flameClearSince = 0;
    if (_flameSince == 0) _flameSince = now;
    if (now - _flameSince >= RobotConfig::FLAME_DEBOUNCE_MS) _flameCritical = true;
  } else {
    _flameSince = 0;
    if (_flameCritical) {
      if (_flameClearSince == 0) _flameClearSince = now;
      if (now - _flameClearSince >= RobotConfig::DIGITAL_CLEAR_DEBOUNCE_MS) _flameCritical = false;
    }
  }
}

void SafetyManager::updateIrChannel(bool detected,
                                    bool& active,
                                    unsigned long& activeSince,
                                    unsigned long& clearSince,
                                    unsigned long now) {
  if (detected) {
    clearSince = 0;
    if (activeSince == 0) activeSince = now;
    if (now - activeSince >= RobotConfig::IR_DEBOUNCE_MS) active = true;
  } else {
    activeSince = 0;
    if (active) {
      if (clearSince == 0) clearSince = now;
      if (now - clearSince >= RobotConfig::DIGITAL_CLEAR_DEBOUNCE_MS) active = false;
    }
  }
}

void SafetyManager::updateIr(bool leftDetected, bool rightDetected, unsigned long now) {
  // Both channels use the same 80 ms confirmation filter. Raw states remain
  // available in SensorSnapshot for diagnostics and dashboard telemetry.
  updateIrChannel(leftDetected, _leftIrObstacle, _leftIrSince, _leftIrClearSince, now);
  updateIrChannel(rightDetected, _rightIrObstacle, _rightIrSince, _rightIrClearSince, now);
}

void SafetyManager::updateTemperature(float temperature, bool valid, unsigned long now) {
  if (!valid || !isfinite(temperature)) {
    _tempWarning = false;
    _tempCriticalSince = 0;
    _tempCriticalClearSince = 0;
    return;
  }

  _tempWarning = temperature >= RobotConfig::TEMP_WARNING_C;
  if (!_tempCritical && temperature >= RobotConfig::TEMP_CRITICAL_C) {
    if (_tempCriticalSince == 0) _tempCriticalSince = now;
    if (now - _tempCriticalSince >= RobotConfig::TEMP_DEBOUNCE_MS) _tempCritical = true;
  } else if (_tempCritical && temperature <= RobotConfig::TEMP_CRITICAL_CLEAR_C) {
    if (_tempCriticalClearSince == 0) _tempCriticalClearSince = now;
    if (now - _tempCriticalClearSince >= RobotConfig::TEMP_DEBOUNCE_MS) _tempCritical = false;
  } else {
    _tempCriticalSince = 0;
    _tempCriticalClearSince = 0;
  }
}

void SafetyManager::updateUltrasonic(const SensorSnapshot& snapshot) {
  _ultrasonicCaution = false;
  _ultrasonicInvalid = false;
  _ultrasonicFault = false;
  _ultrasonicStale = false;

  switch (snapshot.ultrasonicState) {
    case UltrasonicResultState::DistanceValid:
      _ultrasonicCaution = snapshot.distanceFacingFront && snapshot.distanceCm <= RobotConfig::OBSTACLE_CM;
      _ultrasonicWarning = false;
      break;
    case UltrasonicResultState::ClearNoEcho:
      _ultrasonicWarning = false;
      break;
    case UltrasonicResultState::SensorFault:
      _ultrasonicInvalid = true;
      _ultrasonicFault = true;
      _ultrasonicWarning = snapshot.distanceFacingFront;
      break;
    case UltrasonicResultState::Stale:
      _ultrasonicInvalid = true;
      _ultrasonicStale = true;
      _ultrasonicWarning = snapshot.distanceFacingFront;
      break;
  }
}

void SafetyManager::updateNearObstacle(const SensorSnapshot& snapshot) {
  bool leftIrNear = _leftIrObstacle;
  bool rightIrNear = _rightIrObstacle;
  bool irNear = leftIrNear || rightIrNear;
  bool ultrasonicNear = snapshot.distanceFacingFront && snapshot.distanceValid && snapshot.distanceCm <= RobotConfig::NEAR_OBSTACLE_CM;
  _nearObstacleHazard = irNear || ultrasonicNear;

  if (ultrasonicNear && leftIrNear && rightIrNear) {
    _nearObstacleSource = "ULTRASONIC+BOTH_IR";
  } else if (ultrasonicNear && leftIrNear) {
    _nearObstacleSource = "ULTRASONIC+LEFT_IR";
  } else if (ultrasonicNear && rightIrNear) {
    _nearObstacleSource = "ULTRASONIC+RIGHT_IR";
  } else if (leftIrNear && rightIrNear) {
    _nearObstacleSource = "BOTH_IR";
  } else if (leftIrNear) {
    _nearObstacleSource = "LEFT_IR";
  } else if (rightIrNear) {
    _nearObstacleSource = "RIGHT_IR";
  } else if (ultrasonicNear) {
    _nearObstacleSource = "ULTRASONIC";
  } else {
    _nearObstacleSource = "NONE";
  }
}

void SafetyManager::noteStateChange() {
  bool critical = isCritical();
  bool caution = isCaution();
  String reason = alarmReason();
  if (critical != _lastCritical ||
      caution != _lastCaution ||
      _nearObstacleHazard != _lastNearObstacleHazard ||
      _nearObstacleSource != _lastNearObstacleSource ||
      reason != _lastReason) {
    _changed = true;
    _lastCritical = critical;
    _lastCaution = caution;
    _lastNearObstacleHazard = _nearObstacleHazard;
    _lastNearObstacleSource = _nearObstacleSource;
    _lastReason = reason;
  }
}
