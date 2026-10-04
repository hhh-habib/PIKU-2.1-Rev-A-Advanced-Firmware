"""Execute actual Standalone accepted-command and mode callbacks."""
from pathlib import Path
import subprocess,unittest
import test_state_collisions as native
ROOT=Path(__file__).resolve().parents[1]
class CallbackCollisions(unittest.TestCase):
 def test_stop_release_mode_fence_later_dispatch(self):
  native.StateCollisions.setUpClass()
  try:
   w=native.StateCollisions.work
   source=(ROOT/'src/main.cpp').read_text()
   callbacks="\n".join(native.declaration(source,m) for m in ["void onDashboardCommandAccepted(","void onDashboardModeAccepted("])
   prefix=r'''#include "Arduino.h"
#include <cassert>
#include "ManualOverridePolicy.h"
struct Motors {int moves=0;bool stopped=true;void stop(){stopped=true;}} motors;
struct Dashboard {String mode="MANUAL",pending="";String getControlMode(){return mode;}void setPendingCommand(const String& s){pending=s;}void markCommandApplied(const char*,unsigned long,const char*){}void markStopApplied(const char*){}} dashboard;
struct Data {String motorState,navigationDecision,manualMotionInhibitReason;} dashboardData;
struct Nav {int aborts=0;void notifyOperatorStop(){++aborts;}void cancelForModeChange(){}} navigation;
struct Sensors{int snapshot(){return 0;}void updateControlSensors(){}} sensors;
struct Safety{void update(int){}} safety;
enum class ManualSensorHeadState{Idle};struct Head{ManualSensorHeadState state=ManualSensorHeadState::Idle;} manualSensorHeadOperation;
ManualOverridePolicy manualOverride;String currentServoPosition="FRONT",manualMotorDisplayState;bool controlStopThisPass=false;
void stopMotors(){motors.stop();}
void revokeManualOverride(const char*){manualOverride.revoke();}
bool handleManualControl(){++motors.moves;motors.stopped=false;return true;}
'''
   main=r'''int main(){
for(const auto& action:{"STOP","RELEASE"}){
 controlStopThisPass=false;motors.moves=0;dashboard.pending=action;onDashboardCommandAccepted(action,1,100);
 dashboard.pending="FORWARD";onDashboardCommandAccepted("FORWARD",2,100);
 assert(motors.stopped&&motors.moves==0&&dashboard.pending.empty());
 controlStopThisPass=false;onDashboardCommandAccepted("FORWARD",3,101);assert(motors.moves==1);
}
controlStopThisPass=false;motors.moves=0;onDashboardModeAccepted("MANUAL");onDashboardCommandAccepted("BACKWARD",4,102);assert(motors.stopped&&motors.moves==0);
dashboard.mode="AUTO";controlStopThisPass=false;onDashboardCommandAccepted("STOP",5,103);assert(navigation.aborts==1);
}'''
   file=w/'callbacks.cpp';file.write_text(prefix+callbacks+main);exe=w/'callbacks.exe'
   vcvars=Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
   args=['cl','/nologo','/std:c++17','/EHsc','/W4','/I'+str(w),'/I'+str(ROOT/'include'),str(file),'/Fe:'+str(exe)]
   batch=w/'callbacks.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n')
   built=subprocess.run('cmd /d /s /c ""'+str(batch)+'""',cwd=w,capture_output=True,text=True);self.assertEqual(built.returncode,0,built.stdout+built.stderr)
   run=subprocess.run([str(exe)],cwd=w,capture_output=True,text=True);self.assertEqual(run.returncode,0,run.stdout+run.stderr)
  finally:native.StateCollisions.tearDownClass()
