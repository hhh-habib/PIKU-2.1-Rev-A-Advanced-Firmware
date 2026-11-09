#include "ImuManager.h"
#include <Wire.h>
#include "RobotConfig.h"

namespace {
constexpr uint16_t kTransactionTimeoutMs = 5;
constexpr unsigned long kMaxRetryMs = 8000;
constexpr uint8_t kGoodFramesRequired = 3;
// Existing +/-4g, +/-500dps, 21Hz; DATA_RDY is polled, no interrupt pin.
constexpr uint8_t kConfig[][2] = {
  {0x19, 0x00}, {0x1a, 0x04}, {0x1b, 0x08}, {0x1c, 0x08},
  {0x6c, 0x00}, {0x38, 0x01}
};
int16_t signedWord(const uint8_t* p) {
  const uint16_t word = (uint16_t(p[0]) << 8) | p[1];
  return word < 0x8000 ? int16_t(word) : int16_t(int32_t(word) - 65536);
}
bool due(unsigned long now, unsigned long at) {
  return int32_t(uint32_t(now - at)) >= 0;
}
}

bool ImuManager::readRegisters(uint8_t reg, uint8_t* bytes, uint8_t count) {
  const uint16_t previousTimeout = Wire.getTimeOut();
  Wire.setTimeOut(kTransactionTimeoutMs);
  Wire.beginTransmission(snapshot_.address);
  Wire.write(reg);
  bool ok = Wire.endTransmission(false) == 0;
  if (ok) {
    const size_t received = Wire.requestFrom(snapshot_.address, size_t(count), true);
    ok = received == count && Wire.available() == count;
    for (uint8_t i = 0; i < count && Wire.available(); ++i) {
      const int value = Wire.read();
      if (value < 0) ok = false;
      bytes[i] = uint8_t(value);
    }
  }
  Wire.setTimeOut(previousTimeout); // Preserve other I2C users' timeout.
  return ok;
}

bool ImuManager::writeRegister(uint8_t reg, uint8_t value) {
  const uint16_t previousTimeout = Wire.getTimeOut();
  Wire.setTimeOut(kTransactionTimeoutMs);
  Wire.beginTransmission(snapshot_.address);
  Wire.write(reg);
  Wire.write(value);
  const bool ok = Wire.endTransmission() == 0;
  Wire.setTimeOut(previousTimeout);
  return ok;
}

bool ImuManager::begin() {
  snapshot_ = ImuSnapshot();
  phase_ = Phase::Probe68;
  nextStepMs_ = millis();
  retryMs_ = 500;
  configStep_ = goodFrames_ = 0;
  attemptedRead_ = anglesInitialized_ = false;
  snapshot_.healthState = "INITIALIZING";
  // Scheduled, not yet detected. No blocking Adafruit begin/reset or delay loop.
  return true;
}

void ImuManager::retryRecovery(unsigned long now) {
  snapshot_.initialized = snapshot_.healthy = false;
  snapshot_.healthState = "RECOVERING";
  snapshot_.motionState = "UNAVAILABLE";
  nextStepMs_ = now + retryMs_;
  retryMs_ = retryMs_ < kMaxRetryMs / 2 ? retryMs_ * 2 : kMaxRetryMs;
  phase_ = Phase::Backoff;
  goodFrames_ = 0;
}

void ImuManager::failedFrame(unsigned long now) {
  snapshot_.healthy = false;
  snapshot_.healthState = "FAULT";
  snapshot_.motionState = "UNAVAILABLE";
  ++snapshot_.errorCount;
  ++snapshot_.consecutiveFailures;
  goodFrames_ = 0;
  if (snapshot_.consecutiveFailures >= 3) retryRecovery(now);
}

void ImuManager::update(bool motorsStopped) {
  const unsigned long now = millis();
  if (phase_ == Phase::Read) {
    if (snapshot_.hasSample && now - snapshot_.lastSuccessMs > RobotConfig::IMU_STALE_MS) {
      snapshot_.healthy = false;
      snapshot_.healthState = "STALE";
      snapshot_.motionState = "UNAVAILABLE";
      goodFrames_ = 0;
    }
    if (attemptedRead_ && now - lastAttemptMs_ < RobotConfig::IMU_INTERVAL_MS) return;
    attemptedRead_ = true;
    lastAttemptMs_ = snapshot_.lastAttemptMs = now;
    uint8_t frame[15]{}; // INT_STATUS plus one coherent 14-byte sensor burst.
    if (!readRegisters(0x3a, frame, sizeof(frame))) { failedFrame(now); return; }
    if (!(frame[0] & 0x01)) {
      // A successful bus transaction alone is NOT a new sensor sample.
      const unsigned long reference = snapshot_.hasSample ? snapshot_.lastSuccessMs : verifiedMs_;
      if (now - reference > RobotConfig::IMU_STALE_MS) failedFrame(now);
      return;
    }
    bool allZero = true, allOnes = true;
    for (uint8_t i = 1; i < sizeof(frame); ++i) {
      allZero = allZero && frame[i] == 0;
      allOnes = allOnes && frame[i] == 0xff;
    }
    if (allZero || allOnes) { failedFrame(now); return; }
    acceptFrame(frame + 1, now);
    return;
  }

  // No motor writes here. Reconfiguration waits for an existing stopped state.
  if (!motorsStopped) { snapshot_.healthState = "WAITING_FOR_IDLE"; return; }
  if (!due(now, nextStepMs_)) return;
  snapshot_.lastAttemptMs = now;
  uint8_t bytes[4]{};
  bool ok = true;
  switch (phase_) {
    case Phase::Backoff: phase_ = Phase::Probe68; break;
    case Phase::Probe68:
    case Phase::Probe69:
      snapshot_.address = phase_ == Phase::Probe68 ? 0x68 : 0x69;
      if (readRegisters(0x75, bytes, 1) && bytes[0] == 0x68) {
        snapshot_.healthState = "RECOVERING";
        phase_ = Phase::Reset;
      } else if (phase_ == Phase::Probe68) phase_ = Phase::Probe69;
      else ok = false;
      break;
    case Phase::Reset:
      ok = writeRegister(0x6b, 0x80);
      resetStartedMs_ = now;
      nextStepMs_ = now + 100;
      anglesInitialized_ = attemptedRead_ = false;
      phase_ = Phase::ResetWait;
      break;
    case Phase::ResetWait:
      ok = readRegisters(0x6b, bytes, 1);
      if (ok && !(bytes[0] & 0x80)) phase_ = Phase::SignalReset;
      else if (now - resetStartedMs_ >= 300) ok = false;
      else nextStepMs_ = now + 10;
      break;
    case Phase::SignalReset:
      ok = writeRegister(0x68, 0x07);
      nextStepMs_ = now + 100;
      phase_ = Phase::Wake;
      break;
    case Phase::Wake:
      ok = writeRegister(0x6b, 0x01);
      nextStepMs_ = now + 100;
      configStep_ = 0;
      phase_ = Phase::Configure;
      break;
    case Phase::Configure:
      ok = writeRegister(kConfig[configStep_][0], kConfig[configStep_][1]);
      if (++configStep_ == sizeof(kConfig) / sizeof(kConfig[0])) phase_ = Phase::VerifyPower;
      break;
    case Phase::VerifyPower:
      ok = readRegisters(0x6b, bytes, 2) && bytes[0] == 0x01 && bytes[1] == 0;
      phase_ = Phase::VerifyConfig;
      break;
    case Phase::VerifyConfig:
      ok = readRegisters(0x19, bytes, 4) && bytes[0] == 0 && bytes[1] == 4 &&
           bytes[2] == 8 && bytes[3] == 8;
      if (ok) {
        snapshot_.initialized = true;
        snapshot_.healthState = "VERIFYING";
        snapshot_.consecutiveFailures = 0;
        verifiedMs_ = now;
        goodFrames_ = 0;
        phase_ = Phase::Read;
      }
      break;
    case Phase::Read: break;
  }
  if (!ok) { ++snapshot_.errorCount; ++snapshot_.consecutiveFailures; retryRecovery(now); }
}

void ImuManager::acceptFrame(const uint8_t* bytes, unsigned long now) {
  constexpr float accelScale = 9.80665f / 8192.0f;
  constexpr float gyroScale = DEG_TO_RAD / 65.5f;
  snapshot_.accelX = signedWord(bytes) * accelScale;
  snapshot_.accelY = signedWord(bytes + 2) * accelScale;
  snapshot_.accelZ = signedWord(bytes + 4) * accelScale;
  snapshot_.temperature = signedWord(bytes + 6) / 340.0f + 36.53f;
  snapshot_.gyroX = signedWord(bytes + 8) * gyroScale;
  snapshot_.gyroY = signedWord(bytes + 10) * gyroScale;
  snapshot_.gyroZ = signedWord(bytes + 12) * gyroScale;

  const float rollAccel = atan2f(snapshot_.accelY, snapshot_.accelZ) * RAD_TO_DEG;
  const float pitchAccel = atan2f(-snapshot_.accelX,
                                  sqrtf(snapshot_.accelY * snapshot_.accelY +
                                        snapshot_.accelZ * snapshot_.accelZ)) * RAD_TO_DEG;
  if (!anglesInitialized_) {
    snapshot_.roll = rollAccel;
    snapshot_.pitch = pitchAccel;
    anglesInitialized_ = true;
  } else {
    snapshot_.roll = 0.8f * snapshot_.roll + 0.2f * rollAccel;
    snapshot_.pitch = 0.8f * snapshot_.pitch + 0.2f * pitchAccel;
  }

  const float accelMagnitude = sqrtf(snapshot_.accelX * snapshot_.accelX +
                                     snapshot_.accelY * snapshot_.accelY +
                                     snapshot_.accelZ * snapshot_.accelZ);
  const float gyroMagnitude = sqrtf(snapshot_.gyroX * snapshot_.gyroX +
                                    snapshot_.gyroY * snapshot_.gyroY +
                                    snapshot_.gyroZ * snapshot_.gyroZ);
  if (fabsf(snapshot_.roll) >= 20.0f || fabsf(snapshot_.pitch) >= 20.0f) {
    snapshot_.motionState = "TILTED";
  } else if (fabsf(accelMagnitude - 9.80665f) >= 1.5f || gyroMagnitude >= 0.35f) {
    snapshot_.motionState = "MOVING";
  } else {
    snapshot_.motionState = "STILL";
  }
  if (goodFrames_ < kGoodFramesRequired) ++goodFrames_;
  snapshot_.healthy = goodFrames_ >= kGoodFramesRequired;
  snapshot_.healthState = snapshot_.healthy ? "HEALTHY" : "VERIFYING";
  if (!snapshot_.healthy) snapshot_.motionState = "UNAVAILABLE";
  else retryMs_ = 500;
  snapshot_.hasSample = true;
  snapshot_.sampleMs = now;
  snapshot_.lastSuccessMs = now;
  snapshot_.consecutiveFailures = 0;
}

const ImuSnapshot& ImuManager::snapshot() const { return snapshot_; }
