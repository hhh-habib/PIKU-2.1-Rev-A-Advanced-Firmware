#pragma once
#include <stdint.h>

inline bool staRetryDue(uint32_t now, uint32_t lastAttempt, uint32_t interval) {
  return lastAttempt == 0 || now - lastAttempt >= interval;
}

inline uint32_t staNextRetryInterval(uint32_t current, uint32_t maximum) {
  return current >= maximum / 2 ? maximum : current * 2;
}
