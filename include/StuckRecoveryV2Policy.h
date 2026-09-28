#ifndef STUCK_RECOVERY_V2_POLICY_H
#define STUCK_RECOVERY_V2_POLICY_H
#include <stdint.h>

// Decision history only. It makes no claim of measured position or progress.
enum class StuckV2State {
  Normal, Suspected, EscapeScan, EscapeTurn, EscapeVerify, TrappedHold
};
class StuckRecoveryV2Policy {
public:
  static constexpr uint32_t WINDOW_MS = 15000;
  static constexpr uint32_t ESCAPE_LIMIT_MS = 30000;
  static constexpr uint32_t SUSTAINED_CLEAR_MS = 5000;
  static constexpr uint8_t MAX_ATTEMPTS = 3;
  StuckV2State state() const { return state_; }
  uint8_t attempts() const { return attempts_; }
  uint8_t cycles() const { return cycles_; }
  uint8_t reversals() const { return reversals_; }
  bool hasCommittedSide() const { return committed_; }
  bool committedLeft() const { return committedLeft_; }
  bool previousFailedLeft() const { return hasLastTurn_ && lastTurnLeft_; }
  bool alternateTried() const { return alternateTried_; }
  uint8_t sideTurns() const { return sideTurns_; }
  uint8_t totalTurns() const { return totalTurns_; }
  void commitSide(bool left) {
    if (state_ != StuckV2State::EscapeScan) return;
    if (committed_ && committedLeft_ != left) alternateTried_=true;
    if (!committed_ || committedLeft_ != left) sideTurns_=0;
    committed_=true; committedLeft_=left; state_=StuckV2State::EscapeTurn;
  }
  bool startTurn(uint8_t maxSide, uint8_t maxTotal) {
    if (state_ != StuckV2State::EscapeTurn || !committed_ ||
        sideTurns_ >= maxSide || totalTurns_ >= maxTotal) return false;
    ++sideTurns_; ++totalTurns_; return true;
  }
  void continueTurn() {
    if (state_ == StuckV2State::EscapeVerify) state_=StuckV2State::EscapeTurn;
  }
  void prepareAlternateScan() {
    if (state_ == StuckV2State::EscapeVerify && !alternateTried_)
      state_=StuckV2State::EscapeScan;
  }

  void rearm() { state_=StuckV2State::Normal; attempts_=0; clearHistory(); resetEscape(); cooldownStartedMs_=0; }
  void abort() { state_=StuckV2State::Normal; clearHistory(); resetEscape(); cooldownStartedMs_=0; }
  void noteSustainedClear(uint32_t ms) {
    if (state_ == StuckV2State::Normal && ms >= SUSTAINED_CLEAR_MS) {
      clearHistory();
      attempts_=0;
    }
  }
  void noteAvoidance(uint32_t now, uint32_t precedingForwardMs, bool validFront) {
    if (state_ != StuckV2State::Normal || !validFront) return;
    invalidScans_=0;
    if (cycles_ == 0 || now-windowStartedMs_ > WINDOW_MS) {
      clearHistory();
      windowStartedMs_=now;
    }
    if (cycles_ < 255) ++cycles_;
    if (precedingForwardMs > 0 && precedingForwardMs < 1000 &&
        shortFailedSteps_ < 255) ++shortFailedSteps_;
    evaluate();
  }
  bool noteInvalidScan(uint32_t now) {
    if (state_ != StuckV2State::Normal) return false;
    if (invalidScans_ == 0 || now-invalidWindowStartedMs_ > WINDOW_MS) {
      invalidScans_=0;
      invalidWindowStartedMs_=now;
    }
    // Invalid evidence breaks the contiguous valid-cycle history.
    cycles_=0; reversals_=0; shortFailedSteps_=0; blockedScans_=0;
    hasLastTurn_=false;
    if (invalidScans_ < 255) ++invalidScans_;
    return invalidScans_ >= 3;
  }
  void noteBlockedScan() {
    if (state_ != StuckV2State::Normal) return;
    if (blockedScans_ < 255) ++blockedScans_;
    evaluate();
  }
  void noteTurn(bool left) {
    if (state_ != StuckV2State::Normal) return;
    if (hasLastTurn_ && lastTurnLeft_ != left && reversals_ < 255)
      ++reversals_;
    lastTurnLeft_=left;
    hasLastTurn_=true;
    evaluate();
  }
  bool startEscape(uint32_t now) {
    if (state_ != StuckV2State::Suspected) return false;
    if (attempts_ >= MAX_ATTEMPTS) { hold(now); return false; }
    ++attempts_;
    escapeStartedMs_=now;
    resetEscape();
    state_=StuckV2State::EscapeScan;
    return true;
  }
  bool escapeTimedOut(uint32_t now, uint32_t limitMs=ESCAPE_LIMIT_MS) const {
    return state_ != StuckV2State::Normal && state_ != StuckV2State::TrappedHold &&
           now-escapeStartedMs_ >= limitMs;
  }
  void turnVerified() { if(state_==StuckV2State::EscapeTurn) state_=StuckV2State::EscapeVerify; }
  void recovered() { state_=StuckV2State::Normal; attempts_=0; clearHistory(); resetEscape(); cooldownStartedMs_=0; }
  void hold(uint32_t now=0) { state_=StuckV2State::TrappedHold; cooldownStartedMs_=now; }
  bool retryReady(uint32_t now, uint32_t cooldownMs, bool prerequisitesReady) const {
    return state_==StuckV2State::TrappedHold && prerequisitesReady &&
           now-cooldownStartedMs_>=cooldownMs;
  }
  bool beginRetryRound(uint32_t now) {
    if (state_!=StuckV2State::TrappedHold) return false;
    clearHistory(); resetEscape(); attempts_=1;
    escapeStartedMs_=now; cooldownStartedMs_=0;
    state_=StuckV2State::EscapeScan;
    return true;
  }
  const char* name() const {
    switch(state_) {
      case StuckV2State::Normal: return "NORMAL";
      case StuckV2State::Suspected: return "SUSPECTED";
      case StuckV2State::EscapeScan: return "ESCAPE_SCAN";
      case StuckV2State::EscapeTurn: return "ESCAPE_TURN";
      case StuckV2State::EscapeVerify: return "ESCAPE_VERIFY";
      case StuckV2State::TrappedHold: return "TRAPPED_HOLD";
    }
    return "UNKNOWN";
  }
private:
  StuckV2State state_=StuckV2State::Normal;
  uint32_t windowStartedMs_=0, escapeStartedMs_=0, cooldownStartedMs_=0;
  uint32_t invalidWindowStartedMs_=0;
  uint8_t cycles_=0, reversals_=0, shortFailedSteps_=0;
  uint8_t blockedScans_=0, invalidScans_=0, attempts_=0;
  bool hasLastTurn_=false, lastTurnLeft_=false;
  bool committed_=false, committedLeft_=false, alternateTried_=false;
  uint8_t sideTurns_=0, totalTurns_=0;
  void resetEscape() {
    committed_=false; committedLeft_=false; alternateTried_=false;
    sideTurns_=0; totalTurns_=0;
  }
  void clearHistory() {
    windowStartedMs_=0; cycles_=0; reversals_=0; shortFailedSteps_=0;
    blockedScans_=0; invalidScans_=0;
    hasLastTurn_=false; lastTurnLeft_=false;
  }
  void evaluate() {
    if (cycles_ >= 3 &&
        (reversals_ >= 2 || shortFailedSteps_ >= 2 || blockedScans_ >= 3))
      state_=StuckV2State::Suspected;
  }
};
#endif
