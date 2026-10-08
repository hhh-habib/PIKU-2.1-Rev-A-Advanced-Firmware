#include "NavigationUnderTest.h"
#include <cassert>
#include <cstring>
#include "AvailabilityDiagnostics.inc"
#ifdef CPS_COLLISION
#include "RobotRadioStatus.h"
#endif

struct ActualRig {
 MotorController motors;SensorManager sensors;ImuManager imu;ServoScanner servo;
 SafetyManager safety;WebDashboard web;DashboardData data;String facing="FRONT";bool authorized=true;
 static ActualRig* current;
 NavigationManager nav{motors,sensors,imu,servo,safety,web,data,facing,nullptr,nullptr,nullptr
#ifdef CPS_COLLISION
 ,[](){return current->authorized;}
#endif
 };
 ActualRig(){current=this;testNow=100000;testTemperature=20;testHumidity=50;analogValue=100;sensors.startedMs=0;sensors.current.flameSampleValid=true;environment();fresh();}
 void fresh(float cm=12){sensors.finishDistanceSample(cm,true,UltrasonicResultState::DistanceValid);imu.data.initialized=imu.data.healthy=true;imu.data.gyroZ=0;imu.data.sampleMs=imu.data.lastSuccessMs=testNow;}
 void environment(){sensors.updateEnvironment(testNow);safety.update(sensors.snapshot());}
 void step(unsigned ms=1,float cm=12){testNow+=ms;fresh(cm);environment();nav.updateAuto();}
 void ir(unsigned mask){sensors.current.leftObstacleDetected=mask&1;sensors.current.rightObstacleDetected=mask&2;safety.update(sensors.snapshot());testNow+=mask?RobotConfig::IR_DEBOUNCE_MS:RobotConfig::DIGITAL_CLEAR_DEBOUNCE_MS;fresh();environment();}
 void boxed(){auto sample=[&](float cm,bool front){NavigationManager::NavigationScanSample q;q.completed=true;q.trustedReadings=3;q.ultrasonic.state=UltrasonicResultState::DistanceValid;q.ultrasonic.valid=true;q.ultrasonic.distanceCm=cm;q.ultrasonic.sampleMs=testNow;q.ultrasonic.facingFront=front;return q;};nav._leftScan=sample(20,false);nav._frontScan=sample(23,true);nav._rightScan=sample(26,false);nav.selectPathAfterScan(testNow);assert(nav._v2.hasCommittedSide());}
 void unavailable(unsigned affected,unsigned profile){
  if(affected&1)testTemperature=NAN;
  if(affected&2)analogValue=-1;
  if(profile==0){if(affected&1){sensors.current.temperatureLastSuccessMs=0;sensors.current.temperatureValid=false;}if(affected&2){sensors.current.mq2SampleMs=0;sensors.current.gasValid=false;}}
  if(profile==1){if(affected&1)sensors.current.temperatureLastSuccessMs=testNow-5001;if(affected&2)sensors.current.mq2SampleMs=testNow-301;safety.update(sensors.snapshot());return;}
  if(profile==4){if(affected&1)sensors.current.temperatureLastSuccessMs=testNow-90000;if(affected&2)sensors.current.mq2SampleMs=testNow-90000;}
  sensors.lastDhtMs=testNow-RobotConfig::DHT22_INTERVAL_MS;sensors.lastMq2Ms=testNow-RobotConfig::MQ2_INTERVAL_MS;environment();
 }
};
ActualRig* ActualRig::current=nullptr;
int main(int argc,char** argv){assert(argc==2);const String name=argv[1];
 if(name=="phases"){
  for(unsigned affected=1;affected<=3;++affected)for(unsigned profile=0;profile<5;++profile)for(unsigned phase=0;phase<9;++phase){
   ActualRig r;
   if(phase==0){r.fresh(80);r.nav.beginSpinTurn(true,NavigationManager::ScanPurpose::Obstacle,testNow);assert(r.motors.motion==MotorState::SpinLeft);}
   if(phase==1)r.nav.beginScan(NavigationManager::ScanPurpose::Obstacle,testNow);
   if(phase>=2&&phase<8)r.boxed();
   if(phase==3||phase==4){r.ir(1);r.nav.updateAuto();assert(r.motors.motion==MotorState::Backward);}
   if(phase==4){r.step(200);r.ir(0);r.step();assert(r.motors.motion==MotorState::Stopped);}
   if(phase==5){r.nav.completeSpin("GYRO_TARGET",testNow);r.nav.beginRecoveryFrontCheck(testNow);}
   if(phase==6){r.nav.completeSpin("GYRO_TARGET",testNow);r.nav.beginRecoverySideCheck(testNow);}
   if(phase==7)r.nav.enterV2TrappedHold("ESCAPE_TURN_LIMIT",testNow);
   if(phase==8){r.nav.beginFrontValidation(testNow,"NORMAL_FORWARD");r.step(120,80);r.step(1,80);assert(r.motors.motion==MotorState::Forward);}
   const auto initialPhase=r.nav._phase;const auto motion=r.motors.motion;const auto slot=r.nav._v2.sideTurns();
   const auto start=r.nav._turnStartedMs;const auto before=r.nav._v2.episodeStartedMs();const auto requests=r.nav._sampleTargetSequence;
   r.unavailable(affected,profile);assert(!r.safety.blocksMotion()&&r.safety.monitoringUnavailable());
   r.step(1,phase==8?80:12);
   assert(r.nav._phase==initialPhase&&r.motors.motion==motion&&r.nav._v2.sideTurns()==slot&&r.nav._turnStartedMs==start&&r.nav._sampleTargetSequence==requests);
   assert(r.nav._v2.episodeStartedMs()==before);
   if(profile==3){testTemperature=20;analogValue=100;r.sensors.lastDhtMs=r.sensors.lastMq2Ms=0;r.environment();r.unavailable(affected,2);r.step(1,phase==8?80:12);assert(r.nav._phase==initialPhase&&r.nav._v2.sideTurns()==slot);}
  }
 }
 if(name=="independent"){
  ActualRig r;const auto temp=r.sensors.current.temperatureLastSuccessMs,gas=r.sensors.current.mq2SampleMs;
  testTemperature=NAN;testNow+=2000;r.environment();assert(r.sensors.current.temperatureLastSuccessMs==temp&&r.sensors.current.mq2SampleMs==testNow);
  const auto gasNow=r.sensors.current.mq2SampleMs;analogValue=-1;testTemperature=20;testNow+=2000;r.environment();
  assert(r.sensors.current.temperatureLastSuccessMs==testNow&&r.sensors.current.mq2SampleMs==gasNow&&!r.sensors.current.gasValid&&!r.safety.blocksMotion());
  testTemperature=NAN;const auto lastTemp=r.sensors.current.temperatureLastSuccessMs;testNow+=2000;r.environment();
  assert(r.sensors.current.temperatureLastSuccessMs==lastTemp&&r.sensors.current.mq2SampleMs==gasNow&&r.safety.stateName()=="MONITORING_UNAVAILABLE");
  r.sensors.updateControlSensors();assert(!r.sensors.snapshot().temperatureValid&&!r.sensors.snapshot().gasValid);
 }
 if(name=="hazards"){
  for(unsigned cause=0;cause<3;++cause){
   ActualRig r;
   if(cause==0){testTemperature=60;testNow+=2000;r.environment();testNow+=1000;r.environment();assert(r.safety.isTemperatureCritical());}
   if(cause==1){analogValue=2500;for(unsigned i=0;i<24;++i){testNow+=150;r.environment();}assert(r.safety.isGasCritical());}
   if(cause==2){r.sensors.current.flameDetected=true;r.safety.update(r.sensors.snapshot());testNow+=80;r.fresh();r.environment();assert(r.safety.isFlameCritical());}
   r.unavailable(3,2);for(unsigned i=0;i<30;++i){testNow+=2000;r.fresh();r.environment();assert(r.safety.isCritical()&&r.safety.blocksMotion()&&r.safety.stateName()=="CRITICAL");r.nav.updateAuto();assert(r.motors.motion==MotorState::Stopped);if(cause!=0)assert(r.nav.environmentalRecoveryInProgress()&&r.nav._phase==NavigationManager::AutoPhase::HazardActive&&r.nav._hazardClearStartedMs==0);}
   assert(r.safety.monitoringUnavailable());assert(r.safety.blocksMotion(true)==(cause==0));
   testTemperature=20;analogValue=100;r.sensors.current.flameDetected=false;
   for(unsigned i=0;i<35;++i){testNow+=150;r.environment();}assert(!r.safety.isCritical());
  }
 }
 if(name=="gates"){
  for(unsigned gate=0;gate<4;++gate){ActualRig r;r.boxed();r.unavailable(3,0);
   if(gate==0)r.web.pending="STOP";
   if(gate==1)r.sensors.current.ultrasonicState=UltrasonicResultState::SensorFault;
   if(gate==2)r.imu.data.healthy=false;
#ifdef CPS_COLLISION
   if(gate==3)r.authorized=false;
#else
   if(gate==3)r.web.mode="MANUAL";
#endif
   const auto writes=r.motors.spins+r.motors.backwards+r.motors.forwards;r.nav.updateAuto();r.nav.updateAuto();
   assert(r.motors.motion==MotorState::Stopped&&r.motors.spins+r.motors.backwards+r.motors.forwards==writes);
  }
 }
 if(name=="diagnostics"){
  ActualRig r;assert(getGasStatus(r.sensors.snapshot())=="LOW_RELATIVE");r.unavailable(3,2);assert(getGasStatus(r.sensors.snapshot())=="INVALID"&&r.safety.stateName()=="MONITORING_UNAVAILABLE");
  r.sensors.current.mq2SampleMs=0;assert(getGasStatus(r.sensors.snapshot())=="UNAVAILABLE");r.sensors.current.mq2SampleMs=testNow-301;r.sensors.current.gasValid=true;assert(getGasStatus(r.sensors.snapshot())=="STALE");
  testTemperature=60;r.sensors.lastDhtMs=0;r.environment();testNow+=1000;r.environment();assert(r.safety.isTemperatureCritical());r.unavailable(3,2);assert(r.safety.stateName()=="CRITICAL"&&r.safety.alarmReason()=="TEMP_HIGH"&&r.safety.monitoringUnavailable());
#ifdef CPS_COLLISION
  auto snapshot=r.sensors.snapshot();auto packet=robotRadioStatus(snapshot,testNow,RadioMode::Auto,MotorState::Stopped,60,RadioTurn::Spin,RadioGas::Unknown,TemperatureCritical,RadioNavigation::HazardHold);
  assert(!(packet.validityFlags&(TemperatureValid|GasValid))&&(packet.stateFlags&TemperatureCritical));
  snapshot.temperature=20;snapshot.temperatureValid=true;snapshot.temperatureLastSuccessMs=testNow;snapshot.humidityValid=false;
  packet=robotRadioStatus(snapshot,testNow,RadioMode::Auto,MotorState::Stopped,60,RadioTurn::Spin,RadioGas::Unknown,TemperatureCritical,RadioNavigation::HazardHold);
  assert((packet.validityFlags&TemperatureValid)&&!(packet.validityFlags&(GasValid|HumidityValid))&&(packet.stateFlags&TemperatureCritical));
#endif
 }
 if(name=="boundaries"){
  for(bool clear:{false,true}){ActualRig r;r.boxed();r.unavailable(3,4);const auto start=r.nav._v2.episodeStartedMs();testNow=start+30000;r.fresh(clear?40:12);r.nav.updateAuto();assert(r.nav._v2.state()==StuckV2State::TrappedHold&&r.motors.motion==MotorState::Stopped&&!r.safety.blocksMotion());}
  ActualRig r;r.boxed();r.unavailable(3,2);r.nav._relativeTurnDeg=12;r.ir(1);r.nav.updateAuto();assert(r.nav._relativeTurnDeg==0&&r.nav._turnStartedMs==0);
  const auto slot=r.nav._v2.sideTurns();r.ir(0);r.step();assert(r.motors.motion==MotorState::Stopped&&r.nav._phase==NavigationManager::AutoPhase::RecoveryFrontSettle&&r.nav._v2.sideTurns()==slot);
  r.nav.notifyOperatorStop();testTemperature=20;analogValue=100;r.step(2000);assert(r.nav._operatorStopLatched&&r.motors.motion==MotorState::Stopped);
 }
}
