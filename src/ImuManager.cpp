#include "ImuManager.h"

#include <Wire.h>
#include "RobotConfig.h"

bool ImuManager::begin() {
  snapshot_ = ImuSnapshot();
  if (mpu_.begin(0x68, &Wire)) {
    snapshot_.address = 0x68;
  } else if (mpu_.begin(0x69, &Wire)) {
    snapshot_.address = 0x69;
  } else {
    Serial.println("[IMU] ERROR: MPU6050 not detected; robot services will continue");
    return false;
  }

  mpu_.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu_.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu_.setFilterBandwidth(MPU6050_BAND_21_HZ);
  snapshot_.initialized = true;
  snapshot_.healthy = false;
  snapshot_.healthState = "INITIALIZING";
  Serial.println("[IMU] READY: MPU6050 range=+/-4g gyro=+/-500dps filter=21Hz");
  update();
  return true;
}

void ImuManager::update() {
  if (!snapshot_.initialized) return;
  const unsigned long now = millis();
  if (snapshot_.lastSuccessMs != 0 &&
      now - snapshot_.lastSuccessMs > RobotConfig::IMU_STALE_MS) {
    snapshot_.healthy = false;
    snapshot_.healthState = "STALE";
    snapshot_.motionState = "UNAVAILABLE";
  }
  if (lastAttemptMs_ != 0 && now - lastAttemptMs_ < RobotConfig::IMU_INTERVAL_MS) return;
  lastAttemptMs_ = now;
  snapshot_.lastAttemptMs = now;

  sensors_event_t acceleration{};
  sensors_event_t gyro{};
  sensors_event_t temperature{};
  const bool readOk = mpu_.getEvent(&acceleration, &gyro, &temperature);
  const bool finite = readOk &&
    isfinite(acceleration.acceleration.x) && isfinite(acceleration.acceleration.y) &&
    isfinite(acceleration.acceleration.z) && isfinite(gyro.gyro.x) &&
    isfinite(gyro.gyro.y) && isfinite(gyro.gyro.z) &&
    isfinite(temperature.temperature);
  if (!finite) {
    snapshot_.healthy = false;
    snapshot_.errorCount++;
    snapshot_.consecutiveFailures++;
    snapshot_.healthState = snapshot_.lastSuccessMs != 0 &&
        now - snapshot_.lastSuccessMs > RobotConfig::IMU_STALE_MS
      ? "STALE"
      : "FAULT";
    snapshot_.motionState = "UNAVAILABLE";
    if (snapshot_.consecutiveFailures == 1) {
      Serial.println("[IMU] Read failed or returned non-finite data");
    }
    return;
  }

  snapshot_.accelX = acceleration.acceleration.x;
  snapshot_.accelY = acceleration.acceleration.y;
  snapshot_.accelZ = acceleration.acceleration.z;
  snapshot_.gyroX = gyro.gyro.x;
  snapshot_.gyroY = gyro.gyro.y;
  snapshot_.gyroZ = gyro.gyro.z;
  snapshot_.temperature = temperature.temperature;

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
  snapshot_.healthy = true;
  snapshot_.healthState = "HEALTHY";
  snapshot_.sampleMs = now;
  snapshot_.lastSuccessMs = now;
  snapshot_.consecutiveFailures = 0;
}

const ImuSnapshot& ImuManager::snapshot() const { return snapshot_; }
