#include "NavigationUnderTest.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#ifdef _MSC_VER
#include <stdlib.h>
struct RestorationErrors { RestorationErrors(){_set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);} } restorationErrors;
#endif

struct RestoredRig {
 MotorController motors; SensorManager sensors; ImuManager imu; ServoScanner servo;
 SafetyManager safety; WebDashboard dashboard; DashboardData data; String facing="FRONT";
 NavigationManager nav{motors,sensors,imu,servo,safety,dashboard,data,facing,nullptr,nullptr,nullptr};
 float left=80,front=12,right=100; bool open=false,trace=false;
 RestoredRig(){testNow=10000;feed(80);}
 void feed(float cm,bool sonarFront=true,UltrasonicResultState state=UltrasonicResultState::DistanceValid){
  sensors.feed(state,cm);sensors.sample.facingFront=sonarFront;sensors.data.distanceFacingFront=sonarFront;
  imu.data.initialized=imu.data.healthy=true;imu.data.sampleMs=imu.data.lastSuccessMs=testNow;
  imu.data.gyroZ=(motors.motion==MotorState::SpinLeft||motors.motion==MotorState::SpinRight)?2.0f:0.0f;
 }
 void ir(unsigned zone){safety.left=zone&1;safety.right=zone&2;}
 void record(){if(trace)std::printf("%lu,%s,%d,%u,%u,%u,%u,%u,%d,%lu,%u,%u,%.2f\n",testNow,nav.phaseName(),int(motors.motion),motors.forwards,motors.backwards,motors.spins,sensors.requests,servo.commands,servo.angle,nav._phaseStartedMs,nav._v2.sideTurns(),nav._recoveryIrBackoff.missionPulses(),nav._relativeTurnDeg);}
 void update(){nav.updateAuto();record();}
 void step(unsigned dt=50){testNow+=dt;const bool f=servo.angle==90;feed(f?front:(servo.angle==170?left:right),f,open?UltrasonicResultState::ClearNoEcho:UltrasonicResultState::DistanceValid);update();}
 bool scanning()const{return nav._phase==NavigationManager::AutoPhase::ScanServoSettle||nav._phase==NavigationManager::AutoPhase::ScanSampleGap||nav._phase==NavigationManager::AutoPhase::ScanSampleWait||nav._phase==NavigationManager::AutoPhase::RecenterSettle;}
 bool escape()const{return nav._scanPurpose==NavigationManager::ScanPurpose::EscapeV2&&nav._phase==NavigationManager::AutoPhase::Turn;}
 void until(bool (RestoredRig::*condition)()const,unsigned max=400){for(unsigned i=0;i<max&&!(this->*condition)();++i)step();assert((this->*condition)());}
 void startObstacle(){feed(12);update();for(unsigned i=0;i<10&&nav._phase!=NavigationManager::AutoPhase::ReversePrepare;++i){testNow+=50;feed(12);update();}assert(nav._phase==NavigationManager::AutoPhase::ReversePrepare&&motors.motion==MotorState::Stopped);}
 void boxed(float l=20,float r=26){left=l;front=23;right=r;startObstacle();until(&RestoredRig::escape);}
};

void restoredScenario(const char* name,bool trace=false){
 RestoredRig r;r.trace=trace;
 if(!std::strcmp(name,"normal")){r.front=80;r.update();for(unsigned i=0;i<35;++i)r.step();assert(r.motors.forwards>=2&&!r.motors.spins&&!r.motors.backwards);return;}
 if(!std::strcmp(name,"front")||!std::strcmp(name,"left")||!std::strcmp(name,"right")||!std::strcmp(name,"both")||!std::strcmp(name,"clear_single")){
  unsigned zone=!std::strcmp(name,"right")?2:!std::strcmp(name,"both")?3:(!std::strcmp(name,"left")||!std::strcmp(name,"clear_single"))?1:0;
  r.ir(zone);r.front=!std::strcmp(name,"clear_single")?80:12;r.feed(r.front);r.update();assert(r.motors.spins==0);
  for(unsigned i=0;i<20&&!r.scanning();++i)r.step();assert(r.motors.backwards==1&&r.scanning()&&!r.motors.spins);
  for(unsigned i=0;i<100;++i)r.step();assert(r.sensors.requests>=9);return;
 }
 if(!std::strcmp(name,"no_path")){r.left=r.right=10;r.front=80;r.startObstacle();for(unsigned i=0;i<160;++i){r.step();if(r.nav._phase==NavigationManager::AutoPhase::BlockedRetryWait){const auto count=r.motors.backwards,requests=r.sensors.requests;r.step(1199);assert(r.motors.motion==MotorState::Stopped&&r.motors.backwards==count&&r.sensors.requests==requests);r.step(1);assert(r.scanning()&&r.motors.backwards==count);return;}}assert(false);}
 if(!std::strcmp(name,"history")||!std::strcmp(name,"short")){
  r.left=r.right=80;r.startObstacle();for(unsigned i=0;i<500&&!r.escape();++i){if(!std::strcmp(name,"short"))r.front=r.motors.motion==MotorState::Forward?12:80;r.step();}assert(r.escape()&&r.nav._v2.attempts()==1);return;
 }
 if(!std::strcmp(name,"boxed")||!std::strcmp(name,"increment")||!std::strcmp(name,"front_after")){
  r.boxed();assert(r.nav._v2.sideTurns()==1&&r.motors.motion==MotorState::SpinRight);
  if(std::strcmp(name,"boxed")){const auto requests=r.sensors.requests;for(unsigned i=0;i<100&&r.sensors.requests==requests;++i)r.step();assert(r.sensors.requests==requests+1&&r.motors.motion==MotorState::Stopped);}
  return;
 }
 if(!std::strcmp(name,"choice_left")||!std::strcmp(name,"choice_right")||!std::strcmp(name,"tie")||!std::strcmp(name,"single_preference")||!std::strcmp(name,"side20")){
  r.left=!std::strcmp(name,"choice_right")?30:80;r.right=!std::strcmp(name,"choice_left")?30:80;
  if(!std::strcmp(name,"single_preference")){r.left=120;r.right=40;r.ir(1);}
  if(!std::strcmp(name,"side20")){r.left=r.right=20;r.front=80;}
  r.startObstacle();for(unsigned i=0;i<120&&!r.motors.spins&&r.nav._phase!=NavigationManager::AutoPhase::BlockedRetryWait;++i)r.step();
  if(!std::strcmp(name,"side20"))assert(!r.motors.spins&&r.nav._phase==NavigationManager::AutoPhase::BlockedRetryWait);
  else assert(r.motors.motion==((!std::strcmp(name,"choice_right")||!std::strcmp(name,"single_preference"))?MotorState::SpinRight:MotorState::SpinLeft));return;
 }
 if(!std::strcmp(name,"side_update")){r.boxed();r.right=6;const auto pulses=r.motors.backwards;for(unsigned i=0;i<130&&r.motors.backwards==pulses;++i)r.step();assert(r.motors.backwards==pulses+1&&r.motors.motion==MotorState::Backward&&r.nav._v2.sideTurns()==1);return;}
 if(!std::strcmp(name,"normal_fallback")||!std::strcmp(name,"normal_deadline")){
  r.startObstacle();for(unsigned i=0;i<120&&!r.motors.spins;++i){testNow+=50;r.feed(r.servo.angle==90?r.front:(r.servo.angle==170?r.left:r.right),r.servo.angle==90);if(!std::strcmp(name,"normal_fallback"))r.imu.data.healthy=false;r.update();}assert(r.motors.spins==1);
  if(!std::strcmp(name,"normal_fallback")){testNow+=1500;r.feed(r.front);r.imu.data.gyroZ=0;r.update();}else{for(unsigned i=0;i<50;++i){testNow+=50;r.feed(r.front);r.imu.data.gyroZ=0;r.update();}}assert(r.motors.motion==MotorState::Stopped);assert(r.nav._phase==(!std::strcmp(name,"normal_fallback")?NavigationManager::AutoPhase::PostTurnSettle:NavigationManager::AutoPhase::InvalidStop));return;
 }
 if(!std::strcmp(name,"safety_matrix")){
  for(unsigned context=0;context<9;++context)for(unsigned event=0;event<6;++event){RestoredRig q;
   if(context==0){q.front=80;q.update();}
   if(context>=1&&context<=5){q.startObstacle();if(context>=2)q.step(50);if(context>=3)q.step(220);if(context==4)for(unsigned i=0;i<20&&!q.scanning();++i)q.step();if(context==5)for(unsigned i=0;i<120&&!q.motors.spins;++i)q.step();}
   if(context>=6){q.boxed();if(context==7){q.ir(1);q.step();}if(context==8){q.nav.completeSpin("HOST_READY_GYRO",testNow);q.step(300);q.step(120);}}
   const auto moves=q.motors.spins+q.motors.backwards+q.motors.forwards;q.nav._relativeTurnDeg=100;
   if(event==0)q.dashboard.pending="STOP";if(event==1)q.dashboard.mode="MANUAL";if(event==2)q.safety.critical=true;if(event==3)q.safety.hazard=true;if(event==4)q.sensors.data.ultrasonicState=UltrasonicResultState::SensorFault;if(event==5)q.sensors.data.ultrasonicState=UltrasonicResultState::Stale;
   q.update();q.update();assert(q.motors.motion==MotorState::Stopped&&q.motors.spins+q.motors.backwards+q.motors.forwards==moves);
  }return;
 }
 if(!std::strcmp(name,"side4")||!std::strcmp(name,"side6")||!std::strcmp(name,"side7")||!std::strcmp(name,"side_above")){
  float cm=!std::strcmp(name,"side4")?4:!std::strcmp(name,"side6")?6:!std::strcmp(name,"side7")?7:7.1f;r.left=r.right=cm;r.front=23;r.startObstacle();for(unsigned i=0;i<110&&r.nav._v2.state()!=StuckV2State::TrappedHold;++i)r.step();
  if(cm<=7)assert(!r.motors.spins&&r.nav._v2.state()==StuckV2State::TrappedHold);else assert(r.motors.spins>=1);return;
 }
 if(!std::strcmp(name,"ir_turn")){r.boxed();r.step(50);r.ir(1);r.step();assert(r.motors.motion==MotorState::Backward&&r.nav._relativeTurnDeg==0&&r.nav._v2.sideTurns()==1);r.ir(0);r.step(50);assert(r.nav._phase==NavigationManager::AutoPhase::RecoveryFrontSettle&&r.motors.motion==MotorState::Stopped);const auto count=r.sensors.requests;for(unsigned i=0;i<100&&r.motors.spins<2;++i)r.step();assert(r.sensors.requests>=count+4&&r.nav._v2.sideTurns()==2);return;}
 if(!std::strcmp(name,"pulse_limit")){r.boxed();const auto normalReverse=r.motors.backwards;r.ir(3);for(unsigned i=0;i<40&&r.nav._v2.state()!=StuckV2State::TrappedHold;++i)r.step();assert(r.motors.backwards==normalReverse+5&&r.motors.motion==MotorState::Stopped&&r.nav._v2.state()==StuckV2State::TrappedHold);return;}
 if(!std::strcmp(name,"exhaustion")||!std::strcmp(name,"retry")){r.boxed();for(unsigned i=0;i<500&&r.nav._v2.state()!=StuckV2State::TrappedHold;++i)r.step();assert(r.nav._v2.sideTurns()==7&&r.motors.spins==7&&r.nav._v2.state()==StuckV2State::TrappedHold);if(!std::strcmp(name,"retry")){r.step(2499);assert(r.nav._v2.state()==StuckV2State::TrappedHold);r.step(1);assert(r.scanning()&&r.nav._v2.sideTurns()==0&&r.nav._v2.attempts()==2);r.until(&RestoredRig::escape);assert(r.nav._v2.sideTurns()==1);}return;}
 if(!std::strcmp(name,"timeout")){r.boxed();r.step(30000);assert(r.nav._v2.state()==StuckV2State::TrappedHold&&r.motors.motion==MotorState::Stopped);return;}
 if(!std::strcmp(name,"exit")||!std::strcmp(name,"open_exit")){r.boxed();r.front=30;r.open=!std::strcmp(name,"open_exit");for(unsigned i=0;i<80&&r.nav._v2.state()!=StuckV2State::Normal;++i)r.step();assert(r.nav._v2.state()==StuckV2State::Normal&&r.motors.motion==MotorState::Stopped);r.front=80;for(unsigned i=0;i<15;++i)r.step();assert(r.motors.forwards);return;}
 if(!std::strcmp(name,"open")){r.open=true;r.front=80;r.feed(-1,true,UltrasonicResultState::ClearNoEcho);r.update();for(unsigned i=0;i<25;++i)r.step();assert(r.motors.forwards&&!r.motors.spins);return;}
 if(!std::strcmp(name,"invalid")||!std::strcmp(name,"stale")){r.boxed();r.sensors.data.ultrasonicState=!std::strcmp(name,"stale")?UltrasonicResultState::Stale:UltrasonicResultState::SensorFault;r.update();assert(r.nav._phase==NavigationManager::AutoPhase::SonarReacquire&&r.motors.motion==MotorState::Stopped&&r.nav._v2.state()==StuckV2State::Normal);r.step(120);r.step();r.step();assert(r.nav._phase!=NavigationManager::AutoPhase::Turn);return;}
 if(!std::strcmp(name,"stop_reverse")||!std::strcmp(name,"stop_turn")||!std::strcmp(name,"stop_recovery")){
  if(!std::strcmp(name,"stop_recovery"))r.boxed();else{r.startObstacle();if(!std::strcmp(name,"stop_turn")){for(unsigned i=0;i<100&&r.motors.spins==0;++i)r.step();assert(r.motors.spins);}else r.step(50);}
  const auto moves=r.motors.spins+r.motors.forwards+r.motors.backwards;r.dashboard.pending="STOP";r.update();for(unsigned i=0;i<20;++i)r.step();assert(r.motors.motion==MotorState::Stopped&&r.motors.spins+r.motors.forwards+r.motors.backwards==moves);return;
 }
 if(!std::strcmp(name,"mode")){r.boxed();r.dashboard.mode="MANUAL";r.update();assert(r.motors.motion==MotorState::Stopped);r.front=80;r.dashboard.mode="AUTO";++r.dashboard.modeCounter;r.nav.cancelForModeChange();r.update();for(unsigned i=0;i<10;++i)r.step();assert(r.motors.forwards&&r.nav._v2.state()==StuckV2State::Normal);return;}
 if(!std::strcmp(name,"hazard")){r.boxed();r.safety.hazard=true;r.update();assert(r.motors.motion==MotorState::Stopped&&r.nav._v2.state()==StuckV2State::Normal&&r.nav.environmentalRecoveryInProgress());r.safety.hazard=false;r.front=80;for(unsigned i=0;i<130;++i)r.step();assert(!r.nav.environmentalRecoveryInProgress());return;}
 if(!std::strcmp(name,"critical")){r.boxed();const auto moves=r.motors.spins;r.safety.critical=true;r.update();r.step();assert(r.motors.motion==MotorState::Stopped&&r.motors.spins==moves&&r.nav._v2.state()==StuckV2State::Normal);return;}
#ifndef V32_REFERENCE
 if(!std::strcmp(name,"environment_sonar")||!std::strcmp(name,"environment_ir")){
  r.nav.enterEnvironmentalHazard(testNow);r.nav.beginScan(NavigationManager::ScanPurpose::EnvironmentalRecovery,testNow);
  if(!std::strcmp(name,"environment_sonar")){r.sensors.data.ultrasonicState=UltrasonicResultState::SensorFault;r.update();assert(r.nav.environmentalRecoveryInProgress()&&!r.nav.consumeEnvironmentalRecoveryCompleted());r.front=80;r.step(120);r.step();r.step();assert(r.scanning()&&r.nav._scanPurpose==NavigationManager::ScanPurpose::EnvironmentalRecovery);}
  else{r.ir(1);r.step();assert(r.nav.environmentalRecoveryInProgress()&&r.motors.motion==MotorState::Stopped);r.ir(0);r.step();assert(r.scanning()&&r.nav._scanPurpose==NavigationManager::ScanPurpose::EnvironmentalRecovery);}
  return;
 }
#endif
 assert(!"unknown restoration scenario");
}
#ifndef V32_SCENARIOS_LIBRARY
int main(int argc,char** argv){assert(argc==2);restoredScenario(argv[1],true);}
#endif
