#ifndef ROBOTCONFIG_H
#define ROBOTCONFIG_H

#include <Arduino.h>

namespace RobotConfig {
  enum class FlameSignalDirection : uint8_t {
    DecreasesWithFlame,
    IncreasesWithFlame
  };

  static constexpr const char* NODE_ID = "PIKU_2_1_REV_A";
  static constexpr const char* PRODUCT_NAME = "PIKU 2.1 Rev A";
  static constexpr const char* FIRMWARE_VERSION = "2.1.0-revA-stage2";
  static constexpr const char* FIRMWARE_STAGE = "STAGE-2";

  // Centralized Stage-1 capability map. Disabled inputs are never initialized.
  static constexpr bool HAS_DISPLAY = true;
  static constexpr bool HAS_BUZZER = false;
  static constexpr bool HAS_DHT22 = true;
  static constexpr bool HAS_MQ2 = true;
  static constexpr bool HAS_FLAME_SENSOR = true;
  static constexpr bool HAS_DUAL_FRONT_IR = true;

  static constexpr float OBSTACLE_CM = 25;
  static constexpr float AUTO_OBSTACLE_CM = 20.0f;
  static constexpr float MANUAL_FORWARD_STOP_CM = 25.0f;
  static constexpr float MANUAL_FORWARD_CLEAR_CM = 30.0f;
  static constexpr float NEAR_OBSTACLE_CM = 7.0f;
  static constexpr float ULTRASONIC_MAX_CM = 350;
  // Retained Stage-2 thresholds require calibration after that hardware is installed.
  static constexpr int GAS_ELEVATED_RELATIVE_THRESHOLD = 1200;
  static constexpr int GAS_HIGH_RELATIVE_THRESHOLD = 1800;
  static constexpr int GAS_ELEVATED_RELATIVE_CLEAR_THRESHOLD = 1050;
  static constexpr int GAS_HIGH_RELATIVE_CLEAR_THRESHOLD = 1600;

  static constexpr float TEMP_WARNING_C = 45.0f;
  static constexpr float TEMP_CRITICAL_C = 55.0f;
  static constexpr float TEMP_CRITICAL_CLEAR_C = 50.0f;

  // Change only this constant if both installed IR modules use active-HIGH output.
  static constexpr int IR_ACTIVE_LEVEL = LOW;

  // GPIO6 is the flame module AO connection. These are relative ADC-count
  // deltas from a learned clear-condition baseline, not universal flame values.
  // Characterize the installed sensor and divider, then set the direction,
  // three ordered thresholds, and FLAME_THRESHOLDS_CONFIGURED=true together.
  static constexpr FlameSignalDirection FLAME_SIGNAL_DIRECTION =
    FlameSignalDirection::DecreasesWithFlame;
  static constexpr bool FLAME_THRESHOLDS_CONFIGURED = false;
  static constexpr int FLAME_ELEVATED_DELTA = 0;
  static constexpr int FLAME_POSSIBLE_DELTA = 0;
  static constexpr int FLAME_DETECTED_DELTA = 0;
  static constexpr size_t FLAME_FILTER_SAMPLES = 8;
  static constexpr size_t FLAME_BASELINE_SAMPLES = 32;
  static constexpr uint8_t FLAME_BASELINE_TRACKING_SHIFT = 6;

  static constexpr int CENTER_SCAN = 90;
  static constexpr int LEFT_SCAN = 160;
  static constexpr int RIGHT_SCAN = 20;

  static constexpr int DEFAULT_MOTOR_SPEED_PERCENT = 60;
  static constexpr int MOTOR_DRIVE_MIN_PWM_PERCENT = 65;
  static constexpr int MOTOR_TURN_MIN_PWM_PERCENT = 75;
  static constexpr int MOTOR_PWM_FREQUENCY_HZ = 20000;
  static constexpr int MOTOR_PWM_RESOLUTION_BITS = 8;
  // GPIO18 drives the physically tied L298N ENA/ENB inputs.
  static constexpr int MOTOR_PWM_CHANNEL = 4;

  static constexpr unsigned long MANUAL_SENSOR_INTERVAL_MS = 500;
  static constexpr unsigned long SERIAL_LOG_INTERVAL_MS = 1000;
  static constexpr unsigned long FAST_SENSOR_INTERVAL_MS = 50;
  static constexpr unsigned long FLAME_SAMPLE_INTERVAL_MS = 50;
  static constexpr unsigned long MQ2_INTERVAL_MS = 150;
  static constexpr unsigned long DHT22_INTERVAL_MS = 2000;
  static constexpr unsigned long MQ2_WARMUP_MS = 60000;
  static constexpr size_t MQ2_FILTER_SAMPLES = 16;
  static constexpr unsigned long IMU_INTERVAL_MS = 25;
  static constexpr unsigned long IMU_STALE_MS = 250;
  static constexpr unsigned long OLED_REFRESH_INTERVAL_MS = 250;
  static constexpr unsigned long OLED_PAGE_INTERVAL_MS = 2500;
  static constexpr unsigned long NRF_HEARTBEAT_INTERVAL_MS = 750;
  static constexpr unsigned long NRF_PEER_STALE_MS = 5000;
  static constexpr unsigned long ULTRASONIC_INTERVAL_MS = 100;
  static constexpr unsigned long ULTRASONIC_GUARD_INTERVAL_MS = 100;
  static constexpr unsigned long ULTRASONIC_SAMPLE_TIMEOUT_MS = 40;
  static constexpr unsigned long ULTRASONIC_STALE_MS = 500;
  static constexpr unsigned long NAVIGATION_SCAN_SET_MAX_AGE_MS = 5000;
  static constexpr unsigned long AUTO_FORWARD_STEP_MS = 300;
  static constexpr unsigned long AUTO_STOP_SETTLE_MS = 100;
  static constexpr unsigned long DASHBOARD_PUBLISH_INTERVAL_MS = 250;
  // Manual drive commands are refreshed by the dashboard every 250 ms.
  // Stop motion if three refresh periods are missed.
  static constexpr unsigned long MANUAL_COMMAND_TIMEOUT_MS = 750;
  static constexpr unsigned long WIFI_STATUS_INTERVAL_MS = 1000;
  static constexpr unsigned long WIFI_STA_RETRY_MS = 15000;
  static constexpr unsigned long IR_RECOVERY_SETTLE_MS = 1000;
  static constexpr unsigned long IR_RECOVERY_REVERSE_MS = 220;
  static constexpr unsigned long AUTO_OBSTACLE_REVERSE_MS = 500;
  static constexpr unsigned long IR_RECOVERY_COOLDOWN_MS = 1500;
  static constexpr unsigned long OBSTACLE_WARNING_MS = 1800;
  static constexpr int ULTRASONIC_WARNING_INVALID_COUNT = 3;
  static constexpr int ULTRASONIC_SENSOR_FAULT_COUNT = 3;
  static constexpr unsigned long GAS_DEBOUNCE_MS = 600;
  static constexpr unsigned long GAS_CLEAR_DEBOUNCE_MS = 2000;
  static constexpr unsigned long FLAME_DEBOUNCE_MS = 80;
  static constexpr unsigned long FLAME_STATE_PERSIST_MS = 150;
  static constexpr unsigned long FLAME_CLEAR_PERSIST_MS = 300;
  static constexpr unsigned long IR_DEBOUNCE_MS = 80;
  static constexpr unsigned long TEMP_DEBOUNCE_MS = 1000;
  static constexpr unsigned long DIGITAL_CLEAR_DEBOUNCE_MS = 200;

  static constexpr uint8_t NRF_CHANNEL = 76;
  static constexpr uint32_t NRF_SPI_SPEED_HZ = 4000000;
  static constexpr uint8_t NRF_RETRY_DELAY = 5;
  static constexpr uint8_t NRF_RETRY_COUNT = 8;
  static constexpr uint8_t NRF_PROTOCOL_VERSION = 1;
  static constexpr uint8_t NRF_NODE_ADDRESS[5] = {'N', 'V', '1', '0', '1'};
  static constexpr uint8_t NRF_PIKU_ADDRESS[5] = {'P', '2', '1', 'R', 'A'};
}

#endif // ROBOTCONFIG_H
