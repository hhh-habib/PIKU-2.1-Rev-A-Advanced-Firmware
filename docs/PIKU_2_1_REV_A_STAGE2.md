# PIKU 2.1 Rev A Stage-2

## Objective and status

Stage-2 adds environmental sensing, MPU6050 robot-state telemetry, a local SH1106 status display, and Node V1-compatible nRF24 communication around the physically verified Stage-1 motion/navigation foundation. Firmware identity is `PIKU 2.1 Rev A`, stage `STAGE-2`, version `2.1.0-revA-stage2`.

Implementation and clean-build validation are verified. No firmware was uploaded and no Stage-2 hardware result is claimed.

## Installed hardware and final GPIO map

| Function | GPIO |
| --- | ---: |
| MPU6050 SDA / SCL | 1 / 2 |
| MQ-2 analog through divider | 4 |
| DHT22 data | 5 |
| Flame digital | 6 |
| Left / right front IR | 7 / 8 |
| HC-SR04 trigger / divided echo | 9 / 10 |
| Shared SPI MOSI / SCK / MISO | 11 / 12 / 13 |
| nRF24 CE / CSN | 14 / 15 |
| SH1106 CS / DC / reset | 17 / 47 / 48 |
| Shared L298N ENA+ENB PWM | 18 |
| SG90 signal | 21 |
| L298N IN1 / IN2 / IN3 / IN4 | 39 / 40 / 41 / 42 |

GPIO16 remains unallocated. GPIO17 is OLED CS only. Static review found no collision.

## Power and buses

The protected 3S battery path splits after the master switch. One LM2596 logic rail supplies approximately 5 V to ESP32 VIN, servo, HC-SR04, the nRF24 regulated adapter, and suitable sensor inputs. A separate LM2596 motor rail supplies approximately 6 V to L298N motor Vs and the motors. Grounds are common, the regulated outputs are not joined, and the L298N 5V-EN jumper remains removed. The MPU6050, DHT22, flame sensor, and OLED use 3.3 V as specified; MQ-2 uses 5 V with a physical divider on AO.

`Wire` is initialized once on GPIO1/2 at 400 kHz. MPU6050 initialization probes 0x68 once, then 0x69 once, and remains non-fatal on failure.

`SPI` is initialized once with SCK 12, MISO 13, and MOSI 11 after OLED CS 17 and nRF CSN 15 are driven HIGH. The SH1106 and RF24 drivers share that hardware bus and use independent chip selects.

## Environmental sensing and safety

- DHT22 is sampled every 2 seconds. Failed reads preserve the last numeric value but mark temperature/humidity invalid; only valid temperature can enter warning or critical logic.
- MQ-2 is sampled every 150 ms and filtered with a deterministic 16-sample moving average. The 60-second warmup suppresses gas alarms. Values and thresholds are raw ADC demo values, not calibrated ppm.
- Gas caution is warning-only. Debounced gas high, confirmed active-level flame, or valid critical temperature enters the existing `SafetyManager` critical path and forces the shared motor PWM to zero. Clearing a critical hazard does not replay a stale Manual command.
- Stage-1 HC-SR04 no-echo policy, dual-IR filtering, obstacle fusion, servo calibration, and autonomous recovery remain unchanged.

## MPU6050

Adafruit MPU6050 2.2.6 and Unified Sensor 1.1.15 provide acceleration, gyro, device temperature, health, and sample-age telemetry. Configuration is +/-4 g, +/-500 degrees/s, and a 21 Hz filter. Roll and pitch are smoothed accelerometer-derived estimates; `STILL`, `MOVING`, and `TILTED` are conservative showcase classifications. No yaw, heading, localization, or motor control is claimed.

## SH1106 OLED

Adafruit SH110X 2.1.14 and GFX 1.12.6 drive the 128x64 hardware-SPI display. Four low-rate pages show robot, environment, IMU, and communication state. `SafetyManager` critical state overrides the rotating pages with an alert and reason. Initialization failure logs an error and leaves motion, sensing, Wi-Fi, and radio execution active.

## nRF24 and CPS Node V1 compatibility

The radio implementation mirrors the current live CPS Node V1 source:

- RF24 1.6.1; channel 76; 250 kbps; 16-bit CRC; auto acknowledgement; retries 5/8; PA LOW.
- Five-byte addresses `NV101` (Node receive / PIKU write) and `P21RA` (PIKU receive / Node write).
- Protocol version 1, identical enum values, and identical packed 12-byte `RadioPacket`; compile-time assertions enforce 12 bytes and the 32-byte nRF24 ceiling.
- PIKU sends a heartbeat about every 750 ms. A successful auto-ACK or a valid Node V1 packet marks the peer ONLINE; peer state becomes STALE after 5 seconds. UNSEEN remains distinct from radio ERROR.
- Receive work is bounded to four packets per update. Failed sends increment counters and do not block normal subsystem operation beyond the RF24 library's bounded configured retry transaction.

No successful over-air exchange is claimed until physical testing.

## Dashboard and telemetry

The existing Manual/Auto/STOP/speed/radar interface remains the control priority. Stage-2 panels add validity-aware environment values, IMU vectors/angles/motion, radio hardware/peer/counters/ages, and OLED state. `/data` preserves Stage-1 fields and adds nested `imu`, `radio`, `display`, and machine-readable `capabilities` objects. Invalid floating-point values serialize as JSON `null`.

## Build result

Clean PlatformIO build on 2026-09-07: PASS with no compiler warnings. Resolved pinned Stage-2 libraries include DHT 1.4.7, Unified Sensor 1.1.15, BusIO 1.17.4, GFX 1.12.6, SH110X 2.1.14, MPU6050 2.2.6, and RF24 1.6.1.

- RAM: 52,044 / 327,680 bytes (15.9%).
- Flash: 967,233 / 6,553,600 bytes (14.8%).
- Build configuration retains 16 MB flash, `default_16MB.csv`, `qio_opi`, and `BOARD_HAS_PSRAM` for the N16R8 target.
- Embedded dashboard JavaScript syntax: PASS using Deno/V8.

These are build and static verification results, not runtime timing measurements or physical validation.

## Physical verification still required

After manual upload, verify safe boot STOP, 16 MB flash and 8 MB PSRAM runtime reporting, unchanged Manual/Auto/motor/servo/obstacle behavior, DHT validity and cadence, MQ-2 warmup and physical threshold response, flame polarity/debounce/critical STOP, MPU detection and orientation signs, OLED wiring/pages/alert override, radio hardware detection, acknowledged heartbeat/packet exchange, peer timeout, dashboard rendering on mobile, and independent Stage-2 failure behavior.

## Next Stage

Stage-3 is the combined CPS dashboard for PIKU 2.0.1, CPS Node V1, and PIKU 2.1 Rev A after physical radio and Stage-2 verification.
