#pragma once

#include <Adafruit_MPU6050.h>

struct ImuSnapshot {
  bool initialized = false;
  bool healthy = false;
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
  void update();
  const ImuSnapshot& snapshot() const;

 private:
  Adafruit_MPU6050 mpu_;
  ImuSnapshot snapshot_;
  unsigned long lastAttemptMs_ = 0;
  bool anglesInitialized_ = false;
};
