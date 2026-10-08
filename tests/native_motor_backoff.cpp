#include "MotorController.h"
#include "RobotConfig.h"
#include <cassert>
#include <vector>
std::vector<uint32_t> duties;
unsigned delayCalls=0;
void pinMode(int,int){} void digitalWrite(int,int){}
void ledcSetup(int,int,int){} void ledcAttachPin(int,int){}
void ledcWrite(int,uint32_t duty){duties.push_back(duty);}
void delay(unsigned long){++delayCalls;}
NativeSerial Serial;
int main(){
 static_assert(RobotConfig::RECOVERY_BACKOFF_MAX_SPEED_PERCENT==75,"75 user-percent maximum");
 static_assert(RobotConfig::RECOVERY_IR_BACKOFF_PULSE_MS==200,"pulse unchanged");
 MotorController motor(1,2,3,4,5);motor.begin();
 for(int selected:{0,40,60,75,80,100}){
  const int effective=selected<75?selected:75;
  motor.stop();motor.setSpeedPercent(effective);motor.backward();const auto reference=duties.back();
  motor.stop();motor.setSpeedPercent(selected);motor.backward();const auto uncapped=duties.back();
  motor.stop();motor.backwardWithSpeedCap(75);assert(duties.back()==reference);assert(motor.getSpeedPercent()==selected);
  const unsigned settled=delayCalls;
  for(int refresh=0;refresh<8;++refresh){motor.backwardWithSpeedCap(75);assert(duties.back()==reference);assert(motor.getSpeedPercent()==selected);}
  assert(delayCalls==settled); // No new blocking settle on refresh.
  if(selected==100){assert(reference!=((1U<<RobotConfig::MOTOR_PWM_RESOLUTION_BITS)-1)*75/100);motor.setSpeedPercent(100);assert(duties.back()==reference);}
  for(int pulse=0;pulse<5;++pulse){motor.stop();motor.backwardWithSpeedCap(75);assert(duties.back()==reference&&motor.getSpeedPercent()==selected);}
  motor.stop();assert(duties.back()==0&&motor.getSpeedPercent()==selected);
  motor.backward();assert(duties.back()==uncapped); // Normal Manual Backward is not capped.
  motor.forward();assert(duties.back()==uncapped);
  motor.spinLeft();const auto turn=duties.back();motor.stop();motor.spinLeft();assert(duties.back()==turn);
  motor.stop();
 }
}
