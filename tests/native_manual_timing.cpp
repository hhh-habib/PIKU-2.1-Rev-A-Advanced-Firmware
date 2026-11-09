#include "NavigationUnderTest.h"
#include "ManualOverridePolicy.h"
#include <cassert>

// Execute the actual leading admission/deadman/safety/RELEASE portion of
// handleManualControl. Downstream drive and sensor gates have separate tests.
struct ManualDashboard {
  String mode="MANUAL",pending="",reason="";unsigned long received=0,sequence=0;
  String getPendingCommand(){return pending;}
  unsigned long pendingCommandSequence(){return sequence;}
  unsigned long lastCommandReceivedMs(){return received;}
  void setPendingCommand(const String& value){pending=value;}
  String getControlMode(){return mode;}
  bool sessionAlive(){return true;}
  void markCommandApplied(const char*,unsigned long,const char* value){reason=value;}
  void markStopApplied(const char* value){reason=value;}
} dashboard;
struct ManualMotors {bool stopped=true;void stop(){stopped=true;}MotorState getState(){return stopped?MotorState::Stopped:MotorState::Forward;}} motors;
struct ManualSafety {bool critical=false;bool blocksMotion(bool){return critical;}} safety;
DashboardData dashboardData;
ManualOverridePolicy manualOverride;
String manualMotorDisplayState;
bool manualMovementActive=false,controlStopThisPass=false;
void syncDashboardState(const String&){}
// ACTUAL_MANUAL_TIMING_BODIES
void fresh(const char* direction){dashboard.pending=direction;dashboard.received=testNow;++dashboard.sequence;}
int main(int argc,char** argv){
  assert(argc==2);const String name=argv[1];testNow=10000;
  static_assert(RobotConfig::MANUAL_COMMAND_TIMEOUT_MS==750,"firmware deadman must stay 750 ms");
  if(name=="ten_seconds"){
    for(unsigned cycle=0;cycle<3;++cycle){
      for(const auto* direction:{"FORWARD","LEFT","FORWARD"}){
        for(unsigned held=0;held<10000;held+=250){fresh(direction);assert(handleManualControl());motors.stopped=false;manualMovementActive=true;testNow+=249;assert(handleManualControl()&&!independentDeadman());++testNow;}
        fresh("RELEASE");motors.stopped=false;assert(handleManualControl()&&motors.stopped&&dashboard.mode=="MANUAL");
      }
    }
  }
  if(name=="expiry_and_release"){
    fresh("FORWARD");motors.stopped=false;testNow+=749;assert(handleManualControl());
    ++testNow;assert(!handleManualControl()&&motors.stopped&&dashboard.pending.empty()&&dashboard.reason=="MANUAL_DEADMAN"&&dashboard.mode=="MANUAL");
    fresh("LEFT");assert(handleManualControl());fresh("RELEASE");testNow+=10000;motors.stopped=false;assert(handleManualControl()&&motors.stopped&&dashboard.mode=="MANUAL");
    fresh("FORWARD");assert(handleManualControl());
  }
  if(name=="stop_and_critical"){
    fresh("FORWARD");controlStopThisPass=true;assert(!handleManualControl()&&dashboard.pending.empty());
    controlStopThisPass=false;safety.critical=true;fresh("FORWARD");motors.stopped=false;
    assert(!handleManualControl()&&motors.stopped&&dashboard.reason=="SAFETY_INTERLOCK");
  }
  if(name=="independent_deadman"){
    fresh("FORWARD");dashboard.pending="";motors.stopped=false;manualMovementActive=true;
    assert(manualOverride.enable(true,true,false,true,true));
    testNow+=749;assert(!independentDeadman()&&!motors.stopped);
    ++testNow;assert(independentDeadman()&&motors.stopped&&!manualMovementActive&&controlStopThisPass&&dashboard.reason=="MANUAL_DEADMAN");
    assert(dashboard.mode=="MANUAL"&&manualOverride.enabled());
    testNow+=10000;assert(!independentDeadman()&&motors.stopped);
  }
}
