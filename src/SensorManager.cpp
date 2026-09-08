#include "SensorManager.h"
#include "PinConfig.h"
#include "RobotConfig.h"

SensorManager* SensorManager::activeInstance = nullptr;

static_assert(!RobotConfig::FLAME_THRESHOLDS_CONFIGURED ||
              (RobotConfig::FLAME_ELEVATED_DELTA > 0 &&
               RobotConfig::FLAME_ELEVATED_DELTA < RobotConfig::FLAME_POSSIBLE_DELTA &&
               RobotConfig::FLAME_POSSIBLE_DELTA < RobotConfig::FLAME_DETECTED_DELTA),
              "Configured flame deltas must be positive and strictly ordered");

SensorManager::SensorManager()
  : lastFastSensorMs(0),
    lastDhtMs(0),
    lastMq2Ms(0),
    lastFlameMs(0),
    startedMs(0),
    lastUltrasonicMs(0),
    dht(PinConfig::DHT22_PIN, DHT22),
    mq2Samples{},
    mq2SampleCount(0),
    mq2SampleIndex(0),
    mq2SampleSum(0),
    flameSamples{},
    flameSampleCount(0),
    flameSampleIndex(0),
    flameSampleSum(0),
    flameBaselineSum(0),
    flameBaselineSampleCount(0),
    flameCandidateState(FlameState::Clear),
    flameCandidateSinceMs(0),
    nextDistanceFacingFront(true),
    ultrasonicState(UltrasonicState::Idle),
    activeDistanceFacingFront(true),
    ultrasonicStateStartedUs(0),
    distanceSampleSequenceCounter(0),
    ultrasonicFaultEvidenceCount(0),
    echoHighAtSampleStart(false),
    echoRiseCaptured(false),
    echoFallCaptured(false),
    echoRiseUs(0),
    echoFallUs(0) {}

void SensorManager::begin() {
  pinMode(PinConfig::TRIG_PIN, OUTPUT);
  pinMode(PinConfig::ECHO_PIN, INPUT);
  digitalWrite(PinConfig::TRIG_PIN, LOW);
  pinMode(PinConfig::LEFT_IR_PIN, INPUT);
  pinMode(PinConfig::RIGHT_IR_PIN, INPUT);
  pinMode(PinConfig::FLAME_ANALOG, INPUT);
  pinMode(PinConfig::MQ2_ANALOG, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(PinConfig::MQ2_ANALOG, ADC_11db);
  analogSetPinAttenuation(PinConfig::FLAME_ANALOG, ADC_11db);
  dht.begin();
  startedMs = millis();
  current.mq2Warmup = true;
  activeInstance = this;
  attachInterrupt(digitalPinToInterrupt(PinConfig::ECHO_PIN), SensorManager::handleEchoInterrupt, CHANGE);
  unsigned long startupMs = millis();
  updateFastSensors(startupMs);
  startDistanceSample(micros(), true);
}

void SensorManager::update() {
  unsigned long now = millis();
  unsigned long nowUs = micros();
  advanceDistanceSample(nowUs);
  if (lastFastSensorMs == 0 || now - lastFastSensorMs >= RobotConfig::FAST_SENSOR_INTERVAL_MS) {
    updateFastSensors(now);
  }
  if (lastFlameMs == 0 || now - lastFlameMs >= RobotConfig::FLAME_SAMPLE_INTERVAL_MS) {
    updateFlame(now);
  }
  updateEnvironment(now);
  if (ultrasonicState == UltrasonicState::Idle &&
      (current.distanceSampleMs == 0 || now - current.distanceSampleMs > RobotConfig::ULTRASONIC_STALE_MS)) {
    if (current.ultrasonicState != UltrasonicResultState::Stale) {
      current.ultrasonicInvalidCount++;
    }
    current.distanceValid = false;
    current.ultrasonicState = UltrasonicResultState::Stale;
  }
  if (ultrasonicState == UltrasonicState::Idle &&
      (lastUltrasonicMs == 0 || now - lastUltrasonicMs >= RobotConfig::ULTRASONIC_INTERVAL_MS)) {
    updateUltrasonic(now, nowUs);
  }
}

void SensorManager::setDistanceFacingFront(bool facingFront) {
  nextDistanceFacingFront = facingFront;
}

unsigned long SensorManager::requestDistanceSample(bool facingFront) {
  nextDistanceFacingFront = facingFront;
  startDistanceSample(micros(), facingFront);
  return distanceSampleSequenceCounter + 1;
}

bool SensorManager::isDistanceSampleInProgress() const {
  return ultrasonicState != UltrasonicState::Idle;
}

unsigned long SensorManager::distanceSampleSequence() const {
  return distanceSampleSequenceCounter;
}

UltrasonicSample SensorManager::latestUltrasonicSample() const {
  UltrasonicSample sample;
  sample.distanceCm = current.distanceCm;
  sample.valid = current.distanceValid;
  sample.facingFront = current.distanceFacingFront;
  sample.state = current.ultrasonicState;
  sample.sampleMs = current.distanceSampleMs;
  sample.sequence = current.distanceSampleSequence;
  return sample;
}

float SensorManager::forceDistanceSample() {
  requestDistanceSample(nextDistanceFacingFront);
  return current.distanceCm;
}

const SensorSnapshot& SensorManager::snapshot() const {
  return current;
}

float SensorManager::readDistanceCm() {
  return current.distanceCm;
}

float SensorManager::readTemperature() {
  return current.temperature;
}

float SensorManager::readHumidity() {
  return current.humidity;
}

int SensorManager::readGasRaw() {
  return current.gasRaw;
}

int SensorManager::readGasFiltered() const {
  return current.gasFiltered;
}

bool SensorManager::isMq2WarmingUp() const {
  return current.mq2Warmup;
}

void SensorManager::updateFastSensors(unsigned long now) {
  lastFastSensorMs = now;
  current.leftIrRaw = digitalRead(PinConfig::LEFT_IR_PIN);
  current.rightIrRaw = digitalRead(PinConfig::RIGHT_IR_PIN);
  current.leftObstacleDetected = current.leftIrRaw == RobotConfig::IR_ACTIVE_LEVEL;
  current.rightObstacleDetected = current.rightIrRaw == RobotConfig::IR_ACTIVE_LEVEL;
  current.irObstacleDetected = current.leftObstacleDetected || current.rightObstacleDetected;
  current.updatedMs = now;
}

void SensorManager::updateFlame(unsigned long now) {
  lastFlameMs = now;
  const int sample = analogRead(PinConfig::FLAME_ANALOG);
  current.flameRaw = sample;
  current.flameSampleValid = sample >= 0 && sample <= 4095;
  current.flameSampleMs = now;

  if (!current.flameSampleValid) {
    current.flameClassificationValid = false;
    current.flameDetected = false;
    return;
  }

  if (flameSampleCount < RobotConfig::FLAME_FILTER_SAMPLES) {
    flameSamples[flameSampleIndex] = sample;
    flameSampleSum += sample;
    flameSampleCount++;
  } else {
    flameSampleSum -= flameSamples[flameSampleIndex];
    flameSamples[flameSampleIndex] = sample;
    flameSampleSum += sample;
  }
  flameSampleIndex = (flameSampleIndex + 1) % RobotConfig::FLAME_FILTER_SAMPLES;
  current.flameFiltered = flameSampleCount > 0
    ? static_cast<int>(flameSampleSum / static_cast<long>(flameSampleCount))
    : sample;

  if (!current.flameBaselineValid) {
    flameBaselineSum += sample;
    flameBaselineSampleCount++;
    if (flameBaselineSampleCount >= RobotConfig::FLAME_BASELINE_SAMPLES) {
      current.flameBaseline = static_cast<int>(
        flameBaselineSum / static_cast<long>(flameBaselineSampleCount));
      current.flameBaselineValid = true;
    }
  }

  if (current.flameBaselineValid) {
    current.flameDelta = RobotConfig::FLAME_SIGNAL_DIRECTION ==
        RobotConfig::FlameSignalDirection::DecreasesWithFlame
      ? current.flameBaseline - current.flameFiltered
      : current.flameFiltered - current.flameBaseline;
  } else {
    current.flameDelta = 0;
  }

  current.flameThresholdsConfigured = RobotConfig::FLAME_THRESHOLDS_CONFIGURED;
  current.flameClassificationValid = current.flameBaselineValid &&
    current.flameThresholdsConfigured;
  FlameState candidate = current.flameClassificationValid
    ? classifyFlameDelta(current.flameDelta)
    : FlameState::Clear;

  if (candidate == current.flameState) {
    flameCandidateState = candidate;
    flameCandidateSinceMs = 0;
  } else if (candidate != flameCandidateState) {
    flameCandidateState = candidate;
    flameCandidateSinceMs = now;
  } else {
    const unsigned long persistenceMs = candidate == FlameState::Clear
      ? RobotConfig::FLAME_CLEAR_PERSIST_MS
      : RobotConfig::FLAME_STATE_PERSIST_MS;
    if (flameCandidateSinceMs != 0 && now - flameCandidateSinceMs >= persistenceMs) {
      current.flameState = candidate;
      flameCandidateSinceMs = 0;
    }
  }
  current.flameDetected = current.flameClassificationValid &&
    current.flameState == FlameState::FlameDetected;

  // Track slow clear-condition drift only while the classified state is clear.
  if (current.flameBaselineValid && current.flameState == FlameState::Clear) {
    current.flameBaseline +=
      (current.flameFiltered - current.flameBaseline) /
      static_cast<int>(1U << RobotConfig::FLAME_BASELINE_TRACKING_SHIFT);
  }
  current.updatedMs = now;
}

FlameState SensorManager::classifyFlameDelta(int delta) const {
  if (delta >= RobotConfig::FLAME_DETECTED_DELTA) return FlameState::FlameDetected;
  if (delta >= RobotConfig::FLAME_POSSIBLE_DELTA) return FlameState::PossibleFlame;
  if (delta >= RobotConfig::FLAME_ELEVATED_DELTA) return FlameState::Elevated;
  return FlameState::Clear;
}

void SensorManager::updateEnvironment(unsigned long now) {
  if (lastMq2Ms == 0 || now - lastMq2Ms >= RobotConfig::MQ2_INTERVAL_MS) updateMq2(now);
  if (lastDhtMs == 0 || now - lastDhtMs >= RobotConfig::DHT22_INTERVAL_MS) updateDht(now);
}

void SensorManager::updateDht(unsigned long now) {
  lastDhtMs = now;
  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();
  current.environmentSampleMs = now;
  current.temperatureValid = isfinite(temperature);
  current.humidityValid = isfinite(humidity);
  if (current.temperatureValid) current.temperature = temperature;
  if (current.humidityValid) current.humidity = humidity;
  if (current.temperatureValid && current.humidityValid) {
    current.environmentLastSuccessMs = now;
  } else {
    current.dhtErrorCount++;
  }
}

void SensorManager::updateMq2(unsigned long now) {
  lastMq2Ms = now;
  const int sample = analogRead(PinConfig::MQ2_ANALOG);
  current.gasRaw = sample;
  if (mq2SampleCount < RobotConfig::MQ2_FILTER_SAMPLES) {
    mq2Samples[mq2SampleIndex] = sample;
    mq2SampleSum += sample;
    mq2SampleCount++;
  } else {
    mq2SampleSum -= mq2Samples[mq2SampleIndex];
    mq2Samples[mq2SampleIndex] = sample;
    mq2SampleSum += sample;
  }
  mq2SampleIndex = (mq2SampleIndex + 1) % RobotConfig::MQ2_FILTER_SAMPLES;
  current.gasFiltered = mq2SampleCount > 0
    ? static_cast<int>(mq2SampleSum / static_cast<long>(mq2SampleCount))
    : sample;
  current.mq2Warmup = now - startedMs < RobotConfig::MQ2_WARMUP_MS;
  current.mq2WarmupAgeMs = now - startedMs;
  current.mq2SampleMs = now;
}

void SensorManager::updateUltrasonic(unsigned long nowMs, unsigned long nowUs) {
  lastUltrasonicMs = nowMs;
  startDistanceSample(nowUs, nextDistanceFacingFront);
}

void SensorManager::startDistanceSample(unsigned long nowUs, bool facingFront) {
  activeDistanceFacingFront = facingFront;
  echoHighAtSampleStart = digitalRead(PinConfig::ECHO_PIN) == HIGH;
  echoRiseCaptured = false;
  echoFallCaptured = false;
  echoRiseUs = 0;
  echoFallUs = 0;
  digitalWrite(PinConfig::TRIG_PIN, LOW);
  ultrasonicState = UltrasonicState::TriggerLow;
  ultrasonicStateStartedUs = nowUs;
}

void SensorManager::advanceDistanceSample(unsigned long nowUs) {
  switch (ultrasonicState) {
    case UltrasonicState::Idle:
      return;
    case UltrasonicState::TriggerLow:
      if (nowUs - ultrasonicStateStartedUs >= 2UL) {
        digitalWrite(PinConfig::TRIG_PIN, HIGH);
        ultrasonicState = UltrasonicState::TriggerHigh;
        ultrasonicStateStartedUs = nowUs;
      }
      return;
    case UltrasonicState::TriggerHigh:
      if (nowUs - ultrasonicStateStartedUs >= 10UL) {
        digitalWrite(PinConfig::TRIG_PIN, LOW);
        ultrasonicState = UltrasonicState::WaitingEcho;
        ultrasonicStateStartedUs = nowUs;
      }
      return;
    case UltrasonicState::WaitingEcho:
      if (echoFallCaptured && echoRiseCaptured) {
        unsigned long durationUs = echoFallUs - echoRiseUs;
        float distance = durationUs * 0.0343f / 2.0f;
        bool valid = distance > 0.0f && distance <= RobotConfig::ULTRASONIC_MAX_CM;
        finishDistanceSample(valid ? distance : -1.0f,
                             valid,
                             valid ? UltrasonicResultState::DistanceValid : UltrasonicResultState::ClearNoEcho);
      } else if (nowUs - ultrasonicStateStartedUs >= 30000UL) {
        if (echoHighAtSampleStart) {
          finishAbnormalDistanceSample();
        } else {
          // A rise without a fall inside the measurement window can be a
          // legitimate reflection beyond useful range. Keep it non-blocking;
          // a line that remains HIGH is detected at the next sample start.
          finishDistanceSample(-1.0f, false, UltrasonicResultState::ClearNoEcho);
        }
      }
      return;
  }
}

void SensorManager::finishDistanceSample(float distanceCm, bool valid, UltrasonicResultState resultState) {
  if (resultState != UltrasonicResultState::SensorFault) {
    ultrasonicFaultEvidenceCount = 0;
  }
  current.distanceCm = distanceCm;
  current.distanceFacingFront = activeDistanceFacingFront;
  current.distanceValid = valid;
  current.ultrasonicState = resultState;
  current.distanceSampleMs = millis();
  current.updatedMs = current.distanceSampleMs;
  if (!valid) current.ultrasonicInvalidCount++;
  if (resultState == UltrasonicResultState::SensorFault) current.ultrasonicFaultCount++;
  distanceSampleSequenceCounter++;
  current.distanceSampleSequence = distanceSampleSequenceCounter;
  ultrasonicState = UltrasonicState::Idle;
}

void SensorManager::finishAbnormalDistanceSample() {
  if (ultrasonicFaultEvidenceCount < RobotConfig::ULTRASONIC_SENSOR_FAULT_COUNT) {
    ultrasonicFaultEvidenceCount++;
  }

  UltrasonicResultState resultState =
    ultrasonicFaultEvidenceCount >= RobotConfig::ULTRASONIC_SENSOR_FAULT_COUNT
      ? UltrasonicResultState::SensorFault
      : UltrasonicResultState::ClearNoEcho;

  current.distanceCm = -1.0f;
  current.distanceFacingFront = activeDistanceFacingFront;
  current.distanceValid = false;
  current.ultrasonicState = resultState;
  current.distanceSampleMs = millis();
  current.updatedMs = current.distanceSampleMs;
  current.ultrasonicInvalidCount++;
  if (resultState == UltrasonicResultState::SensorFault) current.ultrasonicFaultCount++;
  distanceSampleSequenceCounter++;
  current.distanceSampleSequence = distanceSampleSequenceCounter;
  ultrasonicState = UltrasonicState::Idle;
}

bool SensorManager::readActiveDigital(int pin, int activeLevel) const {
  return digitalRead(pin) == activeLevel;
}

void IRAM_ATTR SensorManager::handleEchoInterrupt() {
  if (activeInstance != nullptr) {
    activeInstance->onEchoInterrupt();
  }
}

void IRAM_ATTR SensorManager::onEchoInterrupt() {
  if (ultrasonicState != UltrasonicState::WaitingEcho) {
    return;
  }

  unsigned long nowUs = micros();
  if (digitalRead(PinConfig::ECHO_PIN) == HIGH) {
    echoRiseUs = nowUs;
    echoRiseCaptured = true;
    echoFallCaptured = false;
  } else if (echoRiseCaptured) {
    echoFallUs = nowUs;
    echoFallCaptured = true;
  }
}
