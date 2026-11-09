#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"
#include "ImuManager.h"
#include "DisplayManager.h"
#include <cassert>
#include <iostream>

unsigned long testNow=10000;
unsigned delayCalls=0;
SerialDouble Serial;
WireDouble Wire;
OledState oledState;
SpiDouble SPI;

void step(ImuManager& imu, bool stopped=true, unsigned ms=25) {
  testNow=uint32_t(testNow+ms); Wire.moving=!stopped;
  const unsigned ops=Wire.reads+Wire.writes;
  imu.update(stopped);
  assert(Wire.reads+Wire.writes-ops<=1); // One bounded transaction per service pass.
  assert(Wire.timeout==50 && delayCalls==0 && Wire.writesMoving==0);
}
void run(ImuManager& imu, unsigned n=80, bool stopped=true) {
  for(unsigned i=0;i<n;++i) step(imu,stopped);
}
void ready(ImuManager& imu) {
  imu.begin(); assert(!imu.snapshot().healthy && !imu.snapshot().hasSample);
  for(unsigned i=0;i<100 && !imu.snapshot().healthy;++i) step(imu);
  assert(imu.snapshot().healthy && imu.snapshot().initialized && imu.snapshot().hasSample);
}
DashboardData normal() {
  DashboardData d;d.temperature=28.4f;d.temperatureValid=true;d.humidity=65;
  d.humidityValid=true;d.distanceValid=true;d.frontDistance=45;d.gasValue=850;
  d.gasStatus="LOW_RELATIVE";d.mq2Warmup=false;d.flameSampleValid=true;
  d.staConnected=true;d.staIp="192.168.43.120";return d;
}
void displayAfter(DisplayManager& display, DashboardData& d, unsigned ms=1000) {
  testNow=uint32_t(testNow+ms);display.updateDashboard(d);
}
bool contains(const char* text) { return oledState.text.find(text)!=String::npos; }

int main(int argc, char** argv) {
  assert(argc==2);const String name=argv[1];ImuManager imu;
  if(name=="qualification") {
    imu.begin();while(!imu.snapshot().hasSample) step(imu);
    assert(!imu.snapshot().healthy);step(imu);assert(!imu.snapshot().healthy);
    step(imu);assert(imu.snapshot().healthy);
    assert(Wire.registers[0x1a]==4 && Wire.registers[0x1b]==8 && Wire.registers[0x1c]==8);
  } else if(name=="stationary") {
    ready(imu);run(imu,200,true);assert(imu.snapshot().healthy);
    assert(imu.snapshot().gyroZ==0 && std::fabs(imu.snapshot().accelZ-9.80665f)<.0001f);
    assert(String(imu.snapshot().motionState)=="STILL");
  } else if(name=="units") {
    ready(imu);Wire.raw={{-8192,8192,8192,340,-655,655,6550}};step(imu);
    assert(std::fabs(imu.snapshot().accelX+9.80665f)<.0001f);
    assert(std::fabs(imu.snapshot().temperature-37.53f)<.0001f);
    assert(std::fabs(imu.snapshot().gyroX+10*DEG_TO_RAD)<.0001f);
    assert(std::fabs(imu.snapshot().gyroZ-100*DEG_TO_RAD)<.0001f);
  } else if(name=="boot_retry" || name=="address69" || name=="wrong_identity") {
    if(name=="address69") Wire.device=0x69;
    else if(name=="boot_retry") Wire.present=false;
    else Wire.identity=0x70;
    imu.begin();run(imu,120);if(name=="address69") assert(imu.snapshot().address==0x69);
    else {assert(!imu.snapshot().healthy && !imu.snapshot().hasSample);Wire.present=true;Wire.identity=0x68;run(imu,400);}
    assert(imu.snapshot().healthy);
  } else if(name=="short_read" || name=="bus_error" || name=="zero_frame" || name=="ff_frame") {
    ready(imu);const auto last=imu.snapshot().lastSuccessMs;
    Wire.shortRead=name=="short_read";Wire.fail=name=="bus_error";
    Wire.zeros=name=="zero_frame";Wire.ones=name=="ff_frame";
    step(imu,false);assert(!imu.snapshot().healthy && imu.snapshot().lastSuccessMs==last);
    assert(imu.snapshot().errorCount==1 && imu.snapshot().hasSample);
    Wire.shortRead=Wire.fail=Wire.zeros=Wire.ones=false;
    step(imu,false);assert(!imu.snapshot().healthy);step(imu,false);assert(!imu.snapshot().healthy);
    step(imu,false);assert(imu.snapshot().healthy); // transient read retry needs no reset/stop
  } else if(name=="runtime_recovery") {
    ready(imu);Wire.fail=true;run(imu,3,false);const auto last=imu.snapshot().lastSuccessMs;
    const unsigned writes=Wire.writes;run(imu,120,false);
    assert(!imu.snapshot().healthy && imu.snapshot().lastSuccessMs==last && Wire.writes==writes);
    Wire.fail=false;run(imu,200,true);assert(imu.snapshot().healthy && Wire.writes>writes);
  } else if(name=="moving_init" || name=="moving_mid_init") {
    imu.begin();if(name=="moving_mid_init") run(imu,3,true);
    const unsigned writes=Wire.writes;run(imu,100,false);
    assert(Wire.writes==writes && !imu.snapshot().healthy);
    run(imu,100,true);assert(imu.snapshot().healthy);
  } else if(name=="no_data_ready") {
    ready(imu);Wire.dataReady=false;const auto last=imu.snapshot().lastSuccessMs;
    run(imu,4,false);assert(imu.snapshot().lastSuccessMs==last);
    run(imu,20,false);assert(!imu.snapshot().healthy && imu.snapshot().lastSuccessMs==last);
    Wire.dataReady=true;run(imu,200,true);assert(imu.snapshot().healthy);
  } else if(name=="config_mismatch" || name=="reset_timeout") {
    Wire.badConfig=name=="config_mismatch";Wire.resetStuck=name=="reset_timeout";
    imu.begin();run(imu,300);assert(!imu.snapshot().healthy && !imu.snapshot().hasSample);
    assert(imu.snapshot().errorCount>0);Wire.badConfig=Wire.resetStuck=false;
    run(imu,500);assert(imu.snapshot().healthy);
  } else if(name=="backoff") {
    Wire.present=false;imu.begin();run(imu,4000);assert(!imu.snapshot().healthy);
    assert(Wire.reads<80);Wire.present=true;run(imu,400);assert(imu.snapshot().healthy);
  } else if(name=="clock_wrap") {
    testNow=0xfffffe00UL;ready(imu);run(imu,80);assert(imu.snapshot().healthy);
    Wire.fail=true;run(imu,3);Wire.fail=false;run(imu,300);assert(imu.snapshot().healthy);
  } else if(name=="zero_timestamp") {
    ready(imu);testNow=0;imu.update(true);
    assert(imu.snapshot().hasSample && imu.snapshot().lastSuccessMs==0);
    run(imu,3);assert(imu.snapshot().healthy);
  } else {
    DisplayManager display;assert(display.begin());DashboardData d=normal();
    display.updateDashboard(d);const unsigned initial=oledState.frames;
    if(name=="oled_bridge") {
      WebDashboard web;web._wifiTelemetry.staConnected=true;web._wifiTelemetry.staIp="10.0.0.9";
      web.copyDisplayNetworkState(d);assert(d.staConnected && d.staIp=="10.0.0.9");
      assert(d.temperature==28.4f && d.gasValue==850 && d.controlMode=="AUTO");
      web._wifiTelemetry.staConnected=false;web._wifiTelemetry.staIp="0.0.0.0";
      web.copyDisplayNetworkState(d);assert(!d.staConnected && d.staIp=="0.0.0.0");
    } else if(name=="oled_normal") {
      assert(contains("PIKU 2.1 Rev A\nDIST: 45.0 cm\nT:28.4C H:65%\nGAS: LOW (850)"));
      assert(contains("FLAME: CLEAR\nRADIO: DISCONNECTED\nSTA: 192.168.43.120"));
      assert(!contains("IMU") && !contains("MODE:"));
      size_t col=0;for(char c:oledState.text){if(c=='\n')col=0;else assert(++col<=21);}
    } else if(name=="oled_unchanged") {
      for(unsigned n=0;n<100;++n) displayAfter(display,d,100);
      assert(oledState.frames==initial);d.frontDistance=45.01f;displayAfter(display,d);
      assert(oledState.frames==initial); // different raw value, same visible text
      d.frontDistance=46;displayAfter(display,d,1);assert(oledState.frames==initial);
      displayAfter(display,d);assert(oledState.frames==initial+1);
    } else if(name=="oled_radio_sta") {
      d.radioInitialized=d.radioChipDetected=true;displayAfter(display,d);assert(contains("RADIO: DISCONNECTED"));
      d.radioPeer="ONLINE";displayAfter(display,d);assert(contains("RADIO: CONNECTED"));
      d.radioPeer="OFFLINE";d.staIp="10.0.0.8";displayAfter(display,d);
      assert(contains("RADIO: DISCONNECTED") && contains("STA: 10.0.0.8"));
      d.staConnected=false;displayAfter(display,d);assert(contains("STA: DISCONNECTED") && !contains("10.0.0.8"));
    } else if(name=="oled_alerts") {
      d.flameCritical=true;assert(display.alertUpdatePending(d));displayAfter(display,d,1);
      assert(contains("FLAME DETECTED") && !contains("DIST:"));
      d.gasCritical=d.temperatureCritical=true;displayAfter(display,d,1);
      assert(contains("FLAME DETECTED") && contains("GAS HIGH (850)") && contains("TEMP HIGH: 28.4 C"));
      assert(oledState.frames==initial+2);d.flameCritical=d.gasCritical=d.temperatureCritical=false;
      displayAfter(display,d,1);assert(contains("PIKU 2.1 Rev A") && oledState.frames==initial+3);
    } else if(name=="oled_distance") {
      d.safetyState="CAUTION";d.alarmReason="NEAR_OBSTACLE";d.frontDistance=10;
      displayAfter(display,d);assert(contains("DIST: 10.0 cm") && !contains("ALERT"));
      d.safetyState="CRITICAL";d.alarmReason="BOTH_IR_OBSTACLES";
      displayAfter(display,d);assert(!contains("ALERT"));
    } else if(name=="oled_invalid") {
      d.temperatureValid=d.humidityValid=d.flameSampleValid=d.distanceValid=false;
      d.mq2SampleAgeMs=100000;displayAfter(display,d);
      assert(contains("T:--C H:--%") && contains("GAS: UNAVAILABLE") && contains("FLAME: INVALID"));
      d.mq2Warmup=true;displayAfter(display,d);assert(contains("GAS: WARMING (850)"));
    } else return 2;
  }
  std::cout<<name<<" PASS (host double; not a physical measurement)\n";
}
