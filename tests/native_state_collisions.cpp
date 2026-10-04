#include "NavigationUnderTest.h"
#include <cassert>
#include <cstdio>
#ifdef _MSC_VER
#include <stdlib.h>
struct ErrorPolicy { ErrorPolicy(){_set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);} } errorPolicy;
#endif
bool authorization=true;
bool authorized(){return authorization;}
struct Rig {
 MotorController motors; SensorManager sensors; ImuManager imu; ServoScanner servo;
 SafetyManager safety; WebDashboard web; DashboardData data; String facing="FRONT";
 NavigationManager nav{motors,sensors,imu,servo,safety,web,data,facing,nullptr,nullptr,nullptr
#ifdef CPS_COLLISION
 ,authorized
#endif
 };
 Rig(){authorization=true;feed();refresh();}
 void refresh(){imu.data.initialized=imu.data.healthy=true;imu.data.sampleMs=imu.data.lastSuccessMs=testNow;imu.data.gyroZ=0;}
 void feed(UltrasonicResultState s=UltrasonicResultState::DistanceValid,float cm=60){sensors.feed(s,cm);}
 void step(unsigned long dt=1){testNow+=dt;refresh();nav.updateAuto();}
 void environmental(){
  safety.hazard=true;step();assert(nav.environmentalRecoveryInProgress()&&motors.motion==MotorState::Stopped);
  safety.hazard=false;feed();step();feed();step(RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS);
 }
 void restoreSonar(){
  step(RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS);feed();step();feed();step();
 }
 void freshEnvironmentalScan(){
  assert(nav._scanPurpose==NavigationManager::ScanPurpose::EnvironmentalRecovery);
  assert(!nav._leftScan.completed&&!nav._frontScan.completed&&!nav._rightScan.completed);
  const unsigned long cleared=testNow;
  for(unsigned i=0;i<100&&nav.environmentalRecoveryInProgress();++i){
   if(nav._phase==NavigationManager::AutoPhase::ScanSampleWait){
    feed();sensors.sample.facingFront=nav._scanDirectionIndex==1;
   }
   step(100);assert(motors.motion!=MotorState::Forward);
  }
  assert(!nav.environmentalRecoveryInProgress()&&nav.consumeEnvironmentalRecoveryCompleted());
  assert(nav._leftScan.ultrasonic.sampleMs>cleared&&nav._frontScan.ultrasonic.sampleMs>cleared&&nav._rightScan.ultrasonic.sampleMs>cleared);
 }
 void episode(){nav._v2.noteBoxedIn();assert(nav._v2.startEscape(testNow));nav.beginRecoveryFrontCheck(testNow);}
};
int main(int argc,char** argv){
 assert(argc==2);String name=argv[1];
 if(name=="env_sonar"){Rig r;r.environmental();r.feed(UltrasonicResultState::SensorFault);r.step();assert(r.nav.environmentalRecoveryInProgress());assert(!r.nav.consumeEnvironmentalRecoveryCompleted());assert(r.motors.motion==MotorState::Stopped);}
 else if(name=="env_resume"||name=="env_repeated"){Rig r;r.environmental();for(unsigned i=0;i<(name=="env_repeated"?3U:1U);++i){r.feed(UltrasonicResultState::SensorFault);r.step();r.restoreSonar();assert(r.nav.environmentalRecoveryInProgress());assert(r.motors.motion==MotorState::Stopped);}r.freshEnvironmentalScan();}
 else if(name=="env_ir"){Rig r;r.environmental();r.safety.left=true;r.step();assert(r.nav.environmentalRecoveryInProgress());assert(r.nav._scanPurpose==NavigationManager::ScanPurpose::EnvironmentalRecovery);assert(r.motors.motion==MotorState::Stopped);}
 else if(name=="env_episode"){Rig r;r.episode();r.environmental();assert(r.nav.environmentalRecoveryInProgress());assert(r.nav._scanPurpose==NavigationManager::ScanPurpose::EnvironmentalRecovery);assert(r.nav._v2.episodeActive());}
 else if(name=="env_watch"){Rig r;r.episode();testNow+=12000;const auto spent=r.nav._v2.activeRecoveryMs(testNow);r.safety.hazard=true;r.step();r.step(20000);assert(r.nav._v2.activeRecoveryMs(testNow)==spent+1);r.safety.hazard=false;r.feed();r.step();r.feed();r.step(RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS);const auto scanStart=r.nav._v2.activeRecoveryMs(testNow);r.step(100);assert(r.nav._v2.activeRecoveryMs(testNow)==scanStart+100);r.step(30000);assert(r.nav._v2.state()==StuckV2State::TrappedHold&&r.motors.motion==MotorState::Stopped&&r.nav.environmentalRecoveryInProgress());}
 else if(name=="nested_recovery_hazard_sonar"){Rig r;r.episode();r.environmental();r.feed(UltrasonicResultState::SensorFault);r.step();r.restoreSonar();assert(r.nav._v2.episodeActive());r.freshEnvironmentalScan();}
 else if(name=="trapped_hazard"){Rig r;r.episode();r.nav._v2.hold(testNow);r.nav._phase=NavigationManager::AutoPhase::TrappedHold;const auto held=testNow;r.safety.hazard=true;r.step();assert(r.nav._v2.state()==StuckV2State::TrappedHold);assert(!r.nav._v2.retryReady(held+2499,2500));r.safety.hazard=false;r.feed();r.step();r.feed();r.step(RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS);r.freshEnvironmentalScan();assert(r.nav._v2.state()==StuckV2State::TrappedHold);}
 else if(name=="watch_cooldown"){StuckRecoveryV2Policy p;p.noteBoxedIn();assert(p.startEscape(100));p.hold(18100);assert(p.beginRetryRound(20600));assert(!p.escapeTimedOut(32599,30000));assert(p.escapeTimedOut(32600,30000));}
 else if(name=="watch_pause"){StuckRecoveryV2Policy p;p.noteBoxedIn();assert(p.startEscape(100));p.pause(12100);p.hold(32100);assert(p.beginRetryRound(34600));assert(!p.escapeTimedOut(52599,30000));assert(p.escapeTimedOut(52600,30000));}
 else if(name=="trapped_pause"){StuckRecoveryV2Policy p;p.noteBoxedIn();assert(p.startEscape(100));p.hold(1000);p.pause(1100);assert(p.state()==StuckV2State::TrappedHold);assert(!p.retryReady(3499,2500));assert(p.retryReady(3500,2500));}
 else if(name=="auto_stop"){Rig r;r.environmental();r.nav.notifyOperatorStop();assert(!r.nav.environmentalRecoveryInProgress()&&!r.nav._v2.episodeActive());r.feed();r.step();assert(r.motors.motion==MotorState::Stopped);++r.web.modeCounter;r.step();assert(!r.nav._operatorStopLatched);}
 else if(name=="sonar_turn"){Rig r;r.nav._phase=NavigationManager::AutoPhase::Turn;r.nav._relativeTurnDeg=100;r.motors.spinLeft();r.feed(UltrasonicResultState::SensorFault);r.step();assert(r.motors.motion==MotorState::Stopped&&r.nav.state()==NavigationState::SonarReacquire);}
 else if(name=="critical_forward"){Rig r;r.nav._phase=NavigationManager::AutoPhase::FrontSampleWait;r.nav._sampleTargetSequence=r.sensors.sample.sequence;r.safety.critical=true;r.step();assert(r.motors.motion==MotorState::Stopped&&r.nav.state()==NavigationState::CriticalAbort);}
 else if(name=="env_new_hazard"){Rig r;r.environmental();r.safety.hazard=true;r.step();assert(r.motors.motion==MotorState::Stopped&&r.nav.state()==NavigationState::HazardActive);r.safety.hazard=false;r.feed();r.step();r.step(RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS-1);assert(r.nav.state()==NavigationState::HazardClearWait);}
 else if(name=="env_mode"){Rig r;r.environmental();r.web.mode="MANUAL";r.step();assert(!r.nav.environmentalRecoveryInProgress()&&r.motors.motion==MotorState::Stopped);}
 else if(name=="boot"){Rig r;assert(r.motors.motion==MotorState::Stopped&&!r.nav._v2.episodeActive()&&!r.nav.environmentalRecoveryInProgress());}
#ifdef CPS_COLLISION
 else if(name=="env_integration"||name=="nested_sonar_integration"||name=="nested_integration_sonar"){
  Rig r;r.environmental();
  if(name=="nested_sonar_integration"){r.feed(UltrasonicResultState::SensorFault);r.step();}
  authorization=false;r.step();assert(r.motors.motion==MotorState::Stopped);
  if(name=="nested_integration_sonar")r.feed(UltrasonicResultState::SensorFault);
  r.step(2000);authorization=true;r.step();
  if(name!="env_integration")r.restoreSonar();
  assert(r.nav.environmentalRecoveryInProgress()&&r.motors.motion==MotorState::Stopped);
  r.freshEnvironmentalScan();
 }
#endif
 else {assert(!"Unknown scenario");}
 std::printf("PASS %s\n",argv[1]);
}
