#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include "RobotConfig.h"

enum class UltrasonicResultState {
  DistanceValid,
  ClearNoEcho,
  SensorFault
};

struct SensorSnapshot {
  float distanceCm = -1.0f;
  bool distanceValid = false;
  bool distanceFacingFront = true;
  UltrasonicResultState ultrasonicState = UltrasonicResultState::ClearNoEcho;
  unsigned long distanceSampleSequence = 0;
  float temperature = NAN;
  bool temperatureValid = false;
  float humidity = NAN;
  bool humidityValid = false;
  unsigned long environmentSampleMs = 0;
  int gasRaw = 0;
  int gasFiltered = 0;
  bool mq2Warmup = false;
  unsigned long mq2SampleMs = 0;
  int leftIrRaw = HIGH;
  int rightIrRaw = HIGH;
  bool leftObstacleDetected = false;
  bool rightObstacleDetected = false;
  bool irObstacleDetected = false;
  int flameRaw = HIGH;
  bool flameDetected = false;
  unsigned long updatedMs = 0;
};

class SensorManager {
public:
  SensorManager();

  void begin();
  void update();
  void setDistanceFacingFront(bool facingFront);
  unsigned long requestDistanceSample(bool facingFront);
  bool isDistanceSampleInProgress() const;
  unsigned long distanceSampleSequence() const;
  float forceDistanceSample();
  const SensorSnapshot& snapshot() const;
  float readDistanceCm();
  float readTemperature();
  float readHumidity();
  int readGasRaw();
  int readGasFiltered() const;
  bool isMq2WarmingUp() const;

private:
  SensorSnapshot current;
  unsigned long lastFastSensorMs;
  unsigned long lastDhtMs;
  unsigned long lastMq2Ms;
  unsigned long startedMs;
  unsigned long lastUltrasonicMs;
  DHT dht;
  int mq2Samples[RobotConfig::MQ2_FILTER_SAMPLES];
  size_t mq2SampleCount;
  size_t mq2SampleIndex;
  long mq2SampleSum;
  bool nextDistanceFacingFront;
  enum class UltrasonicState {
    Idle,
    TriggerLow,
    TriggerHigh,
    WaitingEcho
  };
  UltrasonicState ultrasonicState;
  bool activeDistanceFacingFront;
  unsigned long ultrasonicStateStartedUs;
  unsigned long distanceSampleSequenceCounter;
  int ultrasonicFaultEvidenceCount;
  bool echoHighAtSampleStart;
  volatile bool echoRiseCaptured;
  volatile bool echoFallCaptured;
  volatile unsigned long echoRiseUs;
  volatile unsigned long echoFallUs;
  static SensorManager* activeInstance;

  void updateFastSensors(unsigned long now);
  void updateEnvironment(unsigned long now);
  void updateDht(unsigned long now);
  void updateMq2(unsigned long now);
  void updateUltrasonic(unsigned long nowMs, unsigned long nowUs);
  void startDistanceSample(unsigned long nowUs, bool facingFront);
  void advanceDistanceSample(unsigned long nowUs);
  void finishDistanceSample(float distanceCm, bool valid, UltrasonicResultState resultState);
  void finishAbnormalDistanceSample();
  bool readActiveDigital(int pin, int activeLevel) const;
  static void IRAM_ATTR handleEchoInterrupt();
  void IRAM_ATTR onEchoInterrupt();
};

#endif // SENSORMANAGER_H
