#ifndef RECOVERY_IR_BACKOFF_POLICY_H
#define RECOVERY_IR_BACKOFF_POLICY_H
#include <stdint.h>
#include "RobotConfig.h"

// Fixed, nonblocking reverse pulses. The mission budget also bounds repeated
// close-sonar events when the robot has no rear obstacle sensor.
class RecoveryIrBackoffPolicy {
public:
  static constexpr uint32_t PULSE_MS = RobotConfig::RECOVERY_IR_BACKOFF_PULSE_MS;
  static constexpr uint8_t MAX_PULSES = RobotConfig::RECOVERY_IR_BACKOFF_MAX_PULSES;
  enum class Action { None, Backward, Stop, FrontCheck, Failed };

  Action start(uint32_t now, bool requireFirstPulse=false) {
    if (stage_ != Stage::Idle) return Action::None;
    eventPulses_=0;
    requireFirstPulse_=requireFirstPulse;
    return launch(now);
  }
  Action update(uint32_t now, bool irActive) {
    if (stage_ == Stage::Pulse) {
      if (!irActive && !requireFirstPulse_) {
        clearEvent();
        return Action::FrontCheck;
      }
      if (now-pulseStartedMs_ < PULSE_MS) return Action::None;
      stage_=Stage::Check;
      requireFirstPulse_=false;
      return Action::Stop;
    }
    if (stage_ == Stage::Check) {
      if (!irActive) {
        clearEvent();
        return Action::FrontCheck;
      }
      if (eventPulses_ >= MAX_PULSES || missionPulses_ >= MAX_PULSES) {
        clearEvent();
        return Action::Failed;
      }
      return launch(now);
    }
    return Action::None;
  }
  // Called after the capped recovery reverse output has been applied.
  void pulseCommanded(uint32_t now) {
    if (stage_ == Stage::Pulse) pulseStartedMs_=now;
  }
  void cancelPulse() { clearEvent(); } // preserve episode pulse count
  void resetMission() {
    clearEvent();
    missionPulses_=0;
  }
  bool active() const { return stage_ != Stage::Idle; }
  uint8_t eventPulses() const { return eventPulses_; }
  uint8_t missionPulses() const { return missionPulses_; }

private:
  enum class Stage { Idle, Pulse, Check };
  Stage stage_=Stage::Idle;
  uint32_t pulseStartedMs_=0;
  uint8_t eventPulses_=0;
  uint8_t missionPulses_=0;
  bool requireFirstPulse_=false;

  Action launch(uint32_t now) {
    if (missionPulses_ >= MAX_PULSES) {
      clearEvent();
      return Action::Failed;
    }
    ++eventPulses_;
    ++missionPulses_;
    pulseStartedMs_=now;
    stage_=Stage::Pulse;
    return Action::Backward;
  }
  void clearEvent() {
    stage_=Stage::Idle;
    eventPulses_=0;
    requireFirstPulse_=false;
  }
};
#endif
