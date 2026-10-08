"""Historical V3 static contracts projected from the reviewed V4.5 delta; current runtime has separate gates."""
from v45_approved_delta import before_v45
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = before_v45("src/main.cpp",(root / "src/main.cpp").read_text())
safety = before_v45("src/SafetyManager.cpp",(root / "src/SafetyManager.cpp").read_text())
nav = before_v45("src/NavigationManager.cpp",(root / "src/NavigationManager.cpp").read_text())
sensor = before_v45("src/SensorManager.cpp",(root / "src/SensorManager.cpp").read_text())
html = before_v45("src/WebDashboardAssets.cpp",(root / "src/WebDashboardAssets.cpp").read_text())
config = before_v45("include/RobotConfig.h",(root / "include/RobotConfig.h").read_text())

def section(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]

assert "MANUAL_FORWARD_STOP_CM = 20.0f" in config
assert "MANUAL_FORWARD_CLEAR_CM = 25.0f" in config
assert "SONAR_FAST_REACQUIRE_WINDOW_MS = 1000" in config
override_action = section(main, "bool onManualOverrideAction(", "void onDashboardCommandAccepted(")
assert "overrideSnapshot.flameSampleValid" in override_action
assert "overrideSnapshot.ultrasonicState" not in override_action
service = section(main, "bool serviceLoop() {", "void refreshManualForwardSonarGuard(")
assert "const bool sensorFault = !guardSnapshot.flameSampleValid;" in service
assert "guardSnapshot.ultrasonicState == UltrasonicResultState::SensorFault" not in service
manual = section(main, "bool handleManualControl() {", "void triggerObstacleWarning()")
assert "gate.sensorFault = manualOverride.enabled() && !gateSnapshot.flameSampleValid;" in manual
assert "refreshManualForwardSonarGuard(gateSnapshot, millis());" in manual
assert "gate.forwardObstacle = manualForwardBlockedByUltrasonic ||" in manual
assert "safety.blocksManualForwardMotion(true)" in manual
assert "gateSnapshot.distanceValid" not in manual
guard = section(main, "void refreshManualForwardSonarGuard(", "bool handleManualControl() {")
assert "snapshot.distanceFacingFront" in guard
assert "snapshot.ultrasonicState == UltrasonicResultState::DistanceValid" in guard
assert "snapshot.ultrasonicState == UltrasonicResultState::ClearNoEcho" in guard
assert "snapshot.distanceSampleMs <= RobotConfig::ULTRASONIC_STALE_MS" in guard
assert "manualForwardSonarBlocked(" in guard
manual_update = section(main, "void updateManualMode() {", "void setup()")
assert "manualForwardBlockedByUltrasonic &&" in manual_update
assert "motors.getState() == MotorState::Forward" in manual_update
assert "MANUAL_ULTRASONIC_FAULT_STOP" not in manual_update
manual_safety = section(safety, "bool SafetyManager::blocksManualForwardMotion(", "bool SafetyManager::stateChanged()")
assert "_leftIrObstacle || _rightIrObstacle" in manual_safety
assert "_ultrasonicWarning" not in manual_safety
assert "_ultrasonicCaution" not in manual_safety
assert "blocksMotion(allowFlameGas)" in manual_safety

auto = section(nav, "void NavigationManager::updateAuto()", "void NavigationManager::resetRecoveryLatchIfClear()")
assert auto.index("handleOperatorInterruption()") < auto.index("handleCriticalSafety(now)")
assert auto.index("handleCriticalSafety(now)") < auto.index("beginSonarReacquire(now);")
assert auto.index("beginSonarReacquire(now);") < auto.index("handleIrArbitration(now)")
assert "sonarState == UltrasonicResultState::SensorFault" in auto
assert "sonarState == UltrasonicResultState::Stale" in auto
assert "if (_phase == AutoPhase::SonarReacquire)" in auto
assert "updateSonarReacquire(now);" in auto
reacquire_start = section(nav, "void NavigationManager::beginSonarReacquire(", "void NavigationManager::updateSonarReacquire(")
for required in ("_motors.stop();", "_servoScanner.center();", "_turnUsingGyro = false;",
                 "_relativeTurnDeg = 0.0f;", "_recoveryIrBackoff.cancelPulse();",
                 "_v2.pause(now);", "_sensors.setDistanceFacingFront(true);"):
    assert required in reacquire_start
reacquire = section(nav, "void NavigationManager::updateSonarReacquire(", "void NavigationManager::enterEnvironmentalHazard(")
for required in ("_motors.stop();", "front.facingFront",
                 "front.state == UltrasonicResultState::ClearNoEcho",
                 "front.state == UltrasonicResultState::DistanceValid",
                 "_sonarReacquire.observe(front.sequence, healthy)",
                 "updateFrontValidation(now);", "SONAR_FAST_REACQUIRE_WINDOW_MS"):
    assert required in reacquire
assert "if (_environmentalRecoveryActive)" in reacquire
assert reacquire.count("beginScan(") == 1
assert "beginScan(ScanPurpose::EnvironmentalRecovery, now)" in reacquire
for forbidden in ( "_motors.spin", "_motors.backward(", "delay(", "enterV2TrappedHold("):
    assert forbidden not in reacquire_start + reacquire
assert "lastUltrasonicMs >= RobotConfig::ULTRASONIC_INTERVAL_MS" in sensor
backoff = section(nav, "void NavigationManager::updateRecoveryIrBackoff(", "bool NavigationManager::v2ImuReady(")
assert "beginSonarReacquire(now);" in backoff
assert "ESCAPE_BACKOFF_SENSOR_INVALID" not in backoff

assert "d.ultrasonicState === 'CLEAR_NO_ECHO' ? 'OPEN'" in html
assert "d.ultrasonicState === 'SENSOR_FAULT' ? 'FAULT'" in html
assert "d.ultrasonicState === 'STALE' ? 'STALE'" in html
assert "setBound('distanceValidity', distanceBadge)" in html
assert "NOT VALID" not in html
print("V3.2 Manual, reacquisition, and dashboard source contract: PASS")
