#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H

#include <Arduino.h>

enum class MotorState {
  Stopped,
  Forward,
  Backward,
  LeftTurn,
  RightTurn,
  SpinLeft,
  SpinRight
};

class MotorController {
public:
  MotorController(int rightIn1, int rightIn2, int leftIn1, int leftIn2, int motorEnablePwm);

  void begin();
  void setSpeedPercent(int speedPercent);
  int getSpeedPercent() const;
  void stop();
  void forward();
  void backward();
  void backwardWithSpeedCap(int maxPercent);
  void leftTurn();
  void rightTurn();
  void spinLeft();
  void spinRight();

  MotorState getState() const;
  const char* getStateName() const;
  static const char* stateToString(MotorState state);

private:
  int _rightIn1;
  int _rightIn2;
  int _leftIn1;
  int _leftIn2;
  int _motorEnablePwm;

  int _speedPercent;
  int _backwardSpeedCapPercent = 100;
  bool _initialized;
  MotorState currentState;

  uint32_t selectedDuty() const;
  void writeEnableDuty(uint32_t duty);
  void applySelectedSpeed();
  void coast();
  void settleBeforeDirectionChange();
  void energizeRightForward();
  void energizeLeftForward();
  void energizeRightBackward();
  void energizeLeftBackward();
  void setState(MotorState state);
  void logState(const char* stateName) const;
};

#endif // MOTORCONTROLLER_H
