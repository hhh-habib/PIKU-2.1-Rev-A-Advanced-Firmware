#pragma once

#include <Arduino.h>

struct ImuSnapshot {
  bool initialized = false;
  bool healthy = false;
  bool hasSample = false; // Checked frame exists, including millis()==0.
  const char* healthState = "NOT_INITIALIZED";
  uint8_t address = 0;
  float accelX = NAN;
  float accelY = NAN;
  float accelZ = NAN;
  float gyroX = NAN;
  float gyroY = NAN;
  float gyroZ = NAN;
  float roll = NAN;
  float pitch = NAN;
  float temperature = NAN;
  unsigned long sampleMs = 0;
  unsigned long lastAttemptMs = 0;
  unsigned long lastSuccessMs = 0;
  unsigned long errorCount = 0;
  unsigned long consecutiveFailures = 0;
  const char* motionState = "UNAVAILABLE";
};

class ImuManager {
 public:
  bool begin();
  void update(bool motorsStopped = true);
  const ImuSnapshot& snapshot() const;

 private:
  enum class Phase : uint8_t {
    Probe68, Probe69, Reset, ResetWait, SignalReset, Wake, Configure,
    VerifyPower, VerifyConfig, Read, Backoff
  };
  Phase phase_ = Phase::Probe68;
  ImuSnapshot snapshot_;
  unsigned long lastAttemptMs_ = 0;
  bool anglesInitialized_ = false;
  unsigned long nextStepMs_ = 0, resetStartedMs_ = 0, verifiedMs_ = 0;
  unsigned long retryMs_ = 500;
  uint8_t configStep_ = 0, goodFrames_ = 0;
  bool attemptedRead_ = false;
  bool readRegisters(uint8_t reg, uint8_t* bytes, uint8_t count);
  bool writeRegister(uint8_t reg, uint8_t value);
  void retryRecovery(unsigned long now);
  void failedFrame(unsigned long now);
  void acceptFrame(const uint8_t* bytes, unsigned long now);
};
