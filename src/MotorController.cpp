#include "MotorController.h"
#include "RobotConfig.h"

// Tested L298N input transition guard: coast before changing direction.
static constexpr unsigned long DIRECTION_SETTLE_MS = 5;
static constexpr bool MOTOR_DEBUG = false;
MotorController::MotorController(int rightIn1, int rightIn2, int leftIn1, int leftIn2, int motorEnablePwm)
  : _rightIn1(rightIn1),
    _rightIn2(rightIn2),
    _leftIn1(leftIn1),
    _leftIn2(leftIn2),
    _motorEnablePwm(motorEnablePwm),
    _speedPercent(0),
    _initialized(false),
    currentState(MotorState::Stopped) {
}

void MotorController::begin() {
  pinMode(_rightIn1, OUTPUT);
  pinMode(_rightIn2, OUTPUT);
  pinMode(_leftIn1, OUTPUT);
  pinMode(_leftIn2, OUTPUT);
  pinMode(_motorEnablePwm, OUTPUT);

  // Keep the shared bridge enable and every direction input inactive before LEDC setup.
  digitalWrite(_motorEnablePwm, LOW);

  coast();
  ledcSetup(RobotConfig::MOTOR_PWM_CHANNEL,
            RobotConfig::MOTOR_PWM_FREQUENCY_HZ,
            RobotConfig::MOTOR_PWM_RESOLUTION_BITS);
  ledcAttachPin(_motorEnablePwm, RobotConfig::MOTOR_PWM_CHANNEL);
  writeEnableDuty(0);
  currentState = MotorState::Stopped;
  _initialized = true;

  // Store the configured default only after the hardware is safely stopped.
  _speedPercent = RobotConfig::DEFAULT_MOTOR_SPEED_PERCENT;
}

void MotorController::setSpeedPercent(int speedPercent) {
  _speedPercent = constrain(speedPercent, 0, 100);
  if (_initialized) {
    applySelectedSpeed();
  }
}

int MotorController::getSpeedPercent() const {
  return _speedPercent;
}

uint32_t MotorController::selectedDuty() const {
  const int effectivePercent = currentState == MotorState::Backward
    ? min(_speedPercent, _backwardSpeedCapPercent) : _speedPercent;
  if (effectivePercent <= 0) {
    return 0;
  }

  const uint32_t maxDuty = (1UL << RobotConfig::MOTOR_PWM_RESOLUTION_BITS) - 1UL;
  static constexpr uint32_t USER_SPEED_SPAN = 99UL;
  static constexpr uint32_t PERCENT_SCALE = 100UL;
  const uint32_t calibratedPercentNumerator =
    static_cast<uint32_t>(RobotConfig::MOTOR_DRIVE_MIN_PWM_PERCENT) * USER_SPEED_SPAN +
    static_cast<uint32_t>(effectivePercent - 1) *
      (PERCENT_SCALE - RobotConfig::MOTOR_DRIVE_MIN_PWM_PERCENT);
  const uint32_t denominator = PERCENT_SCALE * USER_SPEED_SPAN;
  return (calibratedPercentNumerator * maxDuty + denominator / 2UL) / denominator;
}

void MotorController::writeEnableDuty(uint32_t duty) {
  ledcWrite(RobotConfig::MOTOR_PWM_CHANNEL, duty);
}

void MotorController::applySelectedSpeed() {
  if (currentState == MotorState::Stopped) {
    writeEnableDuty(0);
    return;
  }

  uint32_t duty = selectedDuty();
  const bool needsTurnMinimum =
    currentState == MotorState::LeftTurn ||
    currentState == MotorState::RightTurn ||
    currentState == MotorState::SpinLeft ||
    currentState == MotorState::SpinRight;
  if (duty > 0 && needsTurnMinimum) {
    const uint32_t maxDuty = (1UL << RobotConfig::MOTOR_PWM_RESOLUTION_BITS) - 1UL;
    const uint32_t turnMinimumDuty =
      (static_cast<uint32_t>(RobotConfig::MOTOR_TURN_MIN_PWM_PERCENT) * maxDuty + 99UL) / 100UL;
    if (duty < turnMinimumDuty) {
      duty = turnMinimumDuty;
    }
  }
  writeEnableDuty(duty);
}

void MotorController::setState(MotorState state) {
  currentState = state;
  logState(stateToString(state));
}

MotorState MotorController::getState() const {
  return currentState;
}

const char* MotorController::getStateName() const {
  return stateToString(currentState);
}

const char* MotorController::stateToString(MotorState state) {
  switch (state) {
    case MotorState::Forward:
      return "FORWARD";
    case MotorState::Backward:
      return "BACKWARD";
    case MotorState::LeftTurn:
    case MotorState::SpinLeft:
      return "LEFT";
    case MotorState::RightTurn:
    case MotorState::SpinRight:
      return "RIGHT";
    case MotorState::Stopped:
    default:
      return "STOP";
  }
}

void MotorController::logState(const char* stateName) const {
  if (MOTOR_DEBUG) {
    Serial.print("[MOTOR] ");
    Serial.println(stateName);
  }
}

void MotorController::coast() {
  digitalWrite(_rightIn1, LOW);
  digitalWrite(_rightIn2, LOW);
  digitalWrite(_leftIn1, LOW);
  digitalWrite(_leftIn2, LOW);
}

void MotorController::settleBeforeDirectionChange() {
  writeEnableDuty(0);
  coast();
  delay(DIRECTION_SETTLE_MS);
}

void MotorController::energizeRightForward() {
  digitalWrite(_rightIn1, HIGH);
  digitalWrite(_rightIn2, LOW);
}

void MotorController::energizeLeftForward() {
  digitalWrite(_leftIn1, HIGH);
  digitalWrite(_leftIn2, LOW);
}

void MotorController::energizeRightBackward() {
  digitalWrite(_rightIn1, LOW);
  digitalWrite(_rightIn2, HIGH);
}

void MotorController::energizeLeftBackward() {
  digitalWrite(_leftIn1, LOW);
  digitalWrite(_leftIn2, HIGH);
}

void MotorController::stop() {
  // STOP is always authoritative: disable bridge outputs before direction changes.
  if (_initialized) {
    writeEnableDuty(0);
  }
  coast();
  setState(MotorState::Stopped);
}

void MotorController::forward() {
  if (currentState == MotorState::Forward) {
    applySelectedSpeed();
    return;
  }

  settleBeforeDirectionChange();
  energizeLeftForward();
  energizeRightForward();
  setState(MotorState::Forward);
  applySelectedSpeed();
}

void MotorController::backward() {
  backwardWithSpeedCap(100);
}

void MotorController::backwardWithSpeedCap(int maxPercent) {
  // Keep the selected preference; repeated pulse refreshes and speed edits use this cap.
  _backwardSpeedCapPercent = constrain(maxPercent, 0, 100);
  if (currentState == MotorState::Backward) {
    applySelectedSpeed();
    return;
  }

  settleBeforeDirectionChange();
  energizeLeftBackward();
  energizeRightBackward();
  setState(MotorState::Backward);
  applySelectedSpeed();
}

void MotorController::leftTurn() {
  settleBeforeDirectionChange();
  energizeRightForward();
  digitalWrite(_leftIn1, LOW);
  digitalWrite(_leftIn2, LOW);
  setState(MotorState::LeftTurn);
  applySelectedSpeed();
}

void MotorController::rightTurn() {
  settleBeforeDirectionChange();
  digitalWrite(_rightIn1, LOW);
  digitalWrite(_rightIn2, LOW);
  energizeLeftForward();
  setState(MotorState::RightTurn);
  applySelectedSpeed();
}

void MotorController::spinLeft() {
  settleBeforeDirectionChange();
  energizeLeftBackward();
  energizeRightForward();
  setState(MotorState::SpinLeft);
  applySelectedSpeed();
}

void MotorController::spinRight() {
  settleBeforeDirectionChange();
  energizeLeftForward();
  energizeRightBackward();
  setState(MotorState::SpinRight);
  applySelectedSpeed();
}
