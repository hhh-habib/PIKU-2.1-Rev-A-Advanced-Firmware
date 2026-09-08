# PIKU V2.0.1 Advanced Firmware Testing Record

## Scope

This document records the final reported functional prototype testing for the PIKU V2.0.1 Advanced firmware. It is not a certification report and does not claim calibrated sensor performance.

## Physically tested features

- Manual forward, backward, left, right, pivot/spin, and immediate STOP control
- Auto navigation and autonomous obstacle handling
- Servo centering and autonomous LEFT/FRONT/RIGHT scanning
- Manual LEFT, FRONT, and RIGHT ultrasonic inspection
- Agreement between physical inspection direction, dashboard orientation, and radar direction
- Continuous directional obstacle warning at the configured 7 cm threshold
- Physical buzzer and browser/dashboard warning behavior
- Clearing of directional warnings after obstacle removal or direction change
- Recenter-before-driving behavior with the existing 700 ms servo settling interval
- Fresh FRONT ultrasonic validation before a preserved Manual drive command executes
- Flame, MQ-2 gas, IR, temperature, and ultrasonic warning paths
- Critical-hazard motor blocking and alarm priority
- SoftAP dashboard operation
- Home Wi-Fi scanning, provisioning, connection, and dashboard access
- ST7789 TFT telemetry and warning presentation

No reset was observed during the reported final testing.

During subsequent prototype testing, the installed HC-SR04 successfully reported approximately 341.5 cm. This is an observed result, not calibrated metrology. At greater or poorly reflective distances, an ordinary timeout/no-return can occur and is intentionally non-blocking: nearby positive obstacle evidence remains the ultrasonic trigger for motion response while HC-SR04 and IR monitoring continue. The dashboard radar now plots a 300 cm visual range; valid measurements above 300 cm remain numerically unchanged and are clipped only at the plot boundary.

## Build verification

The corrective publication copy was clean-build verified on 2026-08-19 with:

```sh
pio run --target clean
pio run
```

Result: **PASS**, with no compiler warnings.

- PlatformIO Core: `6.1.19`
- Platform: Espressif 32 `7.0.1`
- Framework package: `framework-arduinoespressif32 3.20017.241212` (Arduino-ESP32 2.0.17 line)
- Environment: `esp32doit-devkit-v1`
- Libraries: ESP32Servo `3.2.1`, DHT sensor library `1.4.7`, Adafruit Unified Sensor `1.1.15`, Adafruit GFX `1.12.6`, Adafruit BusIO `1.17.4`, Adafruit ST7735/ST7789 `1.11.0`, and its resolved seesaw dependency `1.7.9`
- RAM: 50,056 / 327,680 bytes (15.3%)
- Flash: 954,053 / 1,310,720 bytes (72.8%)

Build verification did not upload or flash firmware. The clean command succeeded; PlatformIO then emitted a post-exit telemetry-cache permission traceback outside the project after reporting success. It did not affect cleaning or compilation.

The 2026-08-20 ultrasonic/radar follow-up also passed a clean build with no compiler warnings: RAM 50,056 / 327,680 bytes (15.3%) and flash 954,133 / 1,310,720 bytes (72.8%). No firmware was uploaded.

## Corrective verification boundary

Code-path review and the clean compile cover the Manual deadman, all-mode STOP intake, interruptible Auto sequences, logical Auto radar mapping, active-buzzer output, local secret handling, and dependency pins. These corrective changes were not physically exercised during this repository-only finalization, so the physical-test claims above apply to the previously reported Advanced baseline.

The HC-SR04 clean no-echo/out-of-range behavior and DHT22 last-known-value behavior were deliberately preserved. The available physical documentation depicts an HC-SR04 echo divider and identifies an active buzzer, but does not provide electrical measurements that verify the installed divider ratio or GPIO34 voltage.

## Limitations

- Testing was performed as functional prototype testing rather than formal validation.
- MQ-2 readings remain raw/filtered ADC values and require application-specific calibration.
- IR range depends on physical potentiometer adjustment and mounting.
- Ultrasonic performance depends on target geometry, servo alignment, and the installed electrical interface.
- A clean ultrasonic no-echo timeout is intentionally treated as clear/out-of-range; this is permissive behavior, not proof that the sensor path is healthy.
- DHT22 read failures retain the last known values and do not presently expose freshness validation.
- Dashboard commands use unauthenticated HTTP and are appropriate only for a trusted, supervised network.
- The firmware and robot are not certified life-safety or industrial safety equipment.
