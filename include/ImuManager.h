#pragma once

#include <Adafruit_MPU6050.h>

struct ImuSnapshot {
  bool initialized = false;
  bool healthy = false;
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
  const char* motionState = "ERROR";
};

class ImuManager {
 public:
  bool begin();
  void update();
  const ImuSnapshot& snapshot() const;

 private:
  Adafruit_MPU6050 mpu_;
  ImuSnapshot snapshot_;
  unsigned long lastSampleMs_ = 0;
  bool anglesInitialized_ = false;
};
