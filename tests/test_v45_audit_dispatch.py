"""Actual Standalone service, manual handler, callbacks, outer loop, sensors and safety."""
from pathlib import Path
import subprocess, unittest
import test_v45_producers as producers
import test_state_collisions as native
ROOT=Path(__file__).resolve().parents[1]

class AuditDispatch(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  producers.Producers.setUpClass();w=cls.work=producers.Producers.work
  source=(ROOT/'src/main.cpp').read_text()
  arduino=(w/'Arduino.h').read_text().replace('inline int digitalRead(int){return digitalValue;}', 'inline int digitalRead(int p){return p=='+str(__import__('re').search(r'FLAME_DIGITAL = (\d+)',(ROOT/'include/PinConfig.h').read_text()).group(1))+'?LOW:HIGH;}')
  (w/'Arduino.h').write_text(arduino)

  doubles=(w/'CollisionDoubles.h').read_text()
  doubles=doubles.replace(native.declaration(doubles,'class SafetyManager'),'#include "SafetyUnderTest.h"')
  doubles=doubles.replace('void stop(){motion=MotorState::Stopped;}', 'String getStateName()const{return motion==MotorState::Stopped?"STOP":motion==MotorState::Forward?"FORWARD":motion==MotorState::Backward?"BACKWARD":"TURN";}MotorState getState()const{return motion;}void leftTurn(){motion=MotorState::LeftTurn;}void rightTurn(){motion=MotorState::RightTurn;}void setSpeedPercent(int n){speed=n;}void stop(){motion=MotorState::Stopped;}')
  doubles=doubles.replace('class ImuManager {public:', 'class ImuManager {public: void update(){}')
  doubles=doubles.replace('class WebDashboard {public:', '''class WebDashboard {public:
 bool activity=false, flood=false;unsigned long lastCommandMs=0;
 bool live=true;bool sessionAlive()const{return live;}unsigned long pendingCommandSequence()const{return 1;}unsigned long lastCommandReceivedMs()const{return lastCommandMs;}
 void loop(){if(flood){activity=true;pending="BACKWARD";lastCommandMs=millis();}extern void deliverWeb();deliverWeb();}
 bool consumeControlActivity(){bool v=activity;activity=false;return v;}bool consumeSpeedPercent(int&){return false;}
 void updateBackground(){}bool isAlarmEnabled(){return true;}bool isAlarmMuted(){return false;}bool consumeAlarmTestRequest(){return false;}
 String getTurnMode(){return "SPIN";}void setManualOverrideState(bool){}void invalidateControlSession(){}
 void markCommandApplied(const String&,unsigned long,const String&){}void markStopApplied(const String&){}
''')
  (w/'CollisionDoubles.h').write_text(doubles)
  for suffix in ('h','cpp'):
   (w/('SafetyUnderTest.'+suffix)).write_text((ROOT/('include' if suffix=='h' else 'src')/('SafetyManager.'+suffix)).read_text().replace('SafetyManager.h','SafetyUnderTest.h'))
  (w/'DriveCommandHandler.h').write_text((ROOT/'include/DriveCommandHandler.h').read_text().replace('#include "MotorController.h"','#include "CollisionDoubles.h"'))
  (w/'DriveCommandHandler.cpp').write_text((ROOT/'src/DriveCommandHandler.cpp').read_text())
  enums=native.declaration((ROOT/'include/WebDashboard.h').read_text(),'enum class ManualSensorHeadCommand')+'\n'
  for m in ['enum class ManualSensorHeadState','struct ManualSensorHeadOperation','struct ManualInspectionState']:enums+=native.declaration(source,m)+'\n'
  prefix='''#include "NavigationUnderTest.h"
#include "DriveCommandHandler.h"
#include "ManualMotionGate.h"
#include "ManualOverridePolicy.h"
#include <cassert>
'''+enums+'''
MotorController motors;SensorManager sensors;ImuManager imu;ServoScanner servoScanner;SafetyManager safety;WebDashboard dashboard;DashboardData dashboardData;
DriveCommandHandler driveCommands(motors);ManualOverridePolicy manualOverride;
ManualSensorHeadOperation manualSensorHeadOperation;ManualInspectionState manualInspectionState;
String currentServoPosition="FRONT",manualMotorDisplayState="",lastControlMode="MANUAL",webAction="";
bool controlStopThisPass=false,criticalStopLatched=false,criticalStopRequiresManualRearm=false,manualMovementActive=false,manualForwardBlockedByUltrasonic=false,allowSlowOutputsThisPass=true;
unsigned long criticalStopModeCommandCounter=0,obstacleWarningUntilMs=0,lastServiceLoopStartedMs=0;
NavigationManager navigation{motors,sensors,imu,servoScanner,safety,dashboard,dashboardData,currentServoPosition,nullptr,nullptr,nullptr};
enum class BuzzerMode{Critical,Hazard,Caution,Off};struct Buzzer{void setEnabled(bool){}void setMuted(bool){}void requestTest(){}void setMode(BuzzerMode){}void update(){}}buzzer;
struct Radio{void update(uint8_t){}}radio;struct Trace{void service(const DashboardData&){}}navTrace;
void publishCurrentTelemetry(bool){}void syncDashboardState(const String&){}void clearManualInspectionState(){}void releaseManualServo(){}void delay(unsigned long){}
bool beginManualSensorHeadOperation(ManualSensorHeadCommand,bool){return false;}bool updateManualInspectionWarning(){return false;}uint8_t currentEnvironmentalAlarmMask(){return 0;}
void revokeManualOverride(const char*){manualOverride.revoke();}
bool handleManualControl();void updateManualMode(){handleManualControl();}
void onDashboardCommandAccepted(const String&,unsigned long,unsigned long);void onDashboardModeAccepted(const String&);
void deliverWeb(){if(webAction.empty())return;const auto a=webAction;webAction="";dashboard.activity=true;dashboard.lastCommandMs=testNow;dashboard.pending=a;onDashboardCommandAccepted(a,1,testNow);}
'''
  bodies='\n'.join(native.declaration(source,m) for m in ['void stopMotors() {','bool manualFrontTurnVeto(','void refreshManualForwardSonarGuard(','bool handleManualControl() {','void onDashboardCommandAccepted(','void onDashboardModeAccepted(','bool serviceLoop() {','void loop() {'])
  fixture=(ROOT/'tests/native_v45_standalone_dispatch.cpp').read_text()
  (w/'dispatch.cpp').write_text(prefix+bodies+"\n"+fixture)
  cls.exe=w/'dispatch.exe';vcvars=Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
  args=['cl','/nologo','/std:c++17','/EHsc','/W4','/I'+str(w),'/I'+str(ROOT/'include'),str(w/'dispatch.cpp'),str(w/'NavigationUnderTest.cpp'),str(w/'SensorManager.cpp'),str(w/'SafetyUnderTest.cpp'),str(w/'DriveCommandHandler.cpp'),'/Fe:'+str(cls.exe)]
  batch=w/'dispatch.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n')
  result=subprocess.run('cmd /d /s /c ""'+str(batch)+'""',cwd=w,capture_output=True,text=True,timeout=90)
  if result.returncode:raise RuntimeError(result.stdout+result.stderr)
 @classmethod
 def tearDownClass(cls):producers.Producers.tearDownClass()
 def case(self,name):
  result=subprocess.run([str(self.exe),name],cwd=self.work,capture_output=True,text=True,timeout=20)
  self.assertEqual(result.returncode,0,result.stdout+result.stderr)
 def test_a03_actual_environment_reads_under_web_both_service_calls(self):self.case('load')
 def test_a04_actual_temperature_manual_rearm_auto_and_later_hazard(self):self.case('rearm')
 def test_n14_release_stop_front_exit_same_pass(self):self.case('race')
 def test_owner_correction_actual_manual_producers_each_unavailable_profile(self):self.case('unavailable_manual')
 def test_session_loss_in_auto_and_recovery_preserves_stop(self):self.case('wifi_auto')
 def test_v46_actual_outer_manual_auto_continuity_and_genuine_stop(self):self.case('v46_mode')
