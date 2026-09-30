from pathlib import Path

root = Path(__file__).resolve().parents[1]
nav = (root / "src" / "NavigationManager.cpp").read_text()
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

front = body("updateRecoveryFrontCheck(", "v2ImuReady(")
assert "recoveryFrontClear(reading, RobotConfig::RECOVERY_FRONT_EXIT_CM)" in front
assert "beginRecoverySideCheck(now)" in front
assert "_scanPurpose = ScanPurpose::Obstacle;" in front
assert "_motors.forward()" not in front
assert "beginScan(ScanPurpose::EscapeV2" not in front

active = nav.split("void NavigationManager::updateAuto()", 1)[1].split(
    "void NavigationManager::resetRecoveryLatchIfClear()", 1
)[0]
assert "_phase == AutoPhase::Turn && selectedIr" in active
assert "_motors.stop();" in active.split(
    "if (_phase == AutoPhase::Turn && selectedIr)", 1
)[1].split("beginRecoverySideCheck(now)", 1)[0]
assert "else if (handleIrArbitration(now))" in active
assert "case AutoPhase::OperatorStop:" in active

side_result = body("finishRecoverySideCheck(", "beginRecoveryFrontCheck(")
assert "scanSampleTrusted(side) && !selectedIr" in side_result
assert "recoveryPathSafe(recoveryReading(side), RobotConfig::RECOVERY_HARD_VETO_CM)" in side_result
assert "_recoverySideRechecks == 0" in side_result
assert "enterV2TrappedHold" in side_result

operator = body("notifyOperatorStop(", "cancelForModeChange(")
assert "_motors.stop();" in operator and "_v2.abort();" in operator
mode = nav.split("void NavigationManager::cancelForModeChange()", 1)[1].split(
    "bool NavigationManager::environmentalRecoveryInProgress()", 1
)[0]
assert "_motors.stop();" in mode and "_v2.rearm();" in mode
assert "_scanPurpose = ScanPurpose::Obstacle;" in mode
assert "cancelForModeChange();" in nav.split("bool NavigationManager::handleOperatorInterruption()", 1)[1].split("bool NavigationManager::handleCriticalSafety(", 1)[0]
assert "beginScan(ScanPurpose::EnvironmentalRecovery, now)" in nav

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
print("recovery source and Control order: PASS")
