"""Historical V3 static contracts projected from the reviewed V4.5 delta; current runtime has separate gates."""
from v45_approved_delta import before_v45
from pathlib import Path

root = Path(__file__).resolve().parents[1]
nav = before_v45("src/NavigationManager.cpp",(root / "src" / "NavigationManager.cpp").read_text())
html = (root / "src" / "WebDashboardAssets.cpp").read_text()

def body(name, following):
    return nav.split("void NavigationManager::" + name, 1)[1].split(
        "void NavigationManager::" + following, 1
    )[0]

initial = body("beginV2Escape(", "selectRecoveryDirection(")
assert "reuseScan && scanSampleTrusted" in initial
assert "selectRecoveryDirection(now)" in initial
assert initial.count("beginScan(ScanPurpose::EscapeV2, now)") == 1

post_turn = body("finishRecoveryAfterSettle(", "enterInvalidStop(")
assert "beginRecoveryFrontCheck(now)" in post_turn
assert "beginScan(ScanPurpose::EscapeV2" not in post_turn

front = body("updateRecoveryFrontCheck(", "beginRecoveryIrBackoff(")
assert "recoveryFrontClear(reading, RobotConfig::RECOVERY_FRONT_EXIT_CM)" in front
assert "beginRecoverySideCheck(now)" in front
assert "_scanPurpose = ScanPurpose::Obstacle;" in front
assert "_motors.forward()" not in front
assert "if (_v2.watchdogExhausted() || !_v2.hasCommittedSide()) { beginScan(ScanPurpose::EscapeV2,now); return; }" in front
assert "front.state == UltrasonicResultState::ClearNoEcho" in front
assert "setPhase(AutoPhase::Idle, now);" in front
normal_front = body("updateFrontValidation(", "updateForward(")
assert "front.state == UltrasonicResultState::ClearNoEcho" in normal_front
assert "_motors.forward();" in normal_front
normal_rank = nav.split("bool NavigationManager::selectClearestSide(", 1)[1].split(
    "void NavigationManager::beginSpinTurn(", 1
)[0]
assert "scanPathClearance(_leftScan)" in normal_rank
assert "scanPathClearance(_rightScan)" in normal_rank
assert "scanSampleTrusted(_leftScan)" in normal_rank
assert "scanSampleTrusted(_rightScan)" in normal_rank
scan_clearance = nav.split("float NavigationManager::scanPathClearance(", 1)[1].split(
    "const char* NavigationManager::ultrasonicStateName(", 1
)[0]
assert "UltrasonicResultState::ClearNoEcho" in scan_clearance
assert "RobotConfig::ULTRASONIC_MAX_CM + 1.0f" in scan_clearance

active = nav.split("void NavigationManager::updateAuto()", 1)[1].split(
    "void NavigationManager::resetRecoveryLatchIfClear()", 1
)[0]
assert "_v2.hasCommittedSide() && currentIrZone() != IrZone::Clear" in active
assert "_phase != AutoPhase::RecoveryIrBackoff" in active
ir_branch = active.split("if (!_v2.watchdogExhausted() && _v2.hasCommittedSide() && currentIrZone()", 1)[1].split(
    "else if (!_environmentalRecoveryActive && handleIrArbitration(now))", 1
)[0]
assert "_motors.stop();" in ir_branch
assert "beginRecoveryIrBackoff(now);" in ir_branch
assert "case AutoPhase::RecoveryIrBackoff:" in active
assert "updateRecoveryIrBackoff(now);" in active
assert "else if (!_environmentalRecoveryActive && handleIrArbitration(now))" in active
assert "case AutoPhase::OperatorStop:" in active

side_result = body("finishRecoverySideCheck(", "beginRecoveryFrontCheck(")
assert "currentIrZone() != IrZone::Clear" in side_result
assert "beginRecoveryIrBackoff(now);" in side_result
assert "reading.cm <= RobotConfig::RECOVERY_HARD_VETO_CM" in side_result
assert "beginRecoveryIrBackoff(now, true);" in side_result
assert "recoveryPathSafe(reading, RobotConfig::RECOVERY_HARD_VETO_CM)" in side_result
assert "_v2.continueTurn();" in side_result
assert "beginV2Turn(_v2.committedLeft(), now)" in side_result
assert "_recoverySideRechecks == 0" in side_result
assert "enterV2TrappedHold" in side_result

backoff_start = body("beginRecoveryIrBackoff(", "updateRecoveryIrBackoff(")
assert "_motors.stop();" in backoff_start
assert "_turnUsingGyro = false;" in backoff_start
assert "_relativeTurnDeg = 0.0f;" in backoff_start
assert "_motors.backwardWithSpeedCap(RobotConfig::RECOVERY_BACKOFF_MAX_SPEED_PERCENT);" in backoff_start
assert "_recoveryIrBackoff.pulseCommanded(millis());" in backoff_start
backoff_update = body("updateRecoveryIrBackoff(", "v2ImuReady(")
assert "currentIrZone() != IrZone::Clear" in backoff_update
assert "case RecoveryIrBackoffPolicy::Action::Stop:" in backoff_update
assert "case RecoveryIrBackoffPolicy::Action::FrontCheck:" in backoff_update
assert "beginRecoveryFrontCheck(now);" in backoff_update
assert "case RecoveryIrBackoffPolicy::Action::Failed:" in backoff_update
assert "enterV2TrappedHold" in backoff_update
assert "beginScan(" not in backoff_start + backoff_update
assert "delay(" not in backoff_start + backoff_update
turn = body("beginV2Turn(", "enterV2TrappedHold(")
assert "currentIrZone() != IrZone::Clear" in turn
assert "beginRecoveryIrBackoff(now);" in turn
assert "_v2.startTurn(RobotConfig::RECOVERY_MAX_TURNS)" in turn
assert "RobotConfig::RECOVERY_TURN_TARGET_DEG" in turn

operator = body("notifyOperatorStop(", "cancelForModeChange(")
assert "_motors.stop();" in operator and "_v2.abort();" in operator
assert "_recoveryIrBackoff.resetMission();" in operator
mode = nav.split("void NavigationManager::cancelForModeChange()", 1)[1].split(
    "bool NavigationManager::environmentalRecoveryInProgress()", 1
)[0]
assert "_motors.stop();" in mode and "_v2.rearm();" in mode
assert "_recoveryIrBackoff.resetMission();" in mode
assert "_scanPurpose = ScanPurpose::Obstacle;" in mode
assert "cancelForModeChange();" in nav.split("bool NavigationManager::handleOperatorInterruption()", 1)[1].split("bool NavigationManager::handleCriticalSafety(", 1)[0]
assert "beginScan(ScanPurpose::EnvironmentalRecovery, now)" in nav
assert "_v2.pause(now); // hazard preemption preserves episode budgets" in active
assert "_recoveryIrBackoff.cancelPulse();" in active

side = body("beginRecoverySideCheck(", "finishRecoverySideCheck(")
assert "_v2.committedLeft() ? 0 : 2" in side
assert "commandCurrentScanDirection(now)" in side

control = html.split('id="control" role="tabpanel"', 1)[1].split(
    'id="sensors" role="tabpanel"', 1
)[0]
ordered = (
    'id="autoBtn"', 'id="manualBtn"', 'id="turnModeBtn"',
    'id="motorSpeed"', 'id="overrideBtn"', 'id="enableAlarmBtn"',
    'id="muteAlarmBtn"', 'id="dpad"', 'id="sensorHeadLeftBtn"',
    'id="sensorHeadFrontBtn"', 'id="sensorHeadRightBtn"',
    'id="motor"', 'id="radar"'
)
positions = [control.index(item) for item in ordered]
assert positions == sorted(positions)
assert html.count('id="enableAlarmBtn"') == 1
assert html.count('id="muteAlarmBtn"') == 1
print("recovery V3.1 source and Control order: PASS")
