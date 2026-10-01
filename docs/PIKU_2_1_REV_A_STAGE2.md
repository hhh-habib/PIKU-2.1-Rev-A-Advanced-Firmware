# PIKU 2.1 Rev A Stage-2

**Status: Historical Stage-2 development record. Its Manual sonar-fault rules and physical-test checklist describe that earlier stage; see [README.md](../README.md) and [V3.2 software verification](RECOVERY_V32_SOFTWARE_VERIFICATION.md) for current behavior.**

## Objective and status

Stage-2 adds environmental sensing, MPU6050 robot-state telemetry, a local SH1106 status display, and Node V1-compatible nRF24 communication around the physically verified Stage-1 motion/navigation foundation. Firmware identity is `PIKU 2.1 Rev A`, stage `STAGE-2`, version `2.1.0-revA-stage2`.

Implementation and clean-build validation are verified. No firmware was uploaded and no Stage-2 hardware result is claimed.

## Installed hardware and final GPIO map

| Function | GPIO |
| --- | ---: |
| MPU6050 SDA / SCL | 1 / 2 |
| MQ-2 analog through divider | 4 |
| DHT22 data | 5 |
| Flame module AO (12-bit ADC, 11 dB attenuation) | 6 |
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

GPIO6 is an ESP32-S3 ADC input and must never be driven above the board's permitted ADC/GPIO voltage. Before connecting the flame module AO, measure its maximum output across supply and stimulus conditions. Power the module at 3.3 V only if its specification supports that supply, or install and verify a divider/clamp that keeps AO within the ESP32-S3 input limit. Firmware attenuation does not make an over-voltage electrically safe.

`Wire` is initialized once on GPIO1/2 at 400 kHz. MPU6050 initialization probes 0x68 once, then 0x69 once, and remains non-fatal on failure.

`SPI` is initialized once with SCK 12, MISO 13, and MOSI 11 after OLED CS 17 and nRF CSN 15 are driven HIGH. The SH1106 and RF24 drivers share that hardware bus and use independent chip selects.

## Environmental sensing and safety

- DHT22 is sampled every 2 seconds. Failed reads preserve the last numeric value but mark temperature/humidity invalid. Telemetry reports last-success age and cumulative read errors; only a current valid temperature can enter warning or critical logic.
- MQ-2 is sampled every 150 ms and filtered with a deterministic 16-sample moving average. `WARMING`, `LOW_RELATIVE`, `ELEVATED_RELATIVE`, and `HIGH_RELATIVE` describe qualitative ADC bands only. The exact existing thresholds and hysteresis are unchanged; none represents ppm, a calibrated concentration, or proof that air is safe.
- Flame sensing uses GPIO6 AO with explicit 12-bit/11 dB ADC setup, an 8-sample bounded moving average, a learned clear-condition baseline, configurable rising/falling direction, relative delta, persistence, state, validity, timestamp, and age. States are `CLEAR`, `ELEVATED`, `POSSIBLE_FLAME`, and `FLAME_DETECTED`; only the final valid state enters the existing critical stop path.
- Flame direction and three relative thresholds are deliberately marked unconfigured in source until the installed module is physically characterized. Raw/filter/baseline/delta telemetry remains available, but classification is invalid and cannot assert a flame alarm in that condition. Enabling classification requires positive, strictly ordered module-specific deltas.
- HC-SR04 results distinguish `DISTANCE_VALID`, `CLEAR_NO_ECHO`, `SENSOR_FAULT`, and `STALE`. Clean no-echo remains a valid clear/out-of-range observation. In AUTO, fault or stale input forces STOP/hold; in Manual it blocks forward only, preserving STOP, reverse, and turning for supervised escape.
- Navigation LEFT/FRONT/RIGHT decisions use one captured record per scan containing direction, angle, distance, validity, result state, timestamp, and sequence. Cooperative STOP/critical interruption remains active throughout settle, acquisition, reverse, and turn operations.

## MPU6050

Adafruit MPU6050 2.2.6 and Unified Sensor 1.1.15 provide acceleration, gyro, device temperature, health, and sample-age telemetry. Every read checks the library result and all returned scalar values for finiteness. Telemetry reports `HEALTHY`, `FAULT`, `STALE`, or initialization state together with last-success age, cumulative errors, and consecutive failures; invalid reads do not overwrite the last valid vectors. Configuration is +/-4 g, +/-500 degrees/s, and a 21 Hz filter. Roll and pitch are smoothed accelerometer-derived estimates; `STILL`, `MOVING`, and `TILTED` remain qualitative showcase classifications. No yaw, heading, localization, or motor control is claimed.

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

The existing Manual/Auto/STOP/speed/radar interface remains the control priority. Stage-2 panels add DHT validity/age/errors, MQ-2 warm-up/sample ages and relative band, full flame AO diagnostics, ultrasonic state/age/sequence/error counters, IMU health/age/errors plus vectors/angles/motion, radio hardware/peer/counters/ages, and OLED state. `/data` preserves Stage-1 fields and adds nested `imu`, `radio`, `display`, and machine-readable `capabilities` objects. Invalid floating-point values serialize as JSON `null`.

## Build result

Clean PlatformIO build on 2026-09-08: PASS with no compiler warnings. Resolved pinned Stage-2 libraries include DHT 1.4.7, Unified Sensor 1.1.15, BusIO 1.17.4, GFX 1.12.6, SH110X 2.1.14, MPU6050 2.2.6, and RF24 1.6.1.

- RAM: 52,444 / 327,680 bytes (16.0%).
- Flash: 976,609 / 6,553,600 bytes (14.9%).
- Build configuration retains 16 MB flash, `default_16MB.csv`, `qio_opi`, and `BOARD_HAS_PSRAM` for the N16R8 target.
- Embedded dashboard JavaScript syntax: PASS using Deno/V8.

These are build and static verification results, not runtime timing measurements or physical validation.

## Physical verification still required

No physical result is claimed by this change. After a separately authorized manual upload, perform the following supervised checks with the chassis initially raised and an accessible power cutoff:

1. Measure flame AO at GPIO6 under power-up, clear ambient, strongest intended stimulus, and supply extremes; confirm it never exceeds the ESP32-S3 input limit before connecting it.
2. Record flame raw/filtered/baseline/delta values across repeated clear, distant stimulus, intermediate stimulus, and intended detection conditions; determine rising/falling direction and three ordered deltas, then enable configuration and repeat.
3. Verify flame states transition in order with persistence, do not chatter near boundaries, recover to `CLEAR`, and only valid `FLAME_DETECTED` forces the existing critical motor STOP/alarm path.
4. Verify DHT valid flags, approximately 2 s cadence, last-success age, retained last value on a disconnected/failed read, cumulative error increment, and recovery.
5. Verify MQ-2 reports `WARMING` for the configured interval and then only `LOW_RELATIVE`, `ELEVATED_RELATIVE`, or `HIGH_RELATIVE`; compare JSON, browser, OLED, serial, and alarm wording. Calibrate separately if concentration claims are ever required.
6. Verify HC-SR04 valid clear distance and near obstacle, clean no-echo/out-of-range, forced abnormal echo/fault, stale-data behavior, age/sequence, and invalid/fault counters.
7. In AUTO, induce HC-SR04 `SENSOR_FAULT` and `STALE` during front acquisition, forward guard, and each scan direction; confirm immediate STOP/hold and no clear-path selection.
8. In Manual with HC-SR04 fault/stale, confirm forward is rejected/stopped while STOP, reverse, pivot/spin left, and pivot/spin right remain available for supervised escape; confirm a recovered fresh front sample releases the forward hold correctly.
9. Exercise LEFT/FRONT/RIGHT scanning with distinct target distances and compare direction, logical angle, distance, state, timestamp age, and sequence so no reading is attributed to another head position.
10. Disconnect or disturb MPU6050 communication; confirm failed/non-finite reads set `FAULT`, prolonged lack of success sets `STALE`, age and counters advance, last good vectors are not replaced, and healthy status recovers after valid reads. Verify qualitative roll/pitch signs and `STILL`/`MOVING`/`TILTED` separately.
11. Confirm JSON parses continuously, dashboard diagnostic rows update on mobile, OLED state wording matches, and alarm mute affects sound only—not hazard truth or motion blocking.
12. Regression-test safe boot STOP, Manual deadman, AUTO operator STOP during settle/scan/reverse/turn, mode changes, motors/PWM, servo, both IR channels, temperature warning/critical, Wi-Fi/AP/STA, OLED, and nRF24 behavior.

## Next Stage

Stage-3 is the combined CPS dashboard for PIKU 2.0.1, CPS Node V1, and PIKU 2.1 Rev A after physical radio and Stage-2 verification.
