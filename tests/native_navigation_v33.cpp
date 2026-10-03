#include "NavigationUnderTest.h"
#include <cassert>
struct Rig {
 MotorController motors; SensorManager sensors; ImuManager imu; ServoScanner servo; SafetyManager safety; WebDashboard web; DashboardData data; String facing="FRONT";
 NavigationManager nav{motors,sensors,imu,servo,safety,web,data,facing,nullptr,nullptr,nullptr};
 Rig(){sensors.feed(UltrasonicResultState::DistanceValid); refresh();}
 void refresh(){imu.data.initialized=imu.data.healthy=true;imu.data.sampleMs=imu.data.lastSuccessMs=testNow;imu.data.gyroZ=0;}
 void step(unsigned long delta=1){testNow+=delta;refresh();nav.updateAuto();}
 void feed(UltrasonicResultState state,float cm=20){sensors.feed(state,cm);}
 void scan(float l=25,float f=20,float r=18){set(nav._leftScan,l);set(nav._frontScan,f);set(nav._rightScan,r);}
 void set(NavigationManager::NavigationScanSample& s,float cm){s.completed=true;s.trustedReadings=s.finiteReadings=3;s.ultrasonic.state=UltrasonicResultState::DistanceValid;s.ultrasonic.distanceCm=cm;s.ultrasonic.valid=true;s.ultrasonic.sampleMs=testNow;s.ultrasonic.sequence=sensors.sample.sequence;}
 void recovery(){scan();nav.selectPathAfterScan(testNow);assert(motors.motion==MotorState::SpinLeft);}
 void front(float cm){nav.beginRecoveryFrontCheck(testNow);step(120);feed(UltrasonicResultState::DistanceValid,cm);step();}
};
int main(){
 {Rig r;r.recovery();r.safety.left=true;r.step();for(unsigned i=0;i<5;++i){r.feed(UltrasonicResultState::DistanceValid,20);r.step(200);r.step();}assert(r.motors.backwards==5);auto used=r.nav._v2.sideTurns();r.safety.left=false;r.feed(UltrasonicResultState::DistanceValid,20);r.step(2500);r.safety.left=true;r.step();assert(r.motors.backwards==5&&r.nav._recoveryIrBackoff.missionPulses()==5&&r.nav._v2.sideTurns()==used);}
 {Rig r;r.recovery();r.feed(UltrasonicResultState::SensorFault);r.step();r.step(120);r.feed(UltrasonicResultState::DistanceValid,20);r.step();r.feed(UltrasonicResultState::DistanceValid,20);r.step();r.step(120);r.feed(UltrasonicResultState::DistanceValid,20);r.step();r.set(r.nav._leftScan,15);r.nav.finishRecoverySideCheck(testNow);assert(r.motors.motion==MotorState::SpinLeft&&r.nav._v2.sideTurns()==2);}

 {Rig r;r.scan(150,10,50);r.nav.selectPathAfterScan(testNow);r.nav.updateDiagnostics(testNow);assert(r.data.autoTurnTargetDeg==35);}
 {Rig r;r.recovery();for(unsigned i=1;i<6;++i){r.nav._v2.turnVerified();r.nav._v2.continueTurn();r.nav.beginV2Turn(true,testNow);}assert(r.motors.spins==6);r.front(20);assert(r.motors.motion==MotorState::Stopped);for(unsigned i=0;i<20;++i){r.feed(UltrasonicResultState::DistanceValid);r.step(2500);}assert(r.motors.spins==6&&r.nav._v2.sideTurns()==6);}
 for(auto state:{UltrasonicResultState::SensorFault,UltrasonicResultState::Stale}){Rig r;r.recovery();r.safety.left=true;r.step();auto pulses=r.nav._recoveryIrBackoff.missionPulses();r.safety.left=false;r.feed(state);r.step();assert(r.nav._v2.sideTurns()==1&&r.nav._v2.committedLeft());assert(r.motors.motion==MotorState::Stopped);r.step(120);for(unsigned i=0;i<12;++i){r.feed(state);r.step(100);assert(r.motors.motion==MotorState::Stopped);}r.feed(UltrasonicResultState::DistanceValid,20);r.step();assert(r.nav.state()==NavigationState::SonarReacquire);r.feed(UltrasonicResultState::ClearNoEcho);r.step();assert(r.motors.motion==MotorState::Stopped&&r.nav._v2.episodeActive());assert(r.nav._v2.sideTurns()==1&&r.nav._recoveryIrBackoff.missionPulses()==pulses);}
 {Rig r;r.feed(UltrasonicResultState::SensorFault);r.step();r.step(120);r.feed(UltrasonicResultState::ClearNoEcho);r.step();assert(r.motors.motion==MotorState::Stopped);r.feed(UltrasonicResultState::DistanceValid,60);r.step();assert(r.motors.motion==MotorState::Forward);}
 {Rig r;r.recovery();r.front(30);assert(r.nav._v2.handoffPending()&&r.nav._v2.sideTurns()==1);r.feed(UltrasonicResultState::DistanceValid,20);r.nav.beginObstacleRecovery(testNow);assert(r.nav._v2.episodeActive()&&r.nav._v2.sideTurns()==1&&!r.nav._v2.handoffPending());auto spins=r.motors.spins;r.nav.beginSpinTurn(false,NavigationManager::ScanPurpose::Obstacle,testNow);assert(r.motors.spins==spins);}
 {Rig r;r.recovery();r.safety.hazard=true;r.step();assert(r.motors.motion==MotorState::Stopped&&r.nav._v2.sideTurns()==1);r.safety.hazard=false;r.feed(UltrasonicResultState::DistanceValid,20);r.step();r.feed(UltrasonicResultState::DistanceValid,20);r.step(RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS);assert(r.nav._v2.episodeActive()&&r.nav._v2.sideTurns()==1);}
 {Rig r;r.recovery();r.safety.critical=true;r.step();assert(r.motors.motion==MotorState::Stopped&&r.nav._v2.sideTurns()==1);r.safety.critical=false;r.feed(UltrasonicResultState::DistanceValid,20);r.step();assert(r.nav._v2.episodeActive()&&r.nav._v2.sideTurns()==1);}
 {Rig r;r.recovery();r.web.pending="STOP";r.step();assert(r.motors.motion==MotorState::Stopped);r.feed(UltrasonicResultState::DistanceValid,60);r.step();assert(r.motors.motion==MotorState::Stopped);++r.web.modeCounter;r.step();assert(!r.nav._v2.episodeActive());}
 {Rig r;r.recovery();r.front(30);for(unsigned i=0;i<300&&r.nav._v2.episodeActive();++i){r.feed(UltrasonicResultState::DistanceValid,60);r.step(100);}assert(!r.nav._v2.episodeActive()&&r.nav._v2.sideTurns()==0);}
}
