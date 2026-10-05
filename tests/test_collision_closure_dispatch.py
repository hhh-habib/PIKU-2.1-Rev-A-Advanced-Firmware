"""Standalone production callback + outer dispatcher + NavigationManager, delivery seam."""
from pathlib import Path
import subprocess, unittest
import test_state_collisions as states
ROOT=Path(__file__).resolve().parents[1]
class ClosureDispatch(unittest.TestCase):
 def test_web_release_environmental_decision_same_pass(self):
  states.StateCollisions.setUpClass()
  try:
   w=states.StateCollisions.work
   doubles=(w/'CollisionDoubles.h').read_text()
   doubles=doubles.replace('class SafetyManager {public:','class SafetyManager {public: bool blocksMotion(bool exempt=false)const{return critical||(hazard&&!exempt);}')
   doubles=doubles.replace('class WebDashboard {public:','class WebDashboard {public: bool sessionAlive()const{return false;}void markCommandApplied(const char*,unsigned long,const char*){}void markStopApplied(const char*){}')
   doubles=doubles.replace('class SensorManager { public:','class SensorManager { public: void updateControlSensors(){}')
   doubles=doubles.replace('bool isCritical()const{return critical||hazard;}','bool isCritical()const{return critical||hazard;}void update(const SensorSnapshot&){}')
   (w/'CollisionDoubles.h').write_text(doubles)
   source=(ROOT/'src/main.cpp').read_text()
   enums='\n'.join(states.declaration(source,m) for m in ['enum class ManualSensorHeadState','struct ManualSensorHeadOperation','struct ManualInspectionState'])
   enums=states.declaration((ROOT/'include/WebDashboard.h').read_text(),'enum class ManualSensorHeadCommand')+'\n'+enums
   prefix='#include "NavigationUnderTest.h"\n#include "ManualOverridePolicy.h"\n#include <cassert>\n'+enums+r'''
MotorController motors;SensorManager sensors;ImuManager imu;ServoScanner servoScanner;SafetyManager safety;WebDashboard dashboard;DashboardData dashboardData;
String currentServoPosition="FRONT",manualMotorDisplayState,lastControlMode="AUTO",webAction="RELEASE";
bool controlStopThisPass=false,criticalStopLatched=true,criticalStopRequiresManualRearm=false,manualMovementActive=false;
unsigned long criticalStopModeCommandCounter=0;
ManualSensorHeadOperation manualSensorHeadOperation;ManualInspectionState manualInspectionState;ManualOverridePolicy manualOverride;
NavigationManager navigation{motors,sensors,imu,servoScanner,safety,dashboard,dashboardData,currentServoPosition,nullptr,nullptr,nullptr};
void publishCurrentTelemetry(bool){}void syncDashboardState(const String&){}void clearManualInspectionState(){}void releaseManualServo(){}void updateManualMode(){}void delay(unsigned long){}
bool beginManualSensorHeadOperation(ManualSensorHeadCommand,bool){return false;}bool handleManualControl(){return false;}
void revokeManualOverride(const char*){manualOverride.revoke();}
void onDashboardCommandAccepted(const String&,unsigned long,unsigned long);void onDashboardModeAccepted(const String&);
bool serviceLoop(){if(!webAction.empty()){String a=webAction;webAction="";if(a=="MODE"){dashboard.mode="MANUAL";++dashboard.modeCounter;onDashboardModeAccepted("MANUAL");}else{dashboard.pending=a;onDashboardCommandAccepted(a,1,testNow);}}return false;}
'''
   bodies='\n'.join(states.declaration(source,m) for m in ['void stopMotors() {','void onDashboardCommandAccepted(','void onDashboardModeAccepted(','void loop() {'])
   main=r'''
int main(){for(unsigned point=0;point<3;++point)for(unsigned action=0;action<3;++action){navigation.cancelForModeChange();dashboard.mode=lastControlMode="AUTO";dashboard.pending="";criticalStopLatched=true;criticalStopModeCommandCounter=dashboard.modeCounter;criticalStopRequiresManualRearm=false;controlStopThisPass=false;webAction=action==0?"RELEASE":action==1?"STOP":"MODE";sensors.feed(UltrasonicResultState::DistanceValid,60);navigation.enterEnvironmentalHazard(testNow);navigation.beginScan(NavigationManager::ScanPurpose::EnvironmentalRecovery,testNow);
for(unsigned i=0;i<100&&navigation._phase!=NavigationManager::AutoPhase::RecenterSettle;++i){testNow+=100;sensors.feed(UltrasonicResultState::DistanceValid,navigation._scanDirectionIndex==1?10:80);sensors.sample.facingFront=navigation._scanDirectionIndex==1;imu.data.initialized=imu.data.healthy=true;imu.data.sampleMs=imu.data.lastSuccessMs=testNow;navigation.updateAuto();}
assert(navigation._phase==NavigationManager::AutoPhase::RecenterSettle);testNow+=RobotConfig::AUTO_SCAN_RECENTER_SETTLE_MS;if(point){imu.data.sampleMs=imu.data.lastSuccessMs=testNow;navigation.updateAuto();assert(navigation._phase==NavigationManager::AutoPhase::Turn);}if(point==2){navigation.completeSpin("TEST_IMU_TARGET",testNow);testNow+=RobotConfig::AUTO_POST_TURN_SETTLE_MS;}const auto turns=motors.spins;loop();
assert(motors.motion==MotorState::Stopped&&motors.spins==turns);if(action==0)assert(navigation.environmentalRecoveryInProgress()&&!navigation._operatorStopLatched);else assert(!navigation.environmentalRecoveryInProgress());}

}
'''
   file=w/'dispatch.cpp';file.write_text(prefix+bodies+main);exe=w/'dispatch.exe'
   vcvars=Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
   args=['cl','/nologo','/std:c++17','/EHsc','/W4','/I'+str(w),'/I'+str(ROOT/'include'),str(file),str(w/'NavigationUnderTest.cpp'),'/Fe:'+str(exe)]
   batch=w/'dispatch.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n')
   build=subprocess.run('cmd /d /s /c ""'+str(batch)+'""',cwd=w,capture_output=True,text=True);self.assertEqual(build.returncode,0,build.stdout+build.stderr)
   run=subprocess.run([str(exe)],cwd=w,capture_output=True,text=True);self.assertEqual(run.returncode,0,run.stdout+run.stderr)
  finally:states.StateCollisions.tearDownClass()
if __name__=='__main__':unittest.main()
