#include "NavigationUnderTest.h"
#include "EvidenceSafetyManager.h"
#include <cassert>
#include <cstring>

int main(int argc,char** argv){assert(argc==2);const String name=argv[1];
 SensorManager sensors;EvidenceSafetyManager safety;
 if(name=="freshness"){
  testNow=100000;sensors.startedMs=0;sensors.updateEnvironment(testNow);safety.update(sensors.snapshot());assert(!safety.monitoringUnavailable());
  testTemperature=60;testHumidity=NAN;testNow+=2000;sensors.updateEnvironment(testNow);safety.update(sensors.snapshot());
  assert(sensors.snapshot().temperatureLastSuccessMs==testNow&&sensors.snapshot().temperatureValid&&!sensors.snapshot().humidityValid);
  testNow+=1000;safety.update(sensors.snapshot());assert(safety.hasNonRecoverableCriticalHazard());
  const auto success=sensors.snapshot().temperatureLastSuccessMs,gasSuccess=sensors.snapshot().mq2SampleMs;
  testTemperature=NAN;analogValue=-1;testNow+=2000;sensors.updateEnvironment(testNow);safety.update(sensors.snapshot());
  assert(sensors.snapshot().temperatureLastSuccessMs==success&&sensors.snapshot().mq2SampleMs==gasSuccess);
  assert(safety.hasNonRecoverableCriticalHazard()&&safety.monitoringUnavailable()&&safety.blocksMotion(true));
  testTemperature=20;analogValue=100;testNow+=2000;sensors.updateEnvironment(testNow);safety.update(sensors.snapshot());
  assert(safety.hasNonRecoverableCriticalHazard());testNow+=1000;sensors.updateEnvironment(testNow);safety.update(sensors.snapshot());assert(!safety.isCritical()&&!safety.monitoringUnavailable());
  SensorSnapshot gas=sensors.snapshot();gas.mq2Warmup=false;gas.gasValid=true;gas.gasFiltered=2500;
  gas.mq2SampleMs=testNow;safety.update(gas);testNow+=RobotConfig::GAS_DEBOUNCE_MS;gas.mq2SampleMs=gas.temperatureLastSuccessMs=testNow;safety.update(gas);assert(safety.isGasCritical());
  gas.gasValid=false;gas.gasFiltered=0;testNow+=RobotConfig::GAS_CLEAR_DEBOUNCE_MS+1;gas.temperatureLastSuccessMs=testNow;safety.update(gas);assert(safety.isGasCritical()&&safety.monitoringUnavailable());
 }
 if(name=="cadence"){
  sensors.begin();sensors.updateEnvironment(testNow);const auto reads=dhtReads;
  for(unsigned i=0;i<10;++i){testNow+=10;sensors.updateEnvironment(testNow);}assert(dhtReads==reads);
  testNow+=2000;sensors.updateEnvironment(testNow);assert(dhtReads==reads+1);
  SensorSnapshot s;s.environmentStartedMs=testNow;s.mq2Warmup=true;safety.update(s);assert(safety.monitoringUnavailable()&&!safety.blocksMotion());
  testNow+=2001;safety.update(s);assert(safety.monitoringUnavailable());
  s.temperature=20;s.temperatureValid=true;s.temperatureLastSuccessMs=testNow;s.mq2Warmup=false;s.gasValid=false;safety.update(s);
  assert(safety.monitoringUnavailable()&&!safety.blocksMotion()&&!safety.blocksMotion(true));
  s.gasValid=true;s.mq2SampleMs=testNow;testNow+=301;s.temperatureLastSuccessMs=testNow;safety.update(s);assert(safety.monitoringUnavailable());
 }
 if(name=="request"){
  MotorController motors;ImuManager imu;ServoScanner servo;SafetyManager navSafety;WebDashboard web;DashboardData data;String facing="FRONT";
  NavigationManager nav{motors,sensors,imu,servo,navSafety,web,data,facing,nullptr,nullptr,nullptr
#ifdef CPS_COLLISION
  ,[](){return true;}
#endif
  };
  testNow=0xfffffff0UL;sensors.distanceSampleSequenceCounter=0xfffffffeUL;
  sensors.finishDistanceSample(80,true,UltrasonicResultState::DistanceValid);assert(sensors.distanceSampleSequence()==0xffffffffUL);
  nav.beginFrontValidation(testNow,"N20_WRAP");testNow+=RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS;nav.updateFrontValidation(testNow);
  assert(nav._sampleTargetSequence==0&&nav._phase==NavigationManager::AutoPhase::FrontSampleWait);
  nav.updateFrontValidation(testNow);assert(motors.motion==MotorState::Stopped);
  // The real trigger/echo completion writes sequence zero after clock wrap.
  sensors.advanceDistanceSample(2);sensors.advanceDistanceSample(12);sensors.echoRiseCaptured=sensors.echoFallCaptured=true;sensors.echoRiseUs=20;sensors.echoFallUs=3520;
  sensors.advanceDistanceSample(4000);assert(sensors.latestUltrasonicSample().sequence==0);
  nav.updateFrontValidation(testNow);assert(motors.motion==MotorState::Forward);
  nav.beginFrontValidation(testNow,"N20_PENDING");testNow+=RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS;nav.updateFrontValidation(testNow);
  const auto requestTime=nav._sampleRequestedMs;auto old=sensors.latestUltrasonicSample();old.sequence=nav._sampleTargetSequence;old.sampleMs=requestTime-1;
  assert(!nav.requestedSample(old,testNow,true));old.sampleMs=testNow;old.facingFront=false;assert(!nav.requestedSample(old,testNow,true));
  old.facingFront=true;assert(nav.requestedSample(old,testNow,true));old.sampleMs=0;nav._sampleRequestedMs=0;testNow=0;assert(nav.requestedSample(old,testNow,true));
 }
}
