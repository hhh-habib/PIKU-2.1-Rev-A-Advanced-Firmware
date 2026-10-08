#ifndef MANUAL_OVERRIDE_POLICY_H
#define MANUAL_OVERRIDE_POLICY_H

#include <stdint.h>

// Volatile permission only. Movement commands and their watchdog are separate.
class ManualOverridePolicy {
public:
  static constexpr uint32_t CLEAR_MS = 2000;
  bool enabled() const { return enabled_; }
  bool enable(bool manual, bool flameGas, bool otherCritical,
              bool liveSession, bool confirmed) {
    if (!manual || !flameGas || otherCritical || !liveSession || !confirmed)
      return false;
    enabled_ = true;
    clearSinceMs_ = 0;
    return true;
  }
  void revoke() { enabled_ = false; clearSinceMs_ = 0; }
  void movementStopped() {} // release/dead-man stop motors, not permission
  // Ordinary direction release and movement dead-man deliberately call neither
  // revoke() nor tick() with a false link state.
  void tick(uint32_t now, bool manual, bool flameGas,
            bool otherCritical, bool liveSession) {
    if (!enabled_) return;
    (void)liveSession; // saved permission survives loss; movement still requires a live session
    if (!manual || otherCritical) { revoke(); return; }
    if (flameGas) { clearSinceMs_ = 0; return; }
    if (clearSinceMs_ == 0) clearSinceMs_ = now ? now : 1;
    if (now - clearSinceMs_ >= CLEAR_MS) revoke();
  }
  bool exemptsFlameGas(bool manual, bool liveSession) const {
    return enabled_ && manual && liveSession;
  }
private:
  bool enabled_ = false;
  uint32_t clearSinceMs_ = 0;
};
#endif
