#ifndef SONAR_REACQUIRE_POLICY_H
#define SONAR_REACQUIRE_POLICY_H
#include <stdint.h>

// Counts only new front samples after the servo has settled. Time expiry changes
// diagnostic status only; sampling continues until two trusted samples arrive.
class SonarReacquirePolicy {
public:
  void begin(uint32_t now, uint32_t sequence) {
    startedMs_=now;
    lastSequence_=sequence;
    consecutiveHealthy_=0;
    centered_=false;
  }
  void centered(uint32_t sequence) {
    if (centered_) return;
    lastSequence_=sequence;
    centered_=true;
  }
  bool observe(uint32_t sequence, bool healthyFront) {
    if (!centered_ || sequence <= lastSequence_) return false;
    lastSequence_=sequence;
    consecutiveHealthy_=healthyFront
        ? static_cast<uint8_t>(consecutiveHealthy_ + 1) : 0;
    return consecutiveHealthy_ >= 2;
  }
  void invalidate() { consecutiveHealthy_=0; }
  bool windowExpired(uint32_t now, uint32_t windowMs) const {
    return now - startedMs_ >= windowMs;
  }
  uint8_t consecutiveHealthy() const { return consecutiveHealthy_; }
private:
  uint32_t startedMs_=0;
  uint32_t lastSequence_=0;
  uint8_t consecutiveHealthy_=0;
  bool centered_=false;
};
#endif
