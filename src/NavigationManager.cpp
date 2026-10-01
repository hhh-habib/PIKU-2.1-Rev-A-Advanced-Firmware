#include "NavigationManager.h"

#include <math.h>
#include <string.h>

static_assert(RobotConfig::AUTO_SCAN_SAMPLES_PER_DIRECTION >= 3,
              "AUTO scan aggregation requires at least three readings");

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
  : _motors(motors), _sensors(sensors), _imu(imu), _servoScanner(servoScanner),
    _safety(safety), _dashboard(dashboard), _dashboardData(dashboardData),
    _currentServoPosition(currentServoPosition), _serviceCallback(serviceCallback),
    _publishCallback(publishCallback), _warningCallback(warningCallback),
    _state(NavigationState::Idle), _phase(AutoPhase::Idle),
    _scanPurpose(ScanPurpose::Obstacle), _phaseStartedMs(0), _lastUpdateMs(0),
    _sampleTargetSequence(0), _sampleRequestedMs(0), _lastGuardSequence(0),
    _scanDirectionIndex(0), _scanReadingIndex(0), _scanAccumulator(),
    _leftScan(), _frontScan(), _rightScan(), _turnLeft(true),
    _turnUsingGyro(false), _turnGyroBiasRadPerSecond(0.0f),
    _relativeTurnDeg(0.0f), _turnStartedMs(0), _lastIntegratedSampleMs(0),
    _lastAvoidanceTurnLeft(false), _irEscapeSourceZone(IrZone::Clear),
    _obstacleRecoveryActive(false), _obstacleReevaluationPending(false),
    _obstacleRecoveryAttempts(0), _blockedRetryCount(0),
    _obstacleForwardClearMs(0), _operatorStopLatched(false),
    _operatorStopModeCommandCounter(0), _environmentalRecoveryActive(false),
    _environmentalRecoveryCompleted(false), _hazardClearStartedMs(0),
    _stuckDistanceSamples(), _stuckDistanceCount(0), _stuckDistanceIndex(0),
    _stuckForwardActiveMs(0), _stuckImuStillMs(0),
    _stuckRecoveryForwardMs(0), _stuckRecoveryAttempts(0),
    _stuckRecoveryTrialActive(false), _lastStuckReason("NONE") {
  _dashboardData.autoTurnTargetDeg = RobotConfig::AUTO_AVOIDANCE_TURN_TARGET_DEG;
}

void NavigationManager::updateAuto() {
  const unsigned long now = millis();
  unsigned long elapsedMs = _lastUpdateMs == 0 ? 0 : now - _lastUpdateMs;
  if (elapsedMs > 100) elapsedMs = 100;
  _lastUpdateMs = now;

  if (handleOperatorInterruption()) {
    updateDiagnostics(now);
    return;
  }
  if (handleCriticalSafety(now)) {
    if (RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
        _v2.state() != StuckV2State::Normal) {
      _v2.abort(); // preserve ordinary AUTO hazard-clear/rescan behavior
      _recoverySideRechecks = 0;
      _recoveryIrBackoff.resetMission();
    }
    updateDiagnostics(now);
    return;
  }
  if (_phase == AutoPhase::SonarReacquire) {
    updateSonarReacquire(now);
    updateDiagnostics(now);
    return;
  }
  const UltrasonicResultState sonarState = _sensors.snapshot().ultrasonicState;
  if (sonarState == UltrasonicResultState::SensorFault ||
      sonarState == UltrasonicResultState::Stale) {
    beginSonarReacquire(now);
    updateDiagnostics(now);
    return;
  }
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      _v2.state() == StuckV2State::TrappedHold) {
    updateV2RetryCooldown(now);
    updateDiagnostics(now);
    return;
  }
  const bool v2Active = RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      _v2.state() != StuckV2State::Normal &&
      _v2.state() != StuckV2State::Suspected;
  if (v2Active) {
    if (_v2.escapeTimedOut(now, RobotConfig::RECOVERY_MISSION_TIMEOUT_MS)) {
      enterV2TrappedHold("ESCAPE_TOTAL_TIMEOUT", now);
      updateDiagnostics(now);
      return;
    }
    // Either low IR zone is a collision veto, never a new side preference.
    if (_v2.hasCommittedSide() && currentIrZone() != IrZone::Clear &&
        _phase != AutoPhase::RecoveryIrBackoff) {
      _motors.stop();
      _dashboardData.autoTurnCompletionReason = "ESCAPE_IR_INTERRUPT";
      beginRecoveryIrBackoff(now);
      updateDiagnostics(now);
      return;
    }
  } else if (handleIrArbitration(now)) {
    updateDiagnostics(now);
    return;
  }

  switch (_phase) {
    case AutoPhase::Idle:
      beginFrontValidation(now, "SENSE_FRONT");
      break;
    case AutoPhase::FrontSettle:
    case AutoPhase::FrontSampleWait:
      updateFrontValidation(now);
      break;
    case AutoPhase::Forward:
      updateForward(now, elapsedMs);
      break;
    case AutoPhase::ForwardSettle:
      if (now - _phaseStartedMs >= RobotConfig::AUTO_STOP_SETTLE_MS)
        beginFrontValidation(now, "SENSE_FRONT");
      break;
    case AutoPhase::ReversePrepare:
    case AutoPhase::Reverse:
    case AutoPhase::ReverseSettle:
      updateReverseSequence(now);
      break;
    case AutoPhase::ScanServoSettle:
    case AutoPhase::ScanSampleGap:
    case AutoPhase::ScanSampleWait:
      updateScan(now);
      break;
    case AutoPhase::RecenterSettle:
      if (now - _phaseStartedMs >= RobotConfig::AUTO_SCAN_RECENTER_SETTLE_MS) {
        _servoScanner.detach();
        if (_scanPurpose == ScanPurpose::RecoverySide) finishRecoverySideCheck(now);
        else selectPathAfterScan(now);
      }
      break;
    case AutoPhase::Turn:
      updateSpinTurn(now);
      break;
    case AutoPhase::PostTurnSettle:
      if (now - _phaseStartedMs >= RobotConfig::AUTO_POST_TURN_SETTLE_MS)
        finishRecoveryAfterSettle(now);
      break;
    case AutoPhase::HazardActive:
    case AutoPhase::HazardClearWait:
      updateEnvironmentalRecovery(now);
      break;
    case AutoPhase::InvalidStop:
      _motors.stop();
      if (now - _phaseStartedMs >= RobotConfig::AUTO_INVALID_RETRY_MS &&
          !_safety.isCritical() &&
          _sensors.snapshot().ultrasonicState != UltrasonicResultState::SensorFault) {
        if (_environmentalRecoveryActive) {
          beginScan(ScanPurpose::EnvironmentalRecovery, now);
        } else if (_obstacleRecoveryActive) {
          enterBlockedRetryWait("SENSOR_RECOVERED_RETRY_WAIT", now);
        } else {
          beginFrontValidation(now, "INVALID_RETRY_REVALIDATE");
        }
      }
      break;
    case AutoPhase::BlockedRetryWait:
      updateBlockedRetryWait(now);
      break;
    case AutoPhase::RecoveryFrontSettle:
    case AutoPhase::RecoveryFrontWait:
      updateRecoveryFrontCheck(now);
      break;
    case AutoPhase::RecoveryIrBackoff:
      updateRecoveryIrBackoff(now);
      break;
    case AutoPhase::OperatorStop:
    case AutoPhase::TrappedHold:
      _motors.stop();
      break;
  }
  updateDiagnostics(now);
}

void NavigationManager::resetRecoveryLatchIfClear() {
  if (currentIrZone() == IrZone::Clear) {
    _irEscapeSourceZone = IrZone::Clear;
    _obstacleRecoveryActive = false;
    _obstacleReevaluationPending = false;
    _obstacleRecoveryAttempts = 0;
    _blockedRetryCount = 0;
    _obstacleForwardClearMs = 0;
  }
}

void NavigationManager::notifyOperatorStop() {
  _operatorStopLatched = true;
  _operatorStopModeCommandCounter = _dashboard.modeCommandCounter();
  _environmentalRecoveryActive = false;
  _environmentalRecoveryCompleted = false;
  _hazardClearStartedMs = 0;
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2) _v2.abort();
  _recoverySideRechecks = 0;
  _recoveryIrBackoff.resetMission();
  _obstacleRecoveryActive = false;
  _obstacleReevaluationPending = false;
  _motors.stop();
  _servoScanner.detach();
  _dashboardData.autoTurnCompletionReason = "OPERATOR_STOP";
  setState(NavigationState::OperatorAbort, "AUTO_OPERATOR_STOP");
  setPhase(AutoPhase::OperatorStop, millis());
}

void NavigationManager::cancelForModeChange() {
  _motors.stop();
  _servoScanner.detach();
  _environmentalRecoveryActive = false;
  _environmentalRecoveryCompleted = false;
  _hazardClearStartedMs = 0;
  _operatorStopLatched = false;
  _irEscapeSourceZone = IrZone::Clear;
  _obstacleRecoveryActive = false;
  _obstacleReevaluationPending = false;
  _obstacleRecoveryAttempts = 0;
  _blockedRetryCount = 0;
  _obstacleForwardClearMs = 0;
  _scanPurpose = ScanPurpose::Obstacle;
  resetStuckEvidence(true);
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2) _v2.rearm();
  _recoverySideRechecks = 0;
  _recoveryIrBackoff.resetMission();
  _v2LastReason = "NONE";
  setState(NavigationState::OperatorAbort, "AUTO_MODE_CHANGE");
  setPhase(AutoPhase::Idle, millis());
}

bool NavigationManager::environmentalRecoveryInProgress() const {
  return _environmentalRecoveryActive;
}

bool NavigationManager::consumeEnvironmentalRecoveryCompleted() {
  const bool completed = _environmentalRecoveryCompleted;
  _environmentalRecoveryCompleted = false;
  return completed;
}

NavigationState NavigationManager::state() const { return _state; }

const char* NavigationManager::stateName() const {
  switch (_state) {
    case NavigationState::Idle: return "Idle";
    case NavigationState::FrontSettle: return "FrontSettle";
    case NavigationState::FrontSample: return "FrontSample";
    case NavigationState::Forward: return "Forward";
    case NavigationState::ObstacleReverse: return "ObstacleReverse";
    case NavigationState::ScanLeft: return "ScanLeft";
    case NavigationState::ScanFront: return "ScanFront";
    case NavigationState::ScanRight: return "ScanRight";
    case NavigationState::Recenter: return "Recenter";
    case NavigationState::SpinLeft: return "SpinLeft";
    case NavigationState::SpinRight: return "SpinRight";
    case NavigationState::InvalidStop: return "InvalidStop";
    case NavigationState::SonarReacquire: return "SonarReacquire";
    case NavigationState::ObstacleRecovery: return "ObstacleRecovery";
    case NavigationState::ObstacleScan: return "ObstacleScan";
    case NavigationState::ObstacleSpin: return "ObstacleSpin";
    case NavigationState::ObstacleReevaluate: return "ObstacleReevaluate";
    case NavigationState::BlockedRetryWait: return "BlockedRetryWait";
    case NavigationState::HazardActive: return "HazardActive";
    case NavigationState::HazardClearWait: return "HazardClearWait";
    case NavigationState::HazardRecoveryScan: return "HazardRecoveryScan";
    case NavigationState::StuckSuspected: return "StuckSuspected";
    case NavigationState::StuckRecovery: return "StuckRecovery";
    case NavigationState::RecoveryIrBackoff: return "RecoveryIrBackoff";
    case NavigationState::OperatorAbort: return "OperatorAbort";
    case NavigationState::CriticalAbort: return "CriticalAbort";
    case NavigationState::TrappedHold: return "TrappedHold";
  }
  return "Unknown";
}

void NavigationManager::setState(NavigationState state, const String& decision) {
  _state = state;
  _dashboardData.navigationDecision = decision;
  _dashboard.setNavigationDecision(decision);
}

void NavigationManager::setPhase(AutoPhase phase, unsigned long now) {
  _phase = phase;
  _phaseStartedMs = now;
}

bool NavigationManager::handleOperatorInterruption() {
  const unsigned long modeCounter = _dashboard.modeCommandCounter();
  if (_operatorStopLatched && modeCounter != _operatorStopModeCommandCounter) {
    _operatorStopLatched = false;
    resetStuckEvidence(true);
    if (RobotConfig::ENABLE_STUCK_RECOVERY_V2) _v2.rearm();
    setPhase(AutoPhase::Idle, millis());
  }
  if (_dashboard.getPendingCommand() == "STOP") {
    _dashboard.setPendingCommand("");
    notifyOperatorStop();
    return true;
  }
  if (_dashboard.getControlMode() != "AUTO") {
    cancelForModeChange();
    return true;
  }
  if (_operatorStopLatched) {
    _motors.stop();
    setState(NavigationState::OperatorAbort, "AUTO_OPERATOR_STOP");
    return true;
  }
  return false;
}

bool NavigationManager::handleCriticalSafety(unsigned long now) {
  if (_safety.hasNonRecoverableCriticalHazard()) {
    _environmentalRecoveryActive = false;
    _hazardClearStartedMs = 0;
    _motors.stop();
    _servoScanner.detach();
    _dashboardData.autoTurnCompletionReason = "SAFETY_INTERRUPT";
    setState(NavigationState::CriticalAbort, "NON_RECOVERABLE_SAFETY_STOP");
    return true;
  }
  if (_safety.isAutoRecoverableEnvironmentalHazard()) {
    enterEnvironmentalHazard(now);
    return true;
  }
  if (_environmentalRecoveryActive) {
    if (_phase == AutoPhase::HazardActive || _phase == AutoPhase::HazardClearWait) {
      updateEnvironmentalRecovery(now);
      return true;
    }
    return false;
  }
  if (_safety.isCritical()) {
    _motors.stop();
    _servoScanner.detach();
    _dashboardData.autoTurnCompletionReason = "SAFETY_INTERRUPT";
    setState(NavigationState::CriticalAbort, "SAFETY_STOP");
    return true;
  }
  return false;
}

NavigationManager::IrZone NavigationManager::currentIrZone() const {
  const bool left = _safety.isLeftIrObstacleActive();
  const bool right = _safety.isRightIrObstacleActive();
  if (left && right) return IrZone::Both;
  if (left) return IrZone::Left;
  if (right) return IrZone::Right;
  return IrZone::Clear;
}

const char* NavigationManager::irZoneName(IrZone zone) const {
  switch (zone) {
    case IrZone::Left: return "LEFT";
    case IrZone::Right: return "RIGHT";
    case IrZone::Both: return "BOTH";
    case IrZone::Clear: return "CLEAR";
  }
  return "CLEAR";
}

bool NavigationManager::handleIrArbitration(unsigned long now) {
  const IrZone zone = currentIrZone();
  if (zone == IrZone::Clear) return false;

  // Known IR evidence owns navigation before ultrasonic and blind-zone stuck logic.
  resetStuckEvidence(false);
  if (_phase == AutoPhase::BlockedRetryWait) {
    _irEscapeSourceZone = zone;
    _scanPurpose = zone == IrZone::Both ? ScanPurpose::IrBoth : ScanPurpose::IrSingle;
    _dashboardData.autoAvoidanceSource = zone == IrZone::Both
      ? "IR_BOTH" : (zone == IrZone::Left ? "IR_LEFT" : "IR_RIGHT");
    _dashboardData.autoPreferredPath = zone == IrZone::Left
      ? "RIGHT" : (zone == IrZone::Right ? "LEFT" : "NONE");
    return false;
  }
  if (_obstacleRecoveryActive && _scanPurpose == ScanPurpose::IrBoth) {
    return false;
  }
  if (_obstacleRecoveryActive && _scanPurpose == ScanPurpose::IrSingle) {
    if (zone == _irEscapeSourceZone) {
      return false;
    }
    _motors.stop();
    _dashboardData.autoTurnCompletionReason = "OPPOSITE_IR_INTERRUPT";
    _obstacleRecoveryActive = false;
    beginIrRecovery(zone, now);
    return true;
  }
  if (_obstacleRecoveryActive) {
    _motors.stop();
    _dashboardData.autoTurnCompletionReason = "IR_PRIORITY_INTERRUPT";
    _obstacleRecoveryActive = false;
  }

  beginIrRecovery(zone, now);
  return true;
}

void NavigationManager::beginIrRecovery(IrZone zone, unsigned long now) {
  if (_warningCallback != nullptr) _warningCallback();
  beginUnifiedObstacleRecovery(zone == IrZone::Both
                                 ? ScanPurpose::IrBoth : ScanPurpose::IrSingle,
                               zone,
                               now);
}

void NavigationManager::beginUnifiedObstacleRecovery(ScanPurpose purpose,
                                                       IrZone irSource,
                                                       unsigned long now) {
  _motors.stop();
  _servoScanner.detach();
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2)
    _v2PriorForwardMs = _obstacleForwardClearMs;
  resetStuckEvidence(purpose != ScanPurpose::StuckRecovery);
  _obstacleForwardClearMs = 0;
  _scanPurpose = purpose;
  _irEscapeSourceZone = irSource;
  _obstacleRecoveryActive = true;
  _obstacleReevaluationPending = false;
  _dashboardData.autoSelectedPath = "NONE";

  if (purpose == ScanPurpose::IrSingle) {
    _dashboardData.autoAvoidanceSource = irSource == IrZone::Left ? "IR_LEFT" : "IR_RIGHT";
    _dashboardData.autoPreferredPath = irSource == IrZone::Left ? "RIGHT" : "LEFT";
  } else if (purpose == ScanPurpose::IrBoth) {
    _dashboardData.autoAvoidanceSource = "IR_BOTH";
    _dashboardData.autoPreferredPath = "NONE";
  } else if (purpose == ScanPurpose::StuckRecovery) {
    _dashboardData.autoAvoidanceSource = "STUCK";
    _dashboardData.autoPreferredPath = "NONE";
  } else {
    _dashboardData.autoAvoidanceSource = "ULTRASONIC";
    _dashboardData.autoPreferredPath = "NONE";
  }

  if (_obstacleRecoveryAttempts >= RobotConfig::AUTO_OBSTACLE_ATTEMPTS_BEFORE_RETRY_WAIT) {
    enterBlockedRetryWait("OBSTACLE_RETRY_BACKOFF", now);
    return;
  }

  _obstacleRecoveryAttempts++;
  setState(NavigationState::ObstacleRecovery, "OBSTACLE_REVERSE_PREPARE");
  setPhase(AutoPhase::ReversePrepare, now);
}

void NavigationManager::enterBlockedRetryWait(const char* reason, unsigned long now) {
  _motors.stop();
  _servoScanner.detach();
  _obstacleRecoveryActive = true;
  _blockedRetryCount++;
  _dashboardData.autoTurnCompletionReason = "BLOCKED_RETRY";
  setState(NavigationState::BlockedRetryWait, reason);
  setPhase(AutoPhase::BlockedRetryWait, now);
}

void NavigationManager::updateBlockedRetryWait(unsigned long now) {
  _motors.stop();
  if (now - _phaseStartedMs < RobotConfig::AUTO_BLOCKED_RETRY_WAIT_MS) return;
  _obstacleRecoveryAttempts = 0;
  _stuckRecoveryAttempts = 0;
  // V2 history and lifetime attempt budget survive this normal backoff.
  setState(NavigationState::ObstacleScan, "BLOCKED_RETRY_RESCAN");
  beginScan(_scanPurpose, now);
}

void NavigationManager::beginFrontValidation(unsigned long now, const char* decision) {
  _motors.stop();
  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.center();
  setState(NavigationState::FrontSettle, decision);
  setPhase(AutoPhase::FrontSettle, now);
}

void NavigationManager::updateFrontValidation(unsigned long now) {
  if (_phase == AutoPhase::FrontSettle) {
    if (now - _phaseStartedMs < RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS ||
        _sensors.isDistanceSampleInProgress()) return;
    _sampleTargetSequence = _sensors.requestDistanceSample(true);
    _sampleRequestedMs = now;
    setState(NavigationState::FrontSample, "FRONT_SAMPLE_WAIT");
    setPhase(AutoPhase::FrontSampleWait, now);
    return;
  }
  if (_sensors.distanceSampleSequence() < _sampleTargetSequence) {
    if (now - _sampleRequestedMs > RobotConfig::ULTRASONIC_SAMPLE_TIMEOUT_MS + 20UL)
      enterInvalidStop("FRONT_SAMPLE_TIMEOUT_STOP");
    return;
  }
  const UltrasonicSample front = _sensors.latestUltrasonicSample();
  const bool fresh = front.sequence >= _sampleTargetSequence && front.sampleMs != 0 &&
    now - front.sampleMs <= RobotConfig::ULTRASONIC_STALE_MS;
  const bool trusted = fresh && front.facingFront &&
    ((front.state == UltrasonicResultState::DistanceValid && front.valid) ||
     front.state == UltrasonicResultState::ClearNoEcho);
  if (!trusted) {
    enterInvalidStop("FRONT_ULTRASONIC_INVALID_STOP");
    return;
  }
  const IrZone zone = currentIrZone();
  if (zone != IrZone::Clear) {
    _obstacleReevaluationPending = false;
    _obstacleRecoveryActive = false;
    beginIrRecovery(zone, now);
    return;
  }

  const bool completedObstacleReevaluation = _obstacleReevaluationPending;
  _obstacleReevaluationPending = false;
  _irEscapeSourceZone = IrZone::Clear;
  _lastGuardSequence = front.sequence;
  recordStuckDistance(front);
  if (_environmentalRecoveryActive && completedObstacleReevaluation) {
    _obstacleRecoveryActive = false;
    beginScan(ScanPurpose::EnvironmentalRecovery, now);
  } else if (front.state == UltrasonicResultState::DistanceValid &&
             front.distanceCm <= RobotConfig::AUTO_OBSTACLE_CM) {
    _obstacleRecoveryActive = false;
    beginObstacleRecovery(now);
  } else {
    _obstacleRecoveryActive = false;
    startForward(now, "FORWARD_STEP");
  }
}

void NavigationManager::startForward(unsigned long now, const char* decision) {
  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.detach();
  _dashboardData.autoAvoidanceSource = "NONE";
  _dashboardData.autoPreferredPath = "NONE";
  setState(NavigationState::Forward, decision);
  _motors.forward();
  setPhase(AutoPhase::Forward, now);
}

void NavigationManager::updateForward(unsigned long now, unsigned long elapsedMs) {
  _obstacleForwardClearMs += elapsedMs;
  if (_obstacleForwardClearMs >= RobotConfig::AUTO_OBSTACLE_RETRY_RESET_FORWARD_MS) {
    if (RobotConfig::ENABLE_STUCK_RECOVERY_V2)
      _v2.noteSustainedClear(_obstacleForwardClearMs);
    _obstacleRecoveryAttempts = 0;
    _blockedRetryCount = 0;
    _obstacleForwardClearMs = 0;
  }
  updateStuckEvidence(now, elapsedMs);
  if (!RobotConfig::ENABLE_STUCK_RECOVERY_V2 && stuckEvidenceSatisfied()) {
    beginStuckRecovery(now);
    return;
  }

  const UltrasonicSample front = _sensors.latestUltrasonicSample();
  if (front.sequence != _lastGuardSequence) {
    _lastGuardSequence = front.sequence;
    if (!front.facingFront || front.sampleMs == 0 ||
        now - front.sampleMs > RobotConfig::ULTRASONIC_STALE_MS ||
        front.state == UltrasonicResultState::SensorFault ||
        front.state == UltrasonicResultState::Stale) {
      _motors.stop();
      enterInvalidStop("FORWARD_ULTRASONIC_INVALID_STOP");
      return;
    }
    recordStuckDistance(front);
    if (front.state == UltrasonicResultState::DistanceValid && front.valid &&
        front.distanceCm <= RobotConfig::AUTO_OBSTACLE_CM) {
      _motors.stop();
      beginObstacleRecovery(now);
      return;
    }
  }
  if (now - _phaseStartedMs >= RobotConfig::AUTO_FORWARD_STEP_MS) {
    _motors.stop();
    setState(NavigationState::FrontSettle, "STOP_SETTLE");
    setPhase(AutoPhase::ForwardSettle, now);
  }
}

void NavigationManager::beginObstacleRecovery(unsigned long now) {
  beginUnifiedObstacleRecovery(ScanPurpose::Obstacle, IrZone::Clear, now);
}

void NavigationManager::updateReverseSequence(unsigned long now) {
  if (_phase == AutoPhase::ReversePrepare) {
    if (now - _phaseStartedMs < RobotConfig::AUTO_REVERSE_PREPARE_MS) return;
    if (_scanPurpose == ScanPurpose::StuckRecovery)
      setState(NavigationState::StuckRecovery, "STUCK_RECOVERY_REVERSE");
    else setState(NavigationState::ObstacleReverse, "OBSTACLE_RECOVERY_REVERSE");
    _motors.backward();
    setPhase(AutoPhase::Reverse, now);
    return;
  }
  if (_phase == AutoPhase::Reverse) {
    if (now - _phaseStartedMs < RobotConfig::AUTO_OBSTACLE_REVERSE_MS) return;
    _motors.stop();
    setPhase(AutoPhase::ReverseSettle, now);
    return;
  }
  if (now - _phaseStartedMs >= RobotConfig::AUTO_REVERSE_SETTLE_MS)
    beginScan(_scanPurpose, now);
}

void NavigationManager::beginScan(ScanPurpose purpose, unsigned long now) {
  _scanPurpose = purpose;
  _motors.stop();
  if (purpose == ScanPurpose::EnvironmentalRecovery)
    _dashboardData.autoAvoidanceSource = "HAZARD";
  else if (purpose == ScanPurpose::StuckRecovery)
    _dashboardData.autoAvoidanceSource = "STUCK";
  else if (purpose == ScanPurpose::EscapeV2)
    _dashboardData.autoAvoidanceSource = "STUCK_V2";
  else if (purpose == ScanPurpose::IrBoth)
    _dashboardData.autoAvoidanceSource = "IR_BOTH";
  else if (purpose == ScanPurpose::Obstacle)
    _dashboardData.autoAvoidanceSource = "ULTRASONIC";
  _scanDirectionIndex = 0;
  _scanReadingIndex = 0;
  _scanAccumulator = ScanAccumulator();
  _leftScan = NavigationScanSample();
  _frontScan = NavigationScanSample();
  _rightScan = NavigationScanSample();
  _dashboardData.autoSelectedPath = "NONE";
  commandCurrentScanDirection(now);
}

void NavigationManager::commandCurrentScanDirection(unsigned long now) {
  _scanAccumulator = ScanAccumulator();
  _scanReadingIndex = 0;
  NavigationState scanState = NavigationState::ScanLeft;
  if (_scanPurpose == ScanPurpose::EnvironmentalRecovery)
    scanState = NavigationState::HazardRecoveryScan;
  else if (_scanPurpose == ScanPurpose::StuckRecovery)
    scanState = NavigationState::StuckRecovery;
  else if (_scanPurpose == ScanPurpose::EscapeV2)
    scanState = NavigationState::StuckSuspected;
  else if (_scanPurpose == ScanPurpose::RecoverySide)
    scanState = NavigationState::StuckRecovery;
  else if (_scanPurpose == ScanPurpose::IrSingle ||
           _scanPurpose == ScanPurpose::IrBoth)
    scanState = NavigationState::ObstacleScan;

  if (_scanDirectionIndex == 0) {
    _currentServoPosition = "LEFT";
    _sensors.setDistanceFacingFront(false);
    _servoScanner.lookLeft();
    setState(scanState, "SCAN_LEFT_SETTLE");
  } else if (_scanDirectionIndex == 1) {
    _currentServoPosition = "FRONT";
    _sensors.setDistanceFacingFront(true);
    _servoScanner.center();
    if (_scanPurpose == ScanPurpose::Obstacle) scanState = NavigationState::ScanFront;
    setState(scanState, "SCAN_FRONT_SETTLE");
  } else {
    _currentServoPosition = "RIGHT";
    _sensors.setDistanceFacingFront(false);
    _servoScanner.lookRight();
    if (_scanPurpose == ScanPurpose::Obstacle) scanState = NavigationState::ScanRight;
    setState(scanState, "SCAN_RIGHT_SETTLE");
  }
  setPhase(AutoPhase::ScanServoSettle, now);
}

void NavigationManager::updateScan(unsigned long now) {
  if (_phase == AutoPhase::ScanServoSettle) {
    if (now - _phaseStartedMs < RobotConfig::AUTO_SCAN_SERVO_SETTLE_MS) return;
    setPhase(AutoPhase::ScanSampleGap, now);
  }
  if (_phase == AutoPhase::ScanSampleGap) {
    if (now - _phaseStartedMs < RobotConfig::AUTO_SCAN_SAMPLE_GAP_MS ||
        _sensors.isDistanceSampleInProgress()) return;
    const bool facingFront = _scanDirectionIndex == 1;
    _sampleTargetSequence = _sensors.requestDistanceSample(facingFront);
    _sampleRequestedMs = now;
    setPhase(AutoPhase::ScanSampleWait, now);
    return;
  }
  if (_phase != AutoPhase::ScanSampleWait) return;

  UltrasonicSample sample;
  bool finished = false;
  if (_sensors.distanceSampleSequence() >= _sampleTargetSequence) {
    sample = _sensors.latestUltrasonicSample();
    const bool expectedFront = _scanDirectionIndex == 1;
    if (sample.sequence < _sampleTargetSequence || sample.sampleMs == 0 ||
        now - sample.sampleMs > RobotConfig::ULTRASONIC_STALE_MS ||
        sample.facingFront != expectedFront) {
      sample = UltrasonicSample();
      sample.state = UltrasonicResultState::Stale;
    }
    finished = true;
  } else if (now - _sampleRequestedMs > RobotConfig::ULTRASONIC_SAMPLE_TIMEOUT_MS + 20UL) {
    sample.state = UltrasonicResultState::Stale;
    finished = true;
  }
  if (!finished) return;

  recordScanReading(sample);
  _scanReadingIndex++;
  if (_scanReadingIndex < RobotConfig::AUTO_SCAN_SAMPLES_PER_DIRECTION) {
    setPhase(AutoPhase::ScanSampleGap, now);
    return;
  }
  storeCurrentScan(finalizeCurrentScan());
  if (_scanPurpose == ScanPurpose::RecoverySide) {
    beginRecenter(now);
    return;
  }
  _scanDirectionIndex++;
  if (_scanDirectionIndex < 3) commandCurrentScanDirection(now);
  else beginRecenter(now);
}

void NavigationManager::recordScanReading(const UltrasonicSample& sample) {
  if (sample.sampleMs > _scanAccumulator.newestSampleMs)
    _scanAccumulator.newestSampleMs = sample.sampleMs;
  if (sample.state == UltrasonicResultState::SensorFault) {
    _scanAccumulator.sensorFaultCount++;
  } else if (sample.state == UltrasonicResultState::ClearNoEcho) {
    _scanAccumulator.noEchoCount++;
    _scanAccumulator.trustedCount++;
  } else if (sample.state == UltrasonicResultState::DistanceValid && sample.valid &&
             isfinite(sample.distanceCm) && sample.distanceCm > 0.0f) {
    if (_scanAccumulator.distanceCount < RobotConfig::AUTO_SCAN_SAMPLES_PER_DIRECTION)
      _scanAccumulator.distances[_scanAccumulator.distanceCount++] = sample.distanceCm;
    _scanAccumulator.trustedCount++;
  }
}

NavigationManager::NavigationScanSample NavigationManager::finalizeCurrentScan() const {
  NavigationScanSample result;
  result.angle = _scanDirectionIndex == 0 ? RobotConfig::LEFT_SCAN
    : (_scanDirectionIndex == 1 ? RobotConfig::CENTER_SCAN : RobotConfig::RIGHT_SCAN);
  result.direction = _scanDirectionIndex == 0 ? "LEFT"
    : (_scanDirectionIndex == 1 ? "FRONT" : "RIGHT");
  result.ultrasonic.facingFront = _scanDirectionIndex == 1;
  result.ultrasonic.sampleMs = _scanAccumulator.newestSampleMs;
  result.ultrasonic.sequence = _sensors.distanceSampleSequence();
  result.trustedReadings = _scanAccumulator.trustedCount;
  result.finiteReadings = _scanAccumulator.distanceCount;

  if (_scanAccumulator.noEchoCount >= 2 && _scanAccumulator.sensorFaultCount < 2) {
    result.ultrasonic.state = UltrasonicResultState::ClearNoEcho;
    result.completed = true;
  } else if (_scanAccumulator.distanceCount >= 2 && _scanAccumulator.sensorFaultCount < 2) {
    float ordered[RobotConfig::AUTO_SCAN_SAMPLES_PER_DIRECTION];
    for (uint8_t i = 0; i < _scanAccumulator.distanceCount; ++i)
      ordered[i] = _scanAccumulator.distances[i];
    for (uint8_t i = 0; i < _scanAccumulator.distanceCount; ++i) {
      for (uint8_t j = i + 1; j < _scanAccumulator.distanceCount; ++j) {
        if (ordered[j] < ordered[i]) {
          const float swap = ordered[i];
          ordered[i] = ordered[j];
          ordered[j] = swap;
        }
      }
    }
    result.ultrasonic.distanceCm = _scanAccumulator.distanceCount >= 3
      ? ordered[_scanAccumulator.distanceCount / 2] : ordered[0];
    result.spreadCm = ordered[_scanAccumulator.distanceCount - 1] - ordered[0];
    result.ultrasonic.valid = true;
    result.ultrasonic.state = UltrasonicResultState::DistanceValid;
    result.completed = true;
  } else if (_scanAccumulator.sensorFaultCount > 0) {
    result.ultrasonic.state = UltrasonicResultState::SensorFault;
  } else {
    result.ultrasonic.state = UltrasonicResultState::Stale;
  }
  return result;
}

void NavigationManager::storeCurrentScan(const NavigationScanSample& sample) {
  if (_scanDirectionIndex == 0) _leftScan = sample;
  else if (_scanDirectionIndex == 1) _frontScan = sample;
  else _rightScan = sample;
}

void NavigationManager::beginRecenter(unsigned long now) {
  _motors.stop();
  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.center();
  setState(NavigationState::Recenter, "SCAN_RECENTER");
  setPhase(AutoPhase::RecenterSettle, now);
}

void NavigationManager::selectPathAfterScan(unsigned long now) {
  if (!scanSampleTrusted(_leftScan) || !scanSampleTrusted(_frontScan) ||
      !scanSampleTrusted(_rightScan)) {
    if (_scanPurpose == ScanPurpose::EscapeV2) {
      enterV2TrappedHold("ESCAPE_SCAN_INVALID", now);
      return;
    }
    if (RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
        _scanPurpose != ScanPurpose::EnvironmentalRecovery &&
        _v2.noteInvalidScan(now)) {
      enterV2TrappedHold("REPEATED_INVALID_SCAN", now);
      return;
    }
    if (_scanPurpose == ScanPurpose::StuckRecovery)
      _lastStuckReason = "RECOVERY_SCAN_INVALID";
    enterInvalidStop("ULTRASONIC_SCAN_FAULT_STOP");
    return;
  }

  const float left = scanPathClearance(_leftScan);
  const float front = scanPathClearance(_frontScan);
  const float right = scanPathClearance(_rightScan);
  if (_scanPurpose == ScanPurpose::EscapeV2) {
    selectRecoveryDirection(now);
    return;
  }
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      _scanPurpose != ScanPurpose::EnvironmentalRecovery &&
      _scanPurpose != ScanPurpose::StuckRecovery) {
    const bool allFinite =
        _leftScan.ultrasonic.state == UltrasonicResultState::DistanceValid &&
        _frontScan.ultrasonic.state == UltrasonicResultState::DistanceValid &&
        _rightScan.ultrasonic.state == UltrasonicResultState::DistanceValid;
    _v2.noteAvoidance(now, _v2PriorForwardMs, allFinite);
    _v2PriorForwardMs = 0;
    V2ScanDecisionInput boxed;
    boxed.left = recoveryReading(_leftScan);
    boxed.front = recoveryReading(_frontScan);
    boxed.right = recoveryReading(_rightScan);
    const bool immediate = obviousBoxedIn(boxed, RobotConfig::RECOVERY_BOXED_SIDE_CM);
    if (immediate) _v2.noteBoxedIn();
    if (_v2.state() == StuckV2State::Suspected) {
      _v2LastReason = immediate ? "OBVIOUS_BOXED_IN" : "REPEATED_AVOIDANCE_HISTORY";
      beginV2Escape(now, true);
      return;
    }
  }
  const IrZone currentZone = currentIrZone();
  const bool leftIrClear = currentZone != IrZone::Left && currentZone != IrZone::Both;
  const bool rightIrClear = currentZone != IrZone::Right && currentZone != IrZone::Both;
  const bool leftSafe = leftIrClear && left > RobotConfig::AUTO_OBSTACLE_CM;
  const bool frontSafe = front > RobotConfig::AUTO_OBSTACLE_CM;
  const bool rightSafe = rightIrClear && right > RobotConfig::AUTO_OBSTACLE_CM;

  if (_scanPurpose == ScanPurpose::EnvironmentalRecovery && frontSafe &&
      (!leftSafe || front >= left) && (!rightSafe || front >= right)) {
    _dashboardData.autoSelectedPath = "FRONT";
    setState(NavigationState::Idle, "HAZARD_RECOVERY_FRONT_CLEAR");
    completeEnvironmentalRecovery();
    setPhase(AutoPhase::Idle, now);
    return;
  }

  bool turnLeft = false;
  if (!selectClearestSide(turnLeft)) {
    if (RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
        _scanPurpose != ScanPurpose::EnvironmentalRecovery) {
      _v2.noteBlockedScan();
      if (_v2.state() == StuckV2State::Suspected) {
        beginV2Escape(now, true);
        return;
      }
    }
    if (_scanPurpose == ScanPurpose::EnvironmentalRecovery) {
      enterInvalidStop("HAZARD_RECOVERY_NO_SAFE_PATH_STOP");
    } else {
      if (_scanPurpose == ScanPurpose::StuckRecovery)
      _lastStuckReason = "NO_SAFE_SIDE_PATH";
      enterBlockedRetryWait("BLOCKED_NO_SAFE_SIDE", now);
    }
    return;
  }

  if (_scanPurpose == ScanPurpose::IrSingle) {
    const bool preferLeft = _irEscapeSourceZone == IrZone::Right;
    if (preferLeft && leftSafe) turnLeft = true;
    else if (!preferLeft && rightSafe) turnLeft = false;
  }
  beginSpinTurn(turnLeft, _scanPurpose, now);
}

bool NavigationManager::selectClearestSide(bool& turnLeft) const {
  const float left = scanPathClearance(_leftScan);
  const float right = scanPathClearance(_rightScan);
  const IrZone currentZone = currentIrZone();
  const bool leftSafe = currentZone != IrZone::Left && currentZone != IrZone::Both &&
    scanSampleTrusted(_leftScan) && left > RobotConfig::AUTO_OBSTACLE_CM;
  const bool rightSafe = currentZone != IrZone::Right && currentZone != IrZone::Both &&
    scanSampleTrusted(_rightScan) && right > RobotConfig::AUTO_OBSTACLE_CM;
  if (!leftSafe && !rightSafe) return false;
  if (leftSafe && !rightSafe) turnLeft = true;
  else if (rightSafe && !leftSafe) turnLeft = false;
  else if (fabsf(left - right) <= RobotConfig::AUTO_PATH_TIE_TOLERANCE_CM)
    turnLeft = !_lastAvoidanceTurnLeft;
  else turnLeft = left > right;
  return true;
}

void NavigationManager::beginSpinTurn(bool turnLeft, ScanPurpose purpose, unsigned long now) {
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      purpose != ScanPurpose::EnvironmentalRecovery &&
      purpose != ScanPurpose::StuckRecovery &&
      purpose != ScanPurpose::EscapeV2) {
    _v2.noteTurn(turnLeft);
    if (_v2.state() == StuckV2State::Suspected) {
      beginV2Escape(now, true);
      return;
    }
  }
  _turnLeft = turnLeft;
  _scanPurpose = purpose;
  _relativeTurnDeg = 0.0f;
  _turnStartedMs = now;
  _dashboardData.autoSelectedPath = turnLeft ? "LEFT" : "RIGHT";
  _dashboardData.autoTurnCompletionReason = "IN_PROGRESS";
  const ImuSnapshot& initial = _imu.snapshot();
  _turnUsingGyro = initial.initialized && initial.healthy && isfinite(initial.gyroZ) &&
    initial.lastSuccessMs != 0 && now - initial.lastSuccessMs <= RobotConfig::IMU_STALE_MS;
  _turnGyroBiasRadPerSecond = _turnUsingGyro ? initial.gyroZ : 0.0f;
  _lastIntegratedSampleMs = _turnUsingGyro ? initial.sampleMs : 0;

  const NavigationState turnState = purpose == ScanPurpose::EnvironmentalRecovery
    ? (turnLeft ? NavigationState::SpinLeft : NavigationState::SpinRight)
    : NavigationState::ObstacleSpin;
  setState(turnState, turnLeft ? "SPIN_LEFT_RELATIVE_TURN" : "SPIN_RIGHT_RELATIVE_TURN");
  // These are the exact MotorController primitives used by Manual SPIN mode.
  // The MPU only bounds how long the already-proven wheel actuation continues.
  if (turnLeft) _motors.spinLeft();
  else _motors.spinRight();
  setPhase(AutoPhase::Turn, now);
}

void NavigationManager::updateSpinTurn(unsigned long now) {
  const SensorSnapshot& control = _sensors.snapshot();
  if (control.ultrasonicState == UltrasonicResultState::SensorFault ||
      control.ultrasonicState == UltrasonicResultState::Stale) {
    _motors.stop();
    _dashboardData.autoTurnCompletionReason = "ULTRASONIC_INVALID_INTERRUPT";
    enterInvalidStop("SPIN_ULTRASONIC_INVALID_STOP");
    return;
  }
  const bool escapeTurn=RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      _scanPurpose == ScanPurpose::EscapeV2;
  if (escapeTurn && !_turnUsingGyro) {
    enterV2TrappedHold("ESCAPE_IMU_INVALID", now);
    return;
  }
  if (_turnUsingGyro) {
    const ImuSnapshot& sample = _imu.snapshot();
    const bool usable = sample.initialized && sample.healthy && isfinite(sample.gyroZ) &&
      sample.lastSuccessMs != 0 && now - sample.lastSuccessMs <= RobotConfig::IMU_STALE_MS;
    if (!usable) {
      _turnUsingGyro = false;
      if (escapeTurn) {
        enterV2TrappedHold("ESCAPE_IMU_STALE", now);
        return;
      }
    } else if (sample.sampleMs != _lastIntegratedSampleMs) {
      const unsigned long sampleDeltaMs = sample.sampleMs - _lastIntegratedSampleMs;
      _lastIntegratedSampleMs = sample.sampleMs;
      if (sampleDeltaMs == 0 || sampleDeltaMs > RobotConfig::IMU_STALE_MS) {
        _turnUsingGyro = false;
        if (escapeTurn) {
          enterV2TrappedHold("ESCAPE_IMU_SAMPLE_GAP", now);
          return;
        }
      } else {
        const float rateDegPerSecond =
          fabsf((sample.gyroZ - _turnGyroBiasRadPerSecond) * RAD_TO_DEG);
        if (rateDegPerSecond >= RobotConfig::AUTO_AVOIDANCE_GYRO_DEADBAND_DPS)
          _relativeTurnDeg += rateDegPerSecond * (static_cast<float>(sampleDeltaMs) / 1000.0f);
        const float target=escapeTurn
            ? RobotConfig::RECOVERY_TURN_TARGET_DEG
            : RobotConfig::AUTO_AVOIDANCE_TURN_TARGET_DEG;
        if (_relativeTurnDeg >= target +
                                RobotConfig::AUTO_AVOIDANCE_TURN_TOLERANCE_DEG) {
          completeSpin("GYRO_TARGET", now);
          return;
        }
      }
    }
  }
  if (!escapeTurn && !_turnUsingGyro &&
      now - _turnStartedMs >= RobotConfig::AUTO_AVOIDANCE_TIMED_FALLBACK_MS) {
    completeSpin("TIMED_FALLBACK", now);
    return;
  }
  if (now - _turnStartedMs >= (escapeTurn
      ? RobotConfig::RECOVERY_TURN_TIMEOUT_MS
      : RobotConfig::AUTO_AVOIDANCE_TURN_TIMEOUT_MS)) {
    _motors.stop();
    _dashboardData.autoTurnCompletionReason = "HARD_TIMEOUT";
    if (escapeTurn) enterV2TrappedHold("ESCAPE_TURN_TIMEOUT", now);
    else enterInvalidStop(_turnLeft ? "LEFT_SPIN_TIMEOUT_STOP" : "RIGHT_SPIN_TIMEOUT_STOP");
  }
}

void NavigationManager::completeSpin(const char* reason, unsigned long now) {
  _motors.stop();
  if (_scanPurpose == ScanPurpose::EscapeV2) _v2.turnVerified();
  else _lastAvoidanceTurnLeft = _turnLeft;
  _dashboardData.autoTurnCompletionReason = reason;
  setState(_state, _turnLeft ? "LEFT_SPIN_COMPLETE" : "RIGHT_SPIN_COMPLETE");
  setPhase(AutoPhase::PostTurnSettle, now);
}

void NavigationManager::finishRecoveryAfterSettle(unsigned long now) {
  if (_scanPurpose == ScanPurpose::EscapeV2) {
    beginRecoveryFrontCheck(now);
    return;
  }
  if (_scanPurpose == ScanPurpose::EnvironmentalRecovery) {
    completeEnvironmentalRecovery();
  } else {
    if (_scanPurpose == ScanPurpose::StuckRecovery) {
      resetStuckEvidence(false);
      _stuckRecoveryTrialActive = true;
    } else {
      resetStuckEvidence(true);
    }
    _obstacleReevaluationPending = true;
    beginFrontValidation(now, "OBSTACLE_POST_SPIN_REEVALUATE");
    setState(NavigationState::ObstacleReevaluate, "OBSTACLE_POST_SPIN_REEVALUATE");
    return;
  }
  setState(NavigationState::Idle, "POST_SPIN_REVALIDATE");
  setPhase(AutoPhase::Idle, now);
}

void NavigationManager::enterInvalidStop(const char* reason) {
  const UltrasonicResultState sonar = _sensors.snapshot().ultrasonicState;
  if (sonar == UltrasonicResultState::SensorFault ||
      sonar == UltrasonicResultState::Stale) {
    beginSonarReacquire(millis());
    return;
  }
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      (_v2.state() == StuckV2State::EscapeScan ||
       _v2.state() == StuckV2State::EscapeTurn ||
       _v2.state() == StuckV2State::EscapeVerify)) {
    enterV2TrappedHold(reason, millis());
    return;
  }
  _motors.stop();
  _servoScanner.detach();
  resetStuckEvidence(false);
  setState(NavigationState::InvalidStop, reason);
  setPhase(AutoPhase::InvalidStop, millis());
}

void NavigationManager::beginSonarReacquire(unsigned long now) {
  _motors.stop();
  _servoScanner.center();
  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _turnUsingGyro = false;
  _relativeTurnDeg = 0.0f;
  _turnStartedMs = 0;
  _lastIntegratedSampleMs = 0;
  _recoveryIrBackoff.resetMission();
  _recoverySideRechecks = 0;
  if (RobotConfig::ENABLE_STUCK_RECOVERY_V2) _v2.abort();
  _scanPurpose = ScanPurpose::Obstacle;
  _obstacleRecoveryActive = false;
  _obstacleReevaluationPending = false;
  _obstacleForwardClearMs = 0;
  if (_environmentalRecoveryActive) _environmentalRecoveryCompleted = true;
  _environmentalRecoveryActive = false;
  _hazardClearStartedMs = 0;
  resetStuckEvidence(true);
  _sonarReacquire.begin(now, _sensors.distanceSampleSequence());
  _sonarReacquireExtended = false;
  setState(NavigationState::SonarReacquire, "SONAR_REACQUIRE");
  setPhase(AutoPhase::SonarReacquire, now);
}

void NavigationManager::updateSonarReacquire(unsigned long now) {
  _motors.stop();
  _sensors.setDistanceFacingFront(true);
  if (now - _phaseStartedMs < RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS) return;
  _sonarReacquire.centered(_sensors.distanceSampleSequence());
  const UltrasonicSample front = _sensors.latestUltrasonicSample();
  const bool fresh = front.sampleMs != 0 &&
      now - front.sampleMs <= RobotConfig::ULTRASONIC_STALE_MS;
  const bool healthy = fresh && front.facingFront &&
      (front.state == UltrasonicResultState::ClearNoEcho ||
       (front.state == UltrasonicResultState::DistanceValid && front.valid &&
        isfinite(front.distanceCm) && front.distanceCm > 0.0f));
  if (front.state == UltrasonicResultState::SensorFault ||
      front.state == UltrasonicResultState::Stale) {
    _sonarReacquire.invalidate();
  }
  if (_sonarReacquire.observe(front.sequence, healthy)) {
    _servoScanner.detach();
    _sampleTargetSequence = front.sequence;
    setState(NavigationState::FrontSample, "SONAR_REACQUIRED_FRONT_DECISION");
    setPhase(AutoPhase::FrontSampleWait, now);
    updateFrontValidation(now); // fresh normal AUTO decision, never old turn/backoff
    return;
  }
  if (!_sonarReacquireExtended &&
      _sonarReacquire.windowExpired(
          now, RobotConfig::SONAR_FAST_REACQUIRE_WINDOW_MS)) {
    _sonarReacquireExtended = true;
    setState(NavigationState::SonarReacquire, "SONAR_REACQUIRE_RETRY");
  }
}

void NavigationManager::enterEnvironmentalHazard(unsigned long now) {
  _motors.stop();
  _servoScanner.detach();
  _environmentalRecoveryActive = true;
  _environmentalRecoveryCompleted = false;
  _hazardClearStartedMs = 0;
  _dashboardData.autoAvoidanceSource = "HAZARD";
  _dashboardData.autoTurnCompletionReason = "ENVIRONMENTAL_SAFETY_INTERRUPT";
  if (_phase != AutoPhase::HazardActive) {
    _irEscapeSourceZone = IrZone::Clear;
    _obstacleRecoveryActive = false;
    _obstacleReevaluationPending = false;
    _obstacleRecoveryAttempts = 0;
    _blockedRetryCount = 0;
    _obstacleForwardClearMs = 0;
    _dashboardData.autoPreferredPath = "NONE";
    resetStuckEvidence(true);
    setState(NavigationState::HazardActive, "ENVIRONMENTAL_HAZARD_STOP");
    setPhase(AutoPhase::HazardActive, now);
  }
}

void NavigationManager::updateEnvironmentalRecovery(unsigned long now) {
  _motors.stop();
  if (_safety.isAutoRecoverableEnvironmentalHazard()) {
    enterEnvironmentalHazard(now);
    return;
  }
  if (_safety.hasNonRecoverableCriticalHazard() || _safety.isCritical()) {
    _environmentalRecoveryActive = false;
    _hazardClearStartedMs = 0;
    setState(NavigationState::CriticalAbort, "SAFETY_STOP");
    return;
  }
  if (_phase == AutoPhase::HazardActive) {
    _hazardClearStartedMs = now;
    setState(NavigationState::HazardClearWait, "HAZARD_CONTINUOUS_CLEAR_WAIT");
    setPhase(AutoPhase::HazardClearWait, now);
    return;
  }
  if (now - _hazardClearStartedMs >= RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS) {
    setState(NavigationState::HazardRecoveryScan, "HAZARD_CLEAR_RECOVERY_SCAN");
    beginScan(ScanPurpose::EnvironmentalRecovery, now);
  }
}

void NavigationManager::completeEnvironmentalRecovery() {
  _environmentalRecoveryActive = false;
  _environmentalRecoveryCompleted = true;
  _hazardClearStartedMs = 0;
  _dashboardData.autoHazardRecoveryState = "RESUME_READY";
}

void NavigationManager::resetStuckEvidence(bool clearAttempts) {
  _stuckDistanceCount = 0;
  _stuckDistanceIndex = 0;
  _stuckForwardActiveMs = 0;
  _stuckImuStillMs = 0;
  _stuckRecoveryForwardMs = 0;
  if (clearAttempts) {
    _stuckRecoveryAttempts = 0;
    _stuckRecoveryTrialActive = false;
    _lastStuckReason = "NONE";
  }
}

void NavigationManager::updateStuckEvidence(unsigned long now, unsigned long elapsedMs) {
  _stuckForwardActiveMs += elapsedMs;
  if (_stuckRecoveryTrialActive) {
    _stuckRecoveryForwardMs += elapsedMs;
    if (_stuckRecoveryForwardMs >= RobotConfig::AUTO_STUCK_RETRY_RESET_FORWARD_MS) {
      resetStuckEvidence(true);
      return;
    }
  }
  const ImuSnapshot& sample = _imu.snapshot();
  const bool imuFresh = sample.initialized && sample.healthy && sample.lastSuccessMs != 0 &&
    now - sample.lastSuccessMs <= RobotConfig::IMU_STALE_MS;
  if (imuFresh && strcmp(sample.motionState, "STILL") == 0)
    _stuckImuStillMs += elapsedMs;
  else _stuckImuStillMs = 0;
}

void NavigationManager::recordStuckDistance(const UltrasonicSample& sample) {
  if (sample.state != UltrasonicResultState::DistanceValid || !sample.valid ||
      !sample.facingFront || !isfinite(sample.distanceCm)) {
    _stuckDistanceCount = 0;
    _stuckDistanceIndex = 0;
    return;
  }
  _stuckDistanceSamples[_stuckDistanceIndex] = sample.distanceCm;
  _stuckDistanceIndex = (_stuckDistanceIndex + 1) % RobotConfig::AUTO_STUCK_DISTANCE_SAMPLE_COUNT;
  if (_stuckDistanceCount < RobotConfig::AUTO_STUCK_DISTANCE_SAMPLE_COUNT)
    _stuckDistanceCount++;
}

bool NavigationManager::stuckDistanceUnchanged() const {
  if (_stuckDistanceCount < RobotConfig::AUTO_STUCK_DISTANCE_SAMPLE_COUNT) return false;
  float minimum = _stuckDistanceSamples[0];
  float maximum = _stuckDistanceSamples[0];
  for (uint8_t i = 1; i < RobotConfig::AUTO_STUCK_DISTANCE_SAMPLE_COUNT; ++i) {
    if (_stuckDistanceSamples[i] < minimum) minimum = _stuckDistanceSamples[i];
    if (_stuckDistanceSamples[i] > maximum) maximum = _stuckDistanceSamples[i];
  }
  return maximum - minimum <= RobotConfig::AUTO_STUCK_DISTANCE_VARIATION_CM;
}

bool NavigationManager::stuckEvidenceSatisfied() const {
  return _stuckForwardActiveMs >= RobotConfig::AUTO_STUCK_FORWARD_MIN_MS &&
         _stuckImuStillMs >= RobotConfig::AUTO_STUCK_IMU_STILL_MS &&
         stuckDistanceUnchanged();
}

void NavigationManager::updateV2RetryCooldown(unsigned long now) {
  _motors.stop();
  // serviceLoop continues sensors, Wi-Fi and operator STOP while we wait.
  // A fresh scan, with motors OFF, rechecks sonar before any new turn.
  if (!_v2.retryReady(now, RobotConfig::RECOVERY_COOLDOWN_MS)) return;
  if (!_v2.beginRetryRound(now)) return;
  _recoveryIrBackoff.resetMission();
  _recoverySideRechecks=0;
  _relativeTurnDeg=0.0f;
  _dashboardData.autoSelectedPath="NONE";
  _v2LastReason="RETRY_FRESH_SCAN";
  setState(NavigationState::StuckSuspected, "V2_RETRY_FRESH_SCAN");
  beginScan(ScanPurpose::EscapeV2, now);
}

void NavigationManager::beginV2Escape(unsigned long now, bool reuseScan) {
  _motors.stop();
  _servoScanner.detach();
  _obstacleRecoveryActive = false;
  _obstacleReevaluationPending = false;
  if (!_v2.startEscape(now)) {
    enterV2TrappedHold("ESCAPE_START_FAILED", now);
    return;
  }
  _recoverySideRechecks = 0;
  _recoveryIrBackoff.resetMission();
  setState(NavigationState::StuckSuspected,
           reuseScan ? "STUCK_REUSE_COMPLETE_SCAN" : "STUCK_FRESH_SCAN");
  if (reuseScan && scanSampleTrusted(_leftScan) &&
      scanSampleTrusted(_frontScan) && scanSampleTrusted(_rightScan)) {
    selectRecoveryDirection(now);
  } else {
    beginScan(ScanPurpose::EscapeV2, now);
  }
}

void NavigationManager::selectRecoveryDirection(unsigned long now) {
  V2ScanDecisionInput input;
  input.left = recoveryReading(_leftScan);
  input.front = recoveryReading(_frontScan);
  input.right = recoveryReading(_rightScan);
  input.imuReady = v2ImuReady(now);
  input.previousFailedLeft = _v2.previousFailedLeft();
  input.hardCollisionCm = RobotConfig::RECOVERY_HARD_VETO_CM;
  input.tieToleranceCm = RobotConfig::AUTO_PATH_TIE_TOLERANCE_CM;
  const V2ScanAction action = decideV2Scan(input);
  if (action == V2ScanAction::Hold) {
    enterV2TrappedHold("ESCAPE_INITIAL_SCAN_UNSAFE", now);
    return;
  }
  const bool turnLeft = action == V2ScanAction::TurnLeft;
  _v2.commitSide(turnLeft);
  beginV2Turn(turnLeft, now);
}

void NavigationManager::beginRecoverySideCheck(unsigned long now) {
  _motors.stop();
  _scanPurpose = ScanPurpose::RecoverySide;
  _scanDirectionIndex = _v2.committedLeft() ? 0 : 2;
  _scanReadingIndex = 0;
  _scanAccumulator = ScanAccumulator();
  commandCurrentScanDirection(now);
}

void NavigationManager::finishRecoverySideCheck(unsigned long now) {
  const NavigationScanSample& side =
      _v2.committedLeft() ? _leftScan : _rightScan;
  if (currentIrZone() != IrZone::Clear) {
    beginRecoveryIrBackoff(now);
    return;
  }
  if (scanSampleTrusted(side)) {
    const RecoveryReading reading = recoveryReading(side);
    if (reading.state == RecoveryReadingState::DistanceValid &&
        reading.cm <= RobotConfig::RECOVERY_HARD_VETO_CM) {
      beginRecoveryIrBackoff(now, true);
      return;
    }
    if (recoveryPathSafe(reading, RobotConfig::RECOVERY_HARD_VETO_CM)) {
      _recoverySideRechecks = 0;
      _v2.continueTurn();
      beginV2Turn(_v2.committedLeft(), now);
      return;
    }
  }
  if (_recoverySideRechecks == 0) {
    _recoverySideRechecks = 1;
    beginRecoverySideCheck(now);
    return;
  }
  enterV2TrappedHold("ESCAPE_SELECTED_SIDE_INVALID", now);
}

void NavigationManager::beginRecoveryFrontCheck(unsigned long now) {
  _motors.stop();
  _currentServoPosition = "FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.center();
  setState(NavigationState::StuckRecovery, "ESCAPE_FRONT_CHECK");
  setPhase(AutoPhase::RecoveryFrontSettle, now);
}

void NavigationManager::updateRecoveryFrontCheck(unsigned long now) {
  if (_phase == AutoPhase::RecoveryFrontSettle) {
    if (now - _phaseStartedMs < RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS ||
        _sensors.isDistanceSampleInProgress()) return;
    _sampleTargetSequence = _sensors.requestDistanceSample(true);
    _sampleRequestedMs = now;
    setPhase(AutoPhase::RecoveryFrontWait, now);
    return;
  }
  if (_sensors.distanceSampleSequence() < _sampleTargetSequence) {
    if (now - _sampleRequestedMs > RobotConfig::ULTRASONIC_SAMPLE_TIMEOUT_MS + 20UL)
      enterV2TrappedHold("ESCAPE_FRONT_TIMEOUT", now);
    return;
  }
  const UltrasonicSample front = _sensors.latestUltrasonicSample();
  if (front.sequence < _sampleTargetSequence || !front.facingFront ||
      front.sampleMs == 0 || now - front.sampleMs > RobotConfig::ULTRASONIC_STALE_MS ||
      (front.state != UltrasonicResultState::ClearNoEcho &&
       (front.state != UltrasonicResultState::DistanceValid || !front.valid ||
        !isfinite(front.distanceCm) || front.distanceCm <= 0.0f))) {
    enterV2TrappedHold("ESCAPE_FRONT_INVALID", now);
    return;
  }
  RecoveryReading reading;
  reading.state = front.state == UltrasonicResultState::ClearNoEcho
      ? RecoveryReadingState::ClearNoEcho : RecoveryReadingState::DistanceValid;
  reading.cm = front.distanceCm;
  if (recoveryFrontClear(reading, RobotConfig::RECOVERY_FRONT_EXIT_CM)) {
    _motors.stop();
    _servoScanner.detach();
    _v2.recovered();
    _recoveryIrBackoff.resetMission();
    _scanPurpose = ScanPurpose::Obstacle;
    _recoverySideRechecks = 0;
    _obstacleRecoveryAttempts = 0;
    _obstacleForwardClearMs = 0;
    _v2LastReason = "FRONT_HEADING_CLEAR";
    setState(NavigationState::Idle, "RECOVERY_HANDOFF_NORMAL_AUTO");
    setPhase(AutoPhase::Idle, now);
    return;
  }
  if (_v2.sideTurns() >= RobotConfig::RECOVERY_MAX_TURNS) {
    enterV2TrappedHold("ESCAPE_TURN_LIMIT", now);
    return;
  }
  _recoverySideRechecks = 0;
  beginRecoverySideCheck(now);
}

void NavigationManager::beginRecoveryIrBackoff(unsigned long now,
                                               bool requireFirstPulse) {
  _motors.stop();
  _servoScanner.detach();
  _turnUsingGyro = false;
  _relativeTurnDeg = 0.0f;
  _turnStartedMs = 0;
  _lastIntegratedSampleMs = 0;
  const UltrasonicResultState sonar = _sensors.snapshot().ultrasonicState;
  if (sonar == UltrasonicResultState::SensorFault ||
      sonar == UltrasonicResultState::Stale) {
    beginSonarReacquire(now);
    return;
  }
  const RecoveryIrBackoffPolicy::Action action =
      _recoveryIrBackoff.start(now, requireFirstPulse);
  if (action != RecoveryIrBackoffPolicy::Action::Backward) {
    enterV2TrappedHold("ESCAPE_BACKOFF_BUDGET_EXHAUSTED", now);
    return;
  }
  setState(NavigationState::RecoveryIrBackoff, "RECOVERY_IR_BACKOFF");
  setPhase(AutoPhase::RecoveryIrBackoff, now);
  // MotorController uses its current user-selected speed for backward().
  _motors.backward();
  _recoveryIrBackoff.pulseCommanded(millis());
}

void NavigationManager::updateRecoveryIrBackoff(unsigned long now) {
  const UltrasonicResultState sonar = _sensors.snapshot().ultrasonicState;
  if (sonar == UltrasonicResultState::SensorFault ||
      sonar == UltrasonicResultState::Stale) {
    beginSonarReacquire(now);
    return;
  }
  const RecoveryIrBackoffPolicy::Action action =
      _recoveryIrBackoff.update(now, currentIrZone() != IrZone::Clear);
  switch (action) {
    case RecoveryIrBackoffPolicy::Action::None:
      return;
    case RecoveryIrBackoffPolicy::Action::Backward:
      _motors.backward();
      _recoveryIrBackoff.pulseCommanded(millis());
      return;
    case RecoveryIrBackoffPolicy::Action::Stop:
      _motors.stop();
      return;
    case RecoveryIrBackoffPolicy::Action::FrontCheck:
      _motors.stop();
      beginRecoveryFrontCheck(now);
      return;
    case RecoveryIrBackoffPolicy::Action::Failed:
      enterV2TrappedHold("ESCAPE_IR_BACKOFF_LIMIT", now);
      return;
  }
}

bool NavigationManager::v2ImuReady(unsigned long now) const {
  const ImuSnapshot& sample=_imu.snapshot();
  return sample.initialized && sample.healthy && isfinite(sample.gyroZ) &&
      sample.sampleMs != 0 && sample.lastSuccessMs != 0 &&
      now - sample.lastSuccessMs <= RobotConfig::IMU_STALE_MS;
}

void NavigationManager::beginV2Turn(bool turnLeft, unsigned long now) {
  _motors.stop();
  if (!_v2.hasCommittedSide() || _v2.committedLeft() != turnLeft) {
    enterV2TrappedHold("ESCAPE_DIRECTION_LOCK_INVALID", now);
    return;
  }
  if (currentIrZone() != IrZone::Clear) {
    beginRecoveryIrBackoff(now);
    return;
  }
  if (!v2ImuReady(now) || !_v2.startTurn(RobotConfig::RECOVERY_MAX_TURNS)) {
    enterV2TrappedHold("ESCAPE_TURN_PRECHECK_FAILED", now);
    return;
  }
  _turnLeft=turnLeft;
  _scanPurpose=ScanPurpose::EscapeV2;
  _relativeTurnDeg=0.0f;
  _turnStartedMs=now;
  _dashboardData.autoSelectedPath=turnLeft ? "LEFT" : "RIGHT";
  _dashboardData.autoTurnTargetDeg=RobotConfig::RECOVERY_TURN_TARGET_DEG;
  _dashboardData.autoTurnCompletionReason="IN_PROGRESS";
  const ImuSnapshot& initial=_imu.snapshot();
  _turnUsingGyro=true;
  _turnGyroBiasRadPerSecond=initial.gyroZ;
  _lastIntegratedSampleMs=initial.sampleMs;
  _currentServoPosition="FRONT";
  _sensors.setDistanceFacingFront(true);
  _servoScanner.detach();
  setState(NavigationState::StuckRecovery,
           turnLeft ? "RECOVERY_INCREMENT_LEFT" : "RECOVERY_INCREMENT_RIGHT");
  if (turnLeft) _motors.spinLeft();
  else _motors.spinRight();
  setPhase(AutoPhase::Turn, now);
}

void NavigationManager::enterV2TrappedHold(const char* reason, unsigned long now) {
  const UltrasonicResultState sonar = _sensors.snapshot().ultrasonicState;
  if (sonar == UltrasonicResultState::SensorFault ||
      sonar == UltrasonicResultState::Stale) {
    beginSonarReacquire(now);
    return;
  }
  _motors.stop();
  _servoScanner.detach();
  _v2.hold(now);
  _recoverySideRechecks = 0;
  _recoveryIrBackoff.resetMission();
  _v2LastReason = reason;
  _obstacleRecoveryActive = false;
  _obstacleReevaluationPending = false;
  _dashboardData.autoAssistanceRequired = true;
  setState(NavigationState::TrappedHold, "TRAPPED_HOLD_ASSISTANCE_REQUIRED");
  setPhase(AutoPhase::TrappedHold, now);
  Serial.printf("[Nav] V2 cooldown reason=%s attempts=%u\n", reason, _v2.attempts());
}

void NavigationManager::beginStuckRecovery(unsigned long now) {
  _motors.stop();
  _lastStuckReason = "FORWARD_COMMAND_PLUS_IMU_STILL_PLUS_STATIC_FRONT_GEOMETRY";
  _stuckRecoveryAttempts++;
  _stuckRecoveryTrialActive = false;
  beginUnifiedObstacleRecovery(ScanPurpose::StuckRecovery, IrZone::Clear, now);
}

bool NavigationManager::scanSampleTrusted(const NavigationScanSample& sample) const {
  if (!sample.completed || sample.trustedReadings < 2 || sample.ultrasonic.sampleMs == 0 ||
      millis() - sample.ultrasonic.sampleMs > RobotConfig::NAVIGATION_SCAN_SET_MAX_AGE_MS)
    return false;
  return (sample.ultrasonic.state == UltrasonicResultState::DistanceValid &&
          sample.ultrasonic.valid && isfinite(sample.ultrasonic.distanceCm)) ||
         sample.ultrasonic.state == UltrasonicResultState::ClearNoEcho;
}

RecoveryReading NavigationManager::recoveryReading(
    const NavigationScanSample& sample) const {
  RecoveryReading reading;
  if (!scanSampleTrusted(sample)) return reading;
  reading.state = sample.ultrasonic.state == UltrasonicResultState::ClearNoEcho
      ? RecoveryReadingState::ClearNoEcho : RecoveryReadingState::DistanceValid;
  reading.cm = sample.ultrasonic.distanceCm;
  return reading;
}

float NavigationManager::scanPathClearance(const NavigationScanSample& sample) const {
  return sample.ultrasonic.state == UltrasonicResultState::ClearNoEcho
    ? RobotConfig::ULTRASONIC_MAX_CM + 1.0f : sample.ultrasonic.distanceCm;
}

const char* NavigationManager::ultrasonicStateName(UltrasonicResultState state) const {
  switch (state) {
    case UltrasonicResultState::DistanceValid: return "DISTANCE_VALID";
    case UltrasonicResultState::ClearNoEcho: return "CLEAR_NO_ECHO";
    case UltrasonicResultState::SensorFault: return "SENSOR_FAULT";
    case UltrasonicResultState::Stale: return "STALE";
  }
  return "STALE";
}

const char* NavigationManager::phaseName() const {
  switch (_phase) {
    case AutoPhase::Idle: return "IDLE";
    case AutoPhase::FrontSettle: return "FRONT_SETTLE";
    case AutoPhase::FrontSampleWait: return "FRONT_SAMPLE_WAIT";
    case AutoPhase::Forward: return "FORWARD";
    case AutoPhase::ForwardSettle: return "FORWARD_SETTLE";
    case AutoPhase::ReversePrepare: return "REVERSE_PREPARE";
    case AutoPhase::Reverse: return "REVERSE";
    case AutoPhase::ReverseSettle: return "REVERSE_SETTLE";
    case AutoPhase::ScanServoSettle: return "SCAN_SERVO_SETTLE";
    case AutoPhase::ScanSampleGap: return "SCAN_SAMPLE_GAP";
    case AutoPhase::ScanSampleWait: return "SCAN_SAMPLE_WAIT";
    case AutoPhase::RecenterSettle: return "RECENTER_SETTLE";
    case AutoPhase::Turn: return "SPIN_TURN";
    case AutoPhase::PostTurnSettle: return "POST_TURN_SETTLE";
    case AutoPhase::HazardActive: return "HAZARD_ACTIVE";
    case AutoPhase::HazardClearWait: return "HAZARD_CLEAR_WAIT";
    case AutoPhase::InvalidStop: return "INVALID_STOP";
    case AutoPhase::SonarReacquire: return "SONAR_REACQUIRE";
    case AutoPhase::OperatorStop: return "OPERATOR_STOP";
    case AutoPhase::BlockedRetryWait: return "BLOCKED_RETRY_WAIT";
    case AutoPhase::RecoveryFrontSettle: return "RECOVERY_FRONT_SETTLE";
    case AutoPhase::RecoveryFrontWait: return "RECOVERY_FRONT_WAIT";
    case AutoPhase::RecoveryIrBackoff: return "RECOVERY_IR_BACKOFF";
    case AutoPhase::TrappedHold: return "TRAPPED_HOLD";
  }
  return "UNKNOWN";
}

void NavigationManager::updateDiagnostics(unsigned long now) {
  _dashboardData.autoNavigationState = phaseName();
  _dashboardData.autoIrZoneState = irZoneName(currentIrZone());
  _dashboardData.autoObstacleRecoveryAttempts = _obstacleRecoveryAttempts;
  _dashboardData.autoBlockedRetryCount = _blockedRetryCount;
  _dashboardData.autoTurnTargetDeg =
      RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      (_v2.state() == StuckV2State::EscapeTurn ||
       (_v2.state() == StuckV2State::EscapeVerify &&
        _phase == AutoPhase::PostTurnSettle))
      ? RobotConfig::RECOVERY_TURN_TARGET_DEG
      : RobotConfig::AUTO_AVOIDANCE_TURN_TARGET_DEG;
  _dashboardData.autoRelativeTurnDeg = _relativeTurnDeg;
  _dashboardData.autoScanLeftState = _leftScan.completed
    ? ultrasonicStateName(_leftScan.ultrasonic.state) : "NOT_SAMPLED";
  _dashboardData.autoScanFrontState = _frontScan.completed
    ? ultrasonicStateName(_frontScan.ultrasonic.state) : "NOT_SAMPLED";
  _dashboardData.autoScanRightState = _rightScan.completed
    ? ultrasonicStateName(_rightScan.ultrasonic.state) : "NOT_SAMPLED";
  _dashboardData.autoScanLeftDistanceCm =
    _leftScan.ultrasonic.state == UltrasonicResultState::DistanceValid
      ? _leftScan.ultrasonic.distanceCm : NAN;
  _dashboardData.autoScanFrontDistanceCm =
    _frontScan.ultrasonic.state == UltrasonicResultState::DistanceValid
      ? _frontScan.ultrasonic.distanceCm : NAN;
  _dashboardData.autoScanRightDistanceCm =
    _rightScan.ultrasonic.state == UltrasonicResultState::DistanceValid
      ? _rightScan.ultrasonic.distanceCm : NAN;
  _dashboardData.autoScanLeftTrusted = scanSampleTrusted(_leftScan);
  _dashboardData.autoScanFrontTrusted = scanSampleTrusted(_frontScan);
  _dashboardData.autoScanRightTrusted = scanSampleTrusted(_rightScan);

  if (_safety.isAutoRecoverableEnvironmentalHazard() || _phase == AutoPhase::HazardActive) {
    _dashboardData.autoHazardRecoveryState = "HAZARD_ACTIVE";
    _dashboardData.autoHazardClearElapsedMs = 0;
    _dashboardData.autoHazardClearRemainingMs = RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS;
  } else if (_phase == AutoPhase::HazardClearWait) {
    const unsigned long elapsed = now - _hazardClearStartedMs;
    _dashboardData.autoHazardRecoveryState = "CONTINUOUS_CLEAR_WAIT";
    _dashboardData.autoHazardClearElapsedMs = elapsed;
    _dashboardData.autoHazardClearRemainingMs = elapsed >= RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS
      ? 0 : RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS - elapsed;
  } else if (_environmentalRecoveryActive) {
    _dashboardData.autoHazardRecoveryState = "RECOVERY_SCAN";
    _dashboardData.autoHazardClearElapsedMs = RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS;
    _dashboardData.autoHazardClearRemainingMs = 0;
  } else if (!_environmentalRecoveryCompleted) {
    _dashboardData.autoHazardRecoveryState = "IDLE";
    _dashboardData.autoHazardClearElapsedMs = 0;
    _dashboardData.autoHazardClearRemainingMs = 0;
  }

  if (_scanPurpose == ScanPurpose::StuckRecovery &&
             _phase == AutoPhase::BlockedRetryWait) {
    _dashboardData.autoStuckState = "BLOCKED_RETRY_WAIT";
  } else if (_scanPurpose == ScanPurpose::StuckRecovery &&
             (_phase == AutoPhase::ReversePrepare || _phase == AutoPhase::Reverse ||
              _phase == AutoPhase::ReverseSettle || _phase == AutoPhase::ScanServoSettle ||
              _phase == AutoPhase::ScanSampleGap || _phase == AutoPhase::ScanSampleWait ||
              _phase == AutoPhase::RecenterSettle || _phase == AutoPhase::Turn ||
              _phase == AutoPhase::PostTurnSettle)) {
    _dashboardData.autoStuckState = "RECOVERY";
  } else if (_state == NavigationState::StuckSuspected) {
    _dashboardData.autoStuckState = "SUSPECTED";
  } else {
    _dashboardData.autoStuckState = "MONITORING";
  }
  _dashboardData.autoStuckRecoveryAttempts = _stuckRecoveryAttempts;
  _dashboardData.autoStuckLastReason = _lastStuckReason;
  _dashboardData.autoV2State = RobotConfig::ENABLE_STUCK_RECOVERY_V2
      ? _v2.name() : "OFF";
  _dashboardData.autoV2Attempts = RobotConfig::ENABLE_STUCK_RECOVERY_V2
      ? _v2.attempts() : 0;
  _dashboardData.autoAssistanceRequired =
      RobotConfig::ENABLE_STUCK_RECOVERY_V2 &&
      _v2.state() == StuckV2State::TrappedHold;
  _dashboardData.autoV2Reason = _v2LastReason;
}
