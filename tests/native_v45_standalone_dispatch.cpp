#include <cstdio>
void tick(unsigned long dt=1){testNow+=dt;sensors.finishDistanceSample(60,true,UltrasonicResultState::DistanceValid);imu.data.initialized=imu.data.healthy=true;imu.data.sampleMs=imu.data.lastSuccessMs=testNow;imu.data.gyroZ=0;loop();}
int main(int argc,char** argv){assert(argc==2);const String name=argv[1];testNow=100000;dashboard.mode="MANUAL";tick();
 if(name=="v46_mode"){
  webAction="BACKWARD";tick();assert(motors.motion==MotorState::Backward);
  dashboard.mode="AUTO";++dashboard.modeCounter;onDashboardModeAccepted("AUTO");
  assert(motors.motion==MotorState::Stopped&&controlStopThisPass);
  // Service between background sonar starts so the real producer can admit its
  // requested front job; coarse cadence-aligned ticks kept this double busy.
  for(unsigned i=0;i<64&&motors.motion!=MotorState::Forward;++i){tick(10);assert(!navigation._operatorStopLatched);}
if(motors.motion!=MotorState::Forward)fprintf(stderr,"mode continuity phase=%d state=%s decision=%s sonar=%d critical=%d pending=%s\n",int(navigation._phase),navigation.stateName(),dashboardData.navigationDecision.c_str(),int(sensors.snapshot().ultrasonicState),safety.isCritical(),dashboard.pending.c_str());
assert(!navigation._operatorStopLatched&&motors.motion==MotorState::Forward);
  for(unsigned i=0;i<8;++i){tick(100);assert(!navigation._operatorStopLatched);}
  const auto writes=motors.spins+motors.backwards+motors.forwards;webAction="STOP";tick();tick();
  assert(navigation._operatorStopLatched&&motors.motion==MotorState::Stopped&&motors.spins+motors.backwards+motors.forwards==writes);

  for(const String action:{"STOP","RELEASE"})for(unsigned context=0;context<6;++context){
   dashboard.mode=lastControlMode="AUTO";dashboard.pending="";controlStopThisPass=false;navigation.cancelForModeChange();
   if(context==0)navigation.beginSpinTurn(false,NavigationManager::ScanPurpose::IrSingle,testNow);
   else {
    navigation.beginObstacleRecovery(testNow);
    if(context>=2){testNow+=RobotConfig::AUTO_REVERSE_PREPARE_MS;navigation.updateReverseSequence(testNow);}
    if(context>=3){testNow+=RobotConfig::AUTO_OBSTACLE_REVERSE_MS;navigation.updateReverseSequence(testNow);}
    if(context==4){testNow+=RobotConfig::AUTO_REVERSE_SETTLE_MS;navigation.updateReverseSequence(testNow);}
    if(context==5)navigation.beginScan(NavigationManager::ScanPurpose::Obstacle,testNow);
   }
   // Make old gyro/pulse/evidence completion due alongside actual revocation.
   navigation._relativeTurnDeg=100;
   const auto before=motors.spins+motors.backwards+motors.forwards;webAction=action;tick(500);
   assert(motors.motion==MotorState::Stopped&&controlStopThisPass&&motors.spins+motors.backwards+motors.forwards==before);
   if(action=="STOP"){tick(500);assert(navigation._operatorStopLatched&&motors.spins+motors.backwards+motors.forwards==before);}
   else assert(!navigation._operatorStopLatched); // RELEASE keeps its V4.5 non-latching contract.
  }

 }
 if(name=="load"){
  dashboard.flood=true;const auto reads=dhtReads;for(unsigned i=0;i<10;++i)tick(500);assert(dhtReads>reads&&motors.motion==MotorState::Backward);
  testTemperature=NAN;analogValue=-1;tick(2000);assert(motors.motion==MotorState::Backward&&!criticalStopLatched&&!criticalStopRequiresManualRearm);
  testTemperature=60;analogValue=100;tick(2000);tick(1000);assert(safety.hasNonRecoverableCriticalHazard()&&motors.motion==MotorState::Stopped);
 }
 if(name=="unavailable_manual"){
  dashboard.flood=true;
  for(unsigned affected=1;affected<=3;++affected)for(unsigned profile=0;profile<5;++profile){
   testTemperature=20;analogValue=100;sensors.lastDhtMs=sensors.lastMq2Ms=0;tick();
   if(affected&1)testTemperature=NAN;if(affected&2)analogValue=-1;
   if(profile==0){if(affected&1){sensors.current.temperatureValid=false;sensors.current.temperatureLastSuccessMs=0;}if(affected&2){sensors.current.gasValid=false;sensors.current.mq2SampleMs=0;}}
   if(profile==1||profile==4){if(affected&1)sensors.current.temperatureLastSuccessMs=testNow-(profile==4?90000:5001);if(affected&2)sensors.current.mq2SampleMs=testNow-(profile==4?90000:301);}
   sensors.lastDhtMs=sensors.lastMq2Ms=0;tick();
   assert(motors.motion==MotorState::Backward&&!criticalStopLatched&&!criticalStopRequiresManualRearm&&!controlStopThisPass);
   if(profile==3){testTemperature=20;analogValue=100;sensors.lastDhtMs=sensors.lastMq2Ms=0;tick();if(affected&1)testTemperature=NAN;if(affected&2)analogValue=-1;sensors.lastDhtMs=sensors.lastMq2Ms=0;tick();assert(motors.motion==MotorState::Backward);}
  }
  dashboard.flood=false;webAction="STOP";tick();tick();assert(motors.motion==MotorState::Stopped);
 }
 if(name=="rearm"){
  testTemperature=60;tick(2000);tick(1000);assert(criticalStopLatched&&criticalStopRequiresManualRearm);
  testTemperature=20;tick(2000);tick(1000);assert(!safety.isCritical());webAction="BACKWARD";tick();assert(!criticalStopLatched&&!criticalStopRequiresManualRearm&&motors.motion==MotorState::Backward);
  dashboard.mode="AUTO";++dashboard.modeCounter;onDashboardModeAccepted("AUTO");tick();tick();assert(!criticalStopRequiresManualRearm&&!criticalStopLatched);
  ++dashboard.modeCounter;onDashboardModeAccepted("AUTO");tick();tick();assert(!criticalStopRequiresManualRearm);
  sensors.current.flameSampleValid=true;sensors.current.flameDetected=true;safety.update(sensors.snapshot());testNow+=RobotConfig::FLAME_DEBOUNCE_MS;safety.update(sensors.snapshot());assert(safety.isCritical());
  navigation.updateAuto();assert(navigation.environmentalRecoveryInProgress()&&motors.motion==MotorState::Stopped);
 }
 if(name=="race"){
  for(const String action:{"STOP","RELEASE"}){dashboard.mode=lastControlMode="AUTO";navigation.cancelForModeChange();controlStopThisPass=false;
   auto sample=[](float cm){NavigationManager::NavigationScanSample s;s.completed=true;s.trustedReadings=3;s.ultrasonic.state=UltrasonicResultState::DistanceValid;s.ultrasonic.valid=true;s.ultrasonic.distanceCm=cm;s.ultrasonic.sampleMs=testNow;return s;};
   navigation._leftScan=sample(20);navigation._frontScan=sample(23);navigation._rightScan=sample(26);navigation.selectPathAfterScan(testNow);if(!navigation._v2.hasCommittedSide())fprintf(stderr,"admission %s decision=%s v2=%s imu=%d state=%d\n",action.c_str(),dashboardData.navigationDecision.c_str(),navigation._v2LastReason.c_str(),navigation.v2ImuReady(testNow),int(navigation._phase));assert(navigation._v2.hasCommittedSide());
   navigation.completeSpin("GYRO_TARGET",testNow);navigation.beginRecoveryFrontCheck(testNow);testNow+=RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS;navigation.updateRecoveryFrontCheck(testNow);
   const auto writes=motors.spins+motors.backwards+motors.forwards;sensors.finishDistanceSample(40,true,UltrasonicResultState::DistanceValid);webAction=action;tick();assert(motors.motion==MotorState::Stopped&&motors.spins+motors.backwards+motors.forwards==writes);if(action=="STOP")tick();assert(motors.spins+motors.backwards+motors.forwards==writes);
  }
 }
 if(name=="wifi_auto"){
  dashboard.mode=lastControlMode="AUTO";navigation.cancelForModeChange();dashboard.live=false;
  for(unsigned i=0;i<64&&motors.motion!=MotorState::Forward;++i)tick(10);
  assert(motors.motion==MotorState::Forward&&!navigation._operatorStopLatched);
  auto sample=[](float cm){NavigationManager::NavigationScanSample q;q.completed=true;q.trustedReadings=3;q.ultrasonic.state=UltrasonicResultState::DistanceValid;q.ultrasonic.valid=true;q.ultrasonic.distanceCm=cm;q.ultrasonic.sampleMs=testNow;return q;};
  navigation._leftScan=sample(20);navigation._frontScan=sample(23);navigation._rightScan=sample(26);navigation.selectPathAfterScan(testNow);assert(navigation._v2.hasCommittedSide());tick(10);assert(!navigation._operatorStopLatched);
  webAction="STOP";tick();const auto moves=motors.spins+motors.backwards+motors.forwards;dashboard.live=true;tick();assert(navigation._operatorStopLatched&&motors.motion==MotorState::Stopped&&moves==motors.spins+motors.backwards+motors.forwards);
 }
}
