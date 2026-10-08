#ifndef PINCONFIG_H
#define PINCONFIG_H

namespace PinConfig {
  // Stage-1 L298N direction inputs and shared ENA/ENB PWM output.
  static constexpr int RIGHT_IN1 = 39;
  static constexpr int RIGHT_IN2 = 40;
  static constexpr int LEFT_IN1 = 41;
  static constexpr int LEFT_IN2 = 42;
  static constexpr int MOTOR_ENABLE_PWM = 18;

  static constexpr int SERVO_PIN = 21;
  static constexpr int TRIG_PIN = 9;
  static constexpr int ECHO_PIN = 10;
  static constexpr int LEFT_IR_PIN = 7;
  static constexpr int RIGHT_IR_PIN = 8;

  static constexpr int I2C_SDA = 1;
  static constexpr int I2C_SCL = 2;

  static constexpr int MQ2_ANALOG = 4;
  static constexpr int DHT22_PIN = 5;
  // Digital comparator output (DO) from the 3.3 V flame module; AO is disconnected.
  static constexpr int FLAME_DIGITAL = 6;

  // SH1106 and nRF24 share this single hardware SPI bus.
  static constexpr int SPI_MOSI = 11;
  static constexpr int SPI_SCK = 12;
  static constexpr int SPI_MISO = 13;
  static constexpr int NRF24_CE = 14;
  static constexpr int NRF24_CSN = 15;
  static constexpr int OLED_CS = 17;
  static constexpr int OLED_DC = 47;
  static constexpr int OLED_RESET = 48;

  // Intentionally unallocated in production Stage-2.
  static constexpr int RESERVED_FREE = 16;

  // No Stage-1 buzzer is installed.
  static constexpr int BUZZER_PIN = -1;
}

#endif // PINCONFIG_H
