#pragma once
#include <stdint.h>

// A short-lived, one-use admission ticket issued only after an applied STOP.
// This is freshness/ordering protection, not authentication or a physical-stop sensor.
class ModeTransitionFence {
public:
  static constexpr uint32_t WINDOW_MS = 2000;
  void reset(uint32_t boot) { boot_ = boot; revision_ = 0; active_ = false; }
  void revoke() { advance(); active_ = false; }
  void issue(uint32_t now) { advance(); issuedMs_ = now; active_ = true; }
  uint32_t boot() const { return boot_; }
  uint32_t revision() const { return revision_; }
  bool consume(const char* text, uint32_t now) {
    uint32_t boot = 0, revision = 0;
    const char* cursor = text;
    if (!number(cursor, boot) || *cursor++ != '.' || !number(cursor, revision) ||
        *cursor != '\0' || !active_ || boot != boot_ || revision != revision_ ||
        static_cast<uint32_t>(now - issuedMs_) >= WINDOW_MS) return false;
    active_ = false;
    return true;
  }
private:
  uint32_t boot_ = 0, revision_ = 0, issuedMs_ = 0;
  bool active_ = false;
  void advance() { if (++revision_ == 0) ++revision_; }
  static bool number(const char*& p, uint32_t& value) {
    if (!p || *p < '0' || *p > '9') return false;
    uint8_t digits = 0;
    do {
      if (++digits > 10) return false;
      const uint32_t digit = static_cast<uint32_t>(*p++ - '0');
      if (value > (UINT32_MAX - digit) / 10U) return false;
      value = value * 10U + digit;
    } while (*p >= '0' && *p <= '9');
    return true;
  }
};
