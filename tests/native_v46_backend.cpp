#include "NavigationUnderTest.h"
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <map>
#include "ModeTransitionFence.h"
struct ControlString:std::string {using std::string::string;ControlString(const std::string& s):std::string(s){}void toUpperCase(){for(auto& c:*this)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));}char charAt(unsigned i)const{return at(i);}};
inline bool isDigit(char c){return c>='0'&&c<='9';}
enum class BackendHead {None};enum class BackendSource {None,Dashboard,Radio};
struct BackendHarness {
 MotorController motors;SensorManager sensors;ImuManager imu;ServoScanner servo;SafetyManager safety;WebDashboard web;DashboardData data;String facing="FRONT";
 NavigationManager nav{motors,sensors,imu,servo,safety,web,data,facing,nullptr,nullptr,nullptr
#ifdef CPS_COLLISION
 ,[](){return true;}
#endif
 };
 void fresh(){sensors.feed(UltrasonicResultState::DistanceValid,80);imu.data.initialized=imu.data.healthy=true;imu.data.sampleMs=imu.data.lastSuccessMs=testNow;}
} robot;
class BackendWeb {public:
 ModeTransitionFence _modeFence; bool _modeFenceResponsePending=false;
 struct Data{ControlString controlMode="MANUAL";bool manualFlameGasOverride=false;}_data;
 struct Server{std::map<std::string,ControlString> args;bool hasArg(const char* k)const{return args.count(k)!=0;}ControlString arg(const char* k)const{auto i=args.find(k);return i==args.end()?ControlString(""):i->second;}}_server;
 ControlString _pendingCommand="",_lastAcceptedCommand="";BackendHead _pendingManualSensorHeadCommand=BackendHead::None;BackendSource _pendingCommandSource=BackendSource::None;
 unsigned long _modeCommandCounter=0,_lastCommandSequence=0,_pendingCommandSequence=0,_lastCommandReceivedMs=0,_lastCommandAcceptedMs=0,_lastSessionHeartbeatMs=0;
 bool _controlActivity=false,_requireFreshSessionAfterOverride=false,_overrideCommandAuthorized=false;int reply=0;bool origin=true;unsigned callbacks=0;
 void (*_modeAcceptedCallback)(const ControlString&)=nullptr;
 void (*_commandAcceptedCallback)(const ControlString&,unsigned long,unsigned long)=nullptr;
 bool controlOriginAllowed(){return origin;}bool validSessionToken(){return true;}void prepareControlResponse(){}
 void sendError(int n,const char*,const char*){reply=n;}void sendOk(){reply=200;}
 void handleCommand();void applyMode(const ControlString&);bool applyCommand(const ControlString&,unsigned long=0,bool=false);
} backend;
void modeAccepted(const ControlString& mode){robot.web.mode=mode;++robot.web.modeCounter;robot.motors.stop();robot.nav.cancelForModeChange();}
void commandAccepted(const ControlString& command,unsigned long,unsigned long){++backend.callbacks;if(command=="STOP"||command=="RELEASE"){robot.motors.stop();if(command=="STOP"&&robot.web.mode=="AUTO")robot.nav.notifyOperatorStop();}}
// ACTUAL_BACKEND_BODIES
void request(bool maintenance,const char* tag="manual"){backend._server.args={{"move","STOP"},{"seq","1"}};if(maintenance)backend._server.args["retire"]=tag;backend.handleCommand();}
int main(int argc,char** argv){assert(argc==2);const String name=argv[1];backend._modeAcceptedCallback=modeAccepted;backend._commandAcceptedCallback=commandAccepted;testNow=10000;robot.fresh();robot.web.mode="MANUAL";
 if(name=="reorder")for(bool modeFirst:{false,true}){backend.applyMode("MANUAL");robot.motors.backward();const auto calls=backend.callbacks;
  if(!modeFirst)request(true);backend.applyMode("AUTO");robot.safety.left=true;robot.nav.updateAuto();assert(robot.motors.motion==MotorState::Stopped&&robot.nav._phase==NavigationManager::AutoPhase::ReversePrepare);
  const auto writes=robot.motors.spins;if(modeFirst)request(true);testNow++;robot.fresh();robot.nav.updateAuto();
  assert(!robot.nav._operatorStopLatched&&robot.motors.motion==MotorState::Stopped&&robot.nav._phase==NavigationManager::AutoPhase::ReversePrepare&&robot.motors.spins==writes);if(modeFirst)assert(backend.callbacks==calls);else assert(backend.callbacks==calls+1);
 }
 if(name=="explicit")for(const char* tag:{"","auto","garbage"}){backend.applyMode("AUTO");backend._lastCommandSequence=100;robot.fresh();robot.nav.updateAuto();if(*tag)request(true,tag);else request(false);assert(robot.nav._operatorStopLatched&&robot.motors.motion==MotorState::Stopped);const auto writes=robot.motors.spins+robot.motors.backwards+robot.motors.forwards;robot.nav.updateAuto();assert(robot.motors.spins+robot.motors.backwards+robot.motors.forwards==writes);}
 if(name=="manual"){backend.applyMode("MANUAL");robot.motors.backward();request(true);assert(robot.motors.motion==MotorState::Stopped&&!robot.nav._operatorStopLatched&&backend.reply==200);backend.origin=false;backend._server.args={{"move","STOP"}};const auto calls=backend.callbacks;backend.handleCommand();assert(backend.callbacks==calls);}
}

