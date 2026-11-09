#include "ResponseTypes.h"
#include "RobotConfig.h"
#include "StaReconnectPolicy.h"
#include <cassert>
#include <iostream>

constexpr int SH110X_WHITE=1;
class DisplayManager { public:
  struct Oled { unsigned frames=0; String text; void clearDisplay(){text.clear();} void setTextSize(int){}
    void setTextColor(int){} void setCursor(int,int){} void display(){++frames;}
    void println(const String& value){text+=value+'\n';} void println(){text+='\n';} } oled_;
  bool initialized_=true, _alertVisible=false;
  String _lastAlertReason;
  uint8_t page_=0;
  unsigned long lastRefreshMs_=0,lastPageMs_=0;
  bool hasFrame_=false;
  uint8_t lastAlertMask_=0;
  char lines_[8][22]{};
  unsigned alerts=0,pages=0;
  bool alertUpdatePending(const DashboardData&) const;
  void updateDashboard(const DashboardData&);
  void renderAlertContents(const DashboardData&);
  void renderAlert(const DashboardData& d){++alerts;renderAlertContents(d);}
  void renderPage(const DashboardData&){++pages;}
};
class DashboardPublisher { public:
  struct {void copyDisplayNetworkState(DashboardData&) const {}} dashboard;
  DisplayManager& display; DashboardData& data;
  DashboardPublisher(DisplayManager& d,DashboardData& v):display(d),data(v){}
  void publishIfChanged(){}
  template<class... A>void logSensorReadings(A...){}
  // PUBLISH_SIGNATURE
};
enum { WL_CONNECTED=3, WL_CONNECT_FAILED=4, WL_NO_SSID_AVAIL=1,
       WL_IDLE_STATUS=0, WL_CONNECTION_LOST=5, WL_DISCONNECTED=6, WIFI_AP_STA=3, WIFI_SCAN_RUNNING=-1 };
using wl_status_t=int;
struct WifiDouble { int state=WL_IDLE_STATUS,scan=0; unsigned begins=0,reconnects=0;
  int status()const{return state;} int getMode()const{return WIFI_AP_STA;}
  void mode(int){} int scanComplete()const{return scan;}
  bool reconnect(){++reconnects;return true;}
} WiFi;
class WebDashboard { public:
  enum class StaState {NotConfigured,Connecting,Connected,AuthFailed,NoAp,Disconnected,Scanning};
  StaState _staState=StaState::Connecting;
  bool _staConfigured=true,_staConnectStarted=true,_staCredentialsPendingPersistence=false,_scanStarted=false;
  unsigned long _lastWiFiUpdateMs=0,_lastStaAttemptMs=8000,_staRetryIntervalMs=1500,_wifiReconnectCount=0;
  String _staSsid,_staPassword;
  struct Preferences {void begin(const char*,bool){} void putString(const char*,const String&){} void end(){}} _preferences;
  void updateWiFi(); void setStaStateFromWiFiStatus();
  void beginStaAssociation(){++WiFi.begins;}
  void refreshWiFiTelemetry(bool){} void updateMdns(bool){}
};
// LIVE_BODIES
void publish(DashboardPublisher& publisher,const String& state,const String& reason,bool slow){
  SensorSnapshot sensor;
  publisher.publishSensorReadings("FRONT",90,sensor,false,false,60,"CLEAR","LOW_RELATIVE",
    state,reason,false,"NONE",false,"ENABLED","IDLE","MANUAL","PIVOT","STOP","STOP","IDLE",slow);
}
int main(int argc,char** argv){
  assert(argc==2);const String name=argv[1];
  DisplayManager display;DashboardData data;DashboardPublisher publisher(display,data);
  if(name=="alert_fast"){
    display.updateDashboard(data);testNow+=1;data.safetyState="CRITICAL";data.alarmReason="FLAME";
    display.updateDashboard(data);assert(display.alerts==1&&display. oled_.frames==2);
  }else if(name=="alert_reason"){
    data.safetyState="CRITICAL";data.alarmReason="FLAME";display.updateDashboard(data);
    ++testNow;data.alarmReason="GAS_HIGH_RELATIVE";display.updateDashboard(data);assert(display.alerts==2);
  }else if(name=="alert_busy"){
    publish(publisher,"SAFE","NONE",true);++testNow;publish(publisher,"CRITICAL","FLAME",false);
    assert(display.alerts==1&&data.alarmReason=="FLAME");
  }else if(name=="alert_clear"){
    publish(publisher,"CRITICAL","FLAME",false);++testNow;publish(publisher,"SAFE","NONE",false);
    assert(display.alerts==1&&display.pages==1);
  }else if(name=="alert_repeat"){
    publish(publisher,"CRITICAL","FLAME",false);
    for(unsigned n=0;n<100;++n){++testNow;publish(publisher,"CRITICAL","FLAME",false);}
    assert(display.alerts==1);
  }else if(name=="alert_override"){
    data.safetyState="CRITICAL";data.alarmReason="FLAME";data.motorState="FORWARD";
    data.manualFlameGasOverride=true;display.updateDashboard(data);
    assert(display.oled_.text.find("FLAME")!=String::npos);
    assert(display.oled_.text.find("MOTORS STOPPED")==String::npos);
  }else if(name=="wifi_pending"){
    WebDashboard web;web.updateWiFi();assert(WiFi.reconnects==0&&WiFi.begins==0&&web._lastStaAttemptMs==8000);
  }else if(name=="wifi_expired"){
    WebDashboard web;testNow=30001;web.updateWiFi();assert(WiFi.begins==1&&WiFi.reconnects==0);
    assert(web._lastStaAttemptMs==testNow&&web._staRetryIntervalMs==3000);
  }else if(name=="wifi_connected"){
    WebDashboard web;WiFi.state=WL_CONNECTED;web._staRetryIntervalMs=8000;web.updateWiFi();
    assert(!web._staConnectStarted&&web._staRetryIntervalMs==1500&&WiFi.begins==0&&WiFi.reconnects==0);
  }else if(name=="wifi_scan"){
    WebDashboard web;web._scanStarted=true;WiFi.scan=WIFI_SCAN_RUNNING;web.updateWiFi();
    assert(WiFi.begins==0&&WiFi.reconnects==0);
  }else if(name=="wifi_lost"){
    WebDashboard web;WiFi.state=WL_CONNECTION_LOST;web.updateWiFi();
    assert(WiFi.begins==1&&WiFi.reconnects==0);
  }else return 2;
}
