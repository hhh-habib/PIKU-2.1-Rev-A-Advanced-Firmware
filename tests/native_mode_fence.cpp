#include "NavigationUnderTest.h"
#include "ModeTransitionFence.h"
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <map>
#include <iostream>
#include <sstream>
struct ControlString : std::string {
  using std::string::string;
  ControlString(const std::string& s):std::string(s){}
  ControlString(unsigned long v):std::string(std::to_string(v)){}
  ControlString(unsigned long v,int):std::string(std::to_string(v)){}
  void toUpperCase(){for(auto& c:*this)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));}
  char charAt(unsigned i)const{return at(i);}
};
inline ControlString operator+(const std::string& text,unsigned long value){return text+std::to_string(value);}
inline bool isDigit(char c){return c>='0'&&c<='9';}
constexpr int HEX=16;
inline unsigned long esp_random(){static unsigned long value=100;return ++value;}
enum class BackendHead{None};
struct Rig {
  MotorController motors;SensorManager sensors;ImuManager imu;ServoScanner servo;SafetyManager safety;WebDashboard web;DashboardData data;String facing="FRONT";
  NavigationManager nav{motors,sensors,imu,servo,safety,web,data,facing,nullptr,nullptr,nullptr};
} robot;
class BackendWeb {public:
  struct Data{ControlString controlMode="MANUAL";bool manualFlameGasOverride=false;}_data;
  struct Server {
    std::map<std::string,ControlString> args,headers;ControlString responseBody;int reply=0;
    bool hasArg(const char* k)const{return args.count(k)!=0;}
    ControlString arg(const char* k)const{auto i=args.find(k);return i==args.end()?ControlString(""):i->second;}
    void sendHeader(const char* k,const ControlString& v){headers[k]=v;}
    void send(int n,const char*,const char* body){reply=n;responseBody=body;}
    void send(int n,const char*,const ControlString& body){reply=n;responseBody=body;}
  }_server;
  ModeTransitionFence _modeFence;bool _modeFenceResponsePending=false;
  ControlString _pendingCommand,_lastAcceptedCommand,_lastAppliedCommand,_lastStopReason;
  ControlString _sessionToken;unsigned long _lastSessionHeartbeatMs=0;
  BackendHead _pendingManualSensorHeadCommand=BackendHead::None;
  unsigned long _modeCommandCounter=0,_lastCommandSequence=0,_pendingCommandSequence=0,_lastCommandReceivedMs=0,_lastCommandAcceptedMs=0,_lastCommandAppliedMs=0;
  bool _controlActivity=false,_requireFreshSessionAfterOverride=false,origin=true;unsigned modeCalls=0;
  void (*_modeAcceptedCallback)(const ControlString&)=nullptr;
  void (*_commandAcceptedCallback)(const ControlString&,unsigned long,unsigned long)=nullptr;
  bool controlOriginAllowed(){return origin;}bool validSessionToken();bool sessionAlive()const;void prepareControlResponse(){}
  void sendError(int n,const char*,const char*){_server.reply=n;}
  void sendOk();void handleCommand();void handleMode();void handleModeAuto();void handleModeManual();bool acceptModeFence();
  void applyMode(const ControlString&);bool applyCommand(const ControlString&,unsigned long=0,bool=false);
  void markCommandApplied(const ControlString&,unsigned long,const ControlString&);
  void invalidateControlSession();void handleSession();
} backend;
void modeAccepted(const ControlString& mode){++backend.modeCalls;robot.web.mode=mode;++robot.web.modeCounter;robot.motors.stop();robot.nav.cancelForModeChange();}
void commandAccepted(const ControlString& command,unsigned long seq,unsigned long){
  if(command=="STOP"||command=="RELEASE"){
    robot.motors.stop();backend.markCommandApplied("STOP",seq,command=="STOP"?"OPERATOR_STOP":"MANUAL_RELEASE");
    if(command=="STOP"&&robot.web.mode=="AUTO")robot.nav.notifyOperatorStop();
  }
}
// ACTUAL_BACKEND_BODIES
ControlString stop(unsigned sequence=1){backend._server.args={{"move","STOP"},{"seq",ControlString(sequence)}};backend._server.headers.clear();backend.handleCommand();assert(backend._server.reply==200);return backend._server.headers["X-PIKU-Mode-Fence"];}
void mode(const ControlString& ticket){backend._server.args={{"set","AUTO"},{"fence",ticket}};backend.handleMode();}
int main(int argc,char** argv){assert(argc==2);const String name=argv[1];testNow=10000;backend._modeFence.reset(123);backend._modeAcceptedCallback=modeAccepted;backend._commandAcceptedCallback=commandAccepted;robot.sensors.feed(UltrasonicResultState::DistanceValid,80);
  if(name=="bridge"){
    backend.applyMode("AUTO");
    unsigned long now,sequence;std::string action,move,token,requestedMode,fence;
    while(std::cin>>now>>action>>move>>sequence>>token>>requestedMode>>fence){
      testNow=now;backend._server.args.clear();backend._server.headers.clear();backend._server.reply=200;
      if(token!="-")backend._server.args["token"]=token;
      if(action=="reset"){backend=BackendWeb();backend._modeFence.reset(123);backend._modeAcceptedCallback=modeAccepted;backend._commandAcceptedCallback=commandAccepted;backend.applyMode("AUTO");}
      else if(action=="session"||action=="other")backend.handleSession();
      else if(action=="cmd"){
        backend._server.args["move"]=move;backend._server.args["seq"]=ControlString(sequence);
        if(requestedMode=="retire")backend._server.args["retire"]="manual";
        backend.handleCommand();
      }else if(action=="mode"){
        backend._server.args["set"]=requestedMode;backend._server.args["fence"]=fence;backend.handleMode();
      }else if(action=="expire")backend._lastSessionHeartbeatMs=0;
      else if(action=="revoke")backend.invalidateControlSession();
      else if((action=="data"||action=="ping")&&backend.validSessionToken())backend._lastSessionHeartbeatMs=testNow;
      std::cout<<"{\"status\":"<<backend._server.reply
        <<",\"sequence\":"<<backend._lastCommandSequence
        <<",\"token\":\""<<backend._sessionToken<<"\",\"mode\":\""<<backend._data.controlMode
        <<"\",\"alive\":"<<(backend.sessionAlive()?"true":"false")
        <<",\"revoked\":"<<(backend._requireFreshSessionAfterOverride?"true":"false")
        <<",\"accepted\":\""<<backend._lastAcceptedCommand
        <<"\",\"state\":\""<<backend._server.headers["X-PIKU-Control-State"]
        <<"\",\"fence\":\""<<backend._server.headers["X-PIKU-Mode-Fence"]<<"\"}"<<std::endl;
    }return 0;
  }
  if(name=="revoked_recovery_proof"){
    backend._server.args.clear();backend.handleSession();const auto token=backend._sessionToken;
    backend.invalidateControlSession();stop(10);
    assert(backend._server.headers["X-PIKU-Control-State"]=="MANUAL_REVOKED");
    assert(backend._requireFreshSessionAfterOverride&&backend._sessionToken.empty());
    backend._server.args={{"move","FORWARD"},{"seq","11"},{"token",token}};backend.handleCommand();assert(backend._server.reply==409);
  }
  if(name=="held_session"){
    backend.handleSession();const auto token=backend._sessionToken;
    for(unsigned n=0;n<40;++n){testNow+=250;
      backend._server.args={{"move","FORWARD"},{"seq",ControlString(n+2)},{"token",token}};
      backend.handleCommand();assert(backend._server.reply==200&&backend.sessionAlive());
    }
    assert(backend._sessionToken==token);
  }
  if(name=="stale_session"){
    backend.handleSession();const auto token=backend._sessionToken;const auto stamp=backend._lastSessionHeartbeatMs;
    backend._lastCommandSequence=100;testNow+=2000;
    backend._server.args={{"move","FORWARD"},{"seq","99"},{"token",token}};backend.handleCommand();
    assert(backend._server.reply==409&&backend._lastSessionHeartbeatMs==stamp);
    testNow+=501;assert(!backend.sessionAlive());
  }
  if(name=="other_session"){
    backend.handleSession();const auto stamp=backend._lastSessionHeartbeatMs;testNow+=2000;
    backend._server.args={{"move","FORWARD"},{"seq","2"},{"token","other-owner"}};backend.handleCommand();
    assert(backend._lastSessionHeartbeatMs==stamp);testNow+=501;assert(!backend.sessionAlive());
  }
  if(name=="expired_session"){
    backend.handleSession();const auto token=backend._sessionToken;const auto stamp=backend._lastSessionHeartbeatMs;
    testNow+=2501;backend._server.args={{"move","FORWARD"},{"seq","2"},{"token",token}};backend.handleCommand();
    assert(backend._lastSessionHeartbeatMs==stamp&&!backend.sessionAlive());
  }
  if(name=="normal"){const auto t=stop();assert(!t.empty());mode(t);assert(backend._server.reply==200&&backend.modeCalls==1);mode(t);assert(backend._server.reply==409&&backend.modeCalls==1);}
  if(name=="late_stop"){backend.applyMode("AUTO");const auto t=stop();stop(2);const auto calls=backend.modeCalls;mode(t);assert(backend._server.reply==409&&backend.modeCalls==calls);assert(robot.nav._operatorStopLatched);robot.nav.updateAuto();assert(robot.motors.motion==MotorState::Stopped&&robot.motors.forwards==0);}
  if(name=="expiry"){const auto t=stop();testNow+=2000;mode(t);assert(backend._server.reply==409&&backend.modeCalls==0);}
  if(name=="release"){const auto t=stop();backend._server.args={{"move","RELEASE"},{"seq","2"}};backend.handleCommand();mode(t);assert(backend._server.reply==409&&backend.modeCalls==0);}
  if(name=="safety"){const auto t=stop();backend.markCommandApplied("STOP",1,"CRITICAL_SAFETY");mode(t);assert(backend._server.reply==409&&backend.modeCalls==0);}
  if(name=="duplicate_apply"){const auto t=stop();backend.markCommandApplied("STOP",1,"OPERATOR_STOP");mode(t);assert(backend._server.reply==200&&backend.modeCalls==1);}
  if(name=="stale_stop"){backend._lastCommandSequence=100;const auto t=stop(101);stop(1);mode(t);assert(backend._server.reply==409&&backend.modeCalls==0);}
  if(name=="reboot"){const auto t=stop();backend._modeFence.reset(124);mode(t);assert(backend._server.reply==409&&backend.modeCalls==0);}
  if(name=="aliases"){backend._server.args.clear();backend.handleModeAuto();assert(backend._server.reply==409&&backend.modeCalls==0);auto t=stop();backend._server.args={{"fence",t}};backend.handleModeManual();assert(backend._server.reply==200&&backend.modeCalls==1);}
  if(name=="missing"){backend._server.args={{"set","AUTO"}};backend.handleMode();assert(backend._server.reply==409&&backend.modeCalls==0);}
  if(name=="origin"){const auto t=stop();backend.origin=false;mode(t);assert(backend.modeCalls==0);backend.origin=true;mode(t);assert(backend._server.reply==200&&backend.modeCalls==1);}
  if(name=="old_direction"){stop(5);backend._server.args={{"move","FORWARD"},{"seq","4"}};backend.handleCommand();assert(backend._server.reply==409);backend._server.args={{"move","RELEASE"},{"seq","6"}};backend.handleCommand();backend._server.args={{"move","FORWARD"},{"seq","5"}};backend.handleCommand();assert(backend._server.reply==409&&robot.motors.motion==MotorState::Stopped);}
  if(name=="late_manual"){const auto t=stop();stop(2);backend._server.args={{"set","MANUAL"},{"fence",t}};backend.handleMode();assert(backend._server.reply==409&&backend.modeCalls==0&&robot.motors.motion==MotorState::Stopped);}
  if(name=="revoked_session"){const auto t=stop();backend._sessionToken="old";backend.invalidateControlSession();mode(t);assert(backend._server.reply==409&&backend.modeCalls==0&&backend._sessionToken.empty());backend._server.args={{"set","MANUAL"},{"fence",t}};backend.handleMode();assert(backend._server.reply==409&&backend.modeCalls==0);}
  if(name=="multiple_clients"){const auto first=stop(1);const auto second=stop(2);mode(first);assert(backend._server.reply==409&&backend.modeCalls==0);mode(second);assert(backend._server.reply==200&&backend.modeCalls==1);backend._server.args={{"set","MANUAL"},{"fence",second}};backend.handleMode();assert(backend._server.reply==409&&backend.modeCalls==1);}
  if(name=="session_replacement"){const auto t=stop();backend._server.args.clear();backend.handleSession();assert(backend._server.reply==200&&!backend._sessionToken.empty()&&robot.motors.motion==MotorState::Stopped);mode(t);assert(backend._server.reply==409&&backend.modeCalls==0);const auto fresh=stop(5);mode(fresh);assert(backend._server.reply==200&&backend.modeCalls==1);}
  if(name=="session_sequence"){
    stop(5);backend._server.args.clear();backend.handleSession();assert(backend._lastCommandSequence==6);
    assert(backend._server.responseBody.find("\"commandSequence\":6")!=std::string::npos);
    backend._server.args={{"move","FORWARD"},{"seq","6"}};backend.handleCommand();assert(backend._server.reply==409);
    backend._server.args={{"move","FORWARD"},{"seq","7"}};backend.handleCommand();assert(backend._server.reply==200);
  }
  if(name=="stop_authority"){
    backend._server.args.clear();backend.handleSession();const auto token=backend._sessionToken;
    backend._server.args={{"move","STOP"},{"seq","10"},{"token",token}};backend.handleCommand();
    assert(backend._server.headers["X-PIKU-Control-State"]=="MANUAL_AUTHORIZED");
    backend._server.args={{"move","STOP"},{"seq","11"},{"token","other-browser"}};backend.handleCommand();
    assert(backend._server.headers["X-PIKU-Control-State"]=="MANUAL_OTHER_SESSION");
    backend._lastSessionHeartbeatMs=0;stop(12);assert(backend._server.headers["X-PIKU-Control-State"]=="MANUAL_SESSION_EXPIRED");
  }
  if(name=="stop_sequence_sync"){
    backend._lastCommandSequence=100;stop(1);assert(backend._server.headers["X-PIKU-Command-Sequence"]=="100");
    backend._server.args={{"move","FORWARD"},{"seq","101"}};backend.handleCommand();assert(backend._server.reply==200);
  }
}
