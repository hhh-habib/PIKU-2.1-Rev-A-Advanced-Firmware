#pragma once
#include "NativeTypes.h"
#include "RobotConfig.h"
class MotorController { public:
 MotorState motion=MotorState::Stopped; unsigned spins=0,backwards=0,cappedBackwards=0; int speed=60,effectiveReverse=0;
 void stop(){motion=MotorState::Stopped;} void forward(){motion=MotorState::Forward;}
 void backward(){motion=MotorState::Backward;++backwards;}
 void backwardWithSpeedCap(int cap){backward();++cappedBackwards;effectiveReverse=speed<cap?speed:cap;}
 void spinLeft(){motion=MotorState::SpinLeft;++spins;} void spinRight(){motion=MotorState::SpinRight;++spins;}
};
class SensorManager { public:
 SensorSnapshot data; UltrasonicSample sample; unsigned requests=0;
 void setDistanceFacingFront(bool f){data.distanceFacingFront=f;} bool isDistanceSampleInProgress() const{return false;}
 unsigned long distanceSampleSequence() const{return sample.sequence;}
 unsigned long requestDistanceSample(bool f){setDistanceFacingFront(f);++requests;return sample.sequence+1;}
 UltrasonicSample latestUltrasonicSample() const{return sample;}
 const SensorSnapshot& snapshot() const{return data;}
 void feed(UltrasonicResultState state,float cm=60){sample.state=state;sample.distanceCm=cm;sample.valid=state==UltrasonicResultState::DistanceValid;sample.sampleMs=millis();sample.facingFront=true;++sample.sequence;data.ultrasonicState=state;data.distanceFacingFront=true;}
};
class ImuManager {public: ImuSnapshot data; const ImuSnapshot& snapshot()const{return data;}};
class ServoScanner {public: unsigned commands=0;int angle=90;void center(){angle=90;++commands;}void lookLeft(){angle=170;++commands;}void lookRight(){angle=10;++commands;}void detach(){}};
class SafetyManager {public:bool left=false,right=false,hazard=false,critical=false;
 bool isLeftIrObstacleActive()const{return left;}bool isRightIrObstacleActive()const{return right;}
 bool hasNonRecoverableCriticalHazard()const{return critical;}bool isAutoRecoverableEnvironmentalHazard()const{return hazard;}
 bool isCritical()const{return critical||hazard;}
};
class WebDashboard {public:String mode="AUTO",pending="",decision;unsigned long modeCounter=0;
 const String& getControlMode()const{return mode;}const String& getPendingCommand()const{return pending;}
 void setPendingCommand(const String& c){pending=c;}void setNavigationDecision(const String& d){decision=d;}
 unsigned long modeCommandCounter()const{return modeCounter;}
};
