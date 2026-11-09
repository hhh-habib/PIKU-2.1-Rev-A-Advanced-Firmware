#include "NavigationUnderTest.h"
#include <cassert>
#include <cstring>
#ifdef _MSC_VER
#include <stdlib.h>
struct ProbeErrors { ProbeErrors(){ _set_error_mode(_OUT_TO_STDERR); _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT); } } probeErrors;
#endif

struct ScanRig {
  MotorController motors; SensorManager sensors; ImuManager imu; ServoScanner servo;
  SafetyManager safety; WebDashboard dashboard; DashboardData data; String facing="FRONT";
  NavigationManager nav{motors,sensors,imu,servo,safety,dashboard,data,facing,nullptr,nullptr,nullptr};
  ScanRig() { testNow=10000; sensors.feed(UltrasonicResultState::DistanceValid,80); }
  void good(unsigned long stamp, bool open=false) {
    UltrasonicSample s; s.sampleMs=stamp; s.state=open?UltrasonicResultState::ClearNoEcho:UltrasonicResultState::DistanceValid;
    s.valid=!open; s.distanceCm=open?-1:80; nav.recordScanReading(s);
  }
  void invalid(bool fault=false) { UltrasonicSample s; s.state=fault?UltrasonicResultState::SensorFault:UltrasonicResultState::Stale; nav.recordScanReading(s); }
  bool accepted() { return nav.scanSampleTrusted(nav.finalizeCurrentScan()); }
};
int main(int argc,char** argv) {
  assert(argc==2); const char* name=argv[1]; ScanRig r;
  if(!std::strcmp(name,"good_good_timeout")){r.good(9900);r.good(9950);r.invalid();assert(r.accepted());return 0;}
  if(!std::strcmp(name,"timeout_good_good")){r.invalid();r.good(9900);r.good(9950);assert(r.accepted());return 0;}
  if(!std::strcmp(name,"good_timeout_good")){r.good(9900);r.invalid();r.good(9950);assert(r.accepted());return 0;}
  if(!std::strcmp(name,"one_good")){r.good(9950);r.invalid();r.invalid();assert(!r.accepted());return 0;}
  if(!std::strcmp(name,"all_invalid")){r.invalid();r.invalid();r.invalid();assert(!r.accepted());return 0;}
  if(!std::strcmp(name,"open_quorum")){r.good(9900,true);r.good(9950,true);r.invalid();assert(r.accepted());assert(r.nav.finalizeCurrentScan().ultrasonic.state==UltrasonicResultState::ClearNoEcho);return 0;}
  if(!std::strcmp(name,"mixed_no_quorum")){r.good(9900,true);r.good(9950);r.invalid();assert(!r.accepted());return 0;}
  if(!std::strcmp(name,"expired")){r.good(9900);r.good(9950);r.invalid();testNow=16000;assert(!r.accepted());return 0;}
  if(!std::strcmp(name,"clock_wrap")){testNow=25;r.good(0xfffffff0UL);r.good(0);r.invalid();assert(r.accepted());assert(r.nav.finalizeCurrentScan().ultrasonic.sampleMs==0);return 0;}
  if(!std::strcmp(name,"older_trusted")){r.good(9950);r.good(9900);r.invalid();assert(r.accepted());assert(r.nav.finalizeCurrentScan().ultrasonic.sampleMs==9950);return 0;}
  if(!std::strcmp(name,"fault_quorum")){r.good(9950);r.invalid(true);r.invalid(true);assert(!r.accepted());return 0;}
  if(!std::strcmp(name,"timeout_actual")){
    r.good(9900);r.good(9950);r.nav._phase=NavigationManager::AutoPhase::ScanSampleWait;r.nav._scanReadingIndex=2;
    r.nav._sampleTargetSequence=100;r.nav._sampleRequestedMs=9900;r.nav.updateScan(testNow);
    assert(r.nav.scanSampleTrusted(r.nav._leftScan));assert(r.motors.motion==MotorState::Stopped);return 0;
  }
  if(!std::strcmp(name,"orientation")||!std::strcmp(name,"generation")||!std::strcmp(name,"late")){
    r.nav._phase=NavigationManager::AutoPhase::ScanSampleWait;r.nav._scanDirectionIndex=1;
    r.nav._sampleTargetSequence=10;r.nav._sampleRequestedMs=9900;
    r.sensors.sample.sequence=10;r.sensors.sample.sampleMs=9950;r.sensors.sample.facingFront=true;
    if(!std::strcmp(name,"orientation"))r.sensors.sample.facingFront=false;
    if(!std::strcmp(name,"generation"))r.sensors.sample.sampleMs=9899;
    if(!std::strcmp(name,"late"))testNow=10500;
    assert(!r.nav.requestedSample(r.sensors.sample,testNow,true));r.nav.updateScan(testNow);
    assert(r.nav._scanAccumulator.trustedCount==0);assert(r.data.autoScanRejectedCount==1);assert(r.motors.motion==MotorState::Stopped);return 0;
  }
  if(!std::strcmp(name,"stop_scan")){r.nav._phase=NavigationManager::AutoPhase::ScanSampleWait;r.dashboard.pending="STOP";r.nav.updateAuto();assert(r.nav._phase==NavigationManager::AutoPhase::OperatorStop);assert(r.motors.motion==MotorState::Stopped);return 0;}
  if(!std::strcmp(name,"environment_scan")){r.nav._phase=NavigationManager::AutoPhase::ScanSampleWait;r.safety.hazard=true;r.nav.updateAuto();assert(r.nav._environmentalRecoveryActive);assert(r.motors.motion==MotorState::Stopped);return 0;}
  if(!std::strcmp(name,"fault_scan")){r.nav._phase=NavigationManager::AutoPhase::ScanSampleWait;r.sensors.data.ultrasonicState=UltrasonicResultState::SensorFault;r.nav.updateAuto();assert(r.nav._phase==NavigationManager::AutoPhase::SonarReacquire);assert(r.motors.motion==MotorState::Stopped);return 0;}
  if(!std::strcmp(name,"forward_untrusted_count")){r.nav._phase=NavigationManager::AutoPhase::Forward;r.sensors.sample.sampleMs=9000;r.nav.updateAuto();assert(r.data.autoForwardUntrustedCount==1);assert(r.motors.motion==MotorState::Stopped);return 0;}
  assert(false);
}
