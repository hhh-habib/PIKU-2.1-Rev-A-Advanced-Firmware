# PIKU 2.1 Rev A Stage-1 Engineering Note

## Scope

This Stage-1 firmware migrates the proven PIKU 2.0.1 control architecture to the ESP32-S3 DevKitC-1 N16R8 and includes fixed-enable 4WD motion, Manual and Auto control, SG90 directional scanning, HC-SR04 ranging, dual front IR sensing, and the existing HTTP Wi-Fi dashboard.

Firmware identity: `2.1.0-revA-stage1`.

## ESP32-S3 migration

- PlatformIO environment: `esp32-s3-n16r8`
- Board: `esp32-s3-devkitc-1`
- Flash: 16 MB with `default_16MB.csv`
- PSRAM/memory type: `qio_opi` with `BOARD_HAS_PSRAM`
- Framework: Arduino for ESP32
- No upload or monitor port is hardcoded.

## Active Stage-1 hardware

| Function | GPIO |
| --- | ---: |
| L298N IN1 / right IN1 | 39 |
| L298N IN2 / right IN2 | 40 |
| L298N IN3 / left IN1 | 41 |
| L298N IN4 / left IN2 | 42 |
| SG90 signal | 21 |
| HC-SR04 trigger | 9 |
| HC-SR04 echo through installed divider | 10 |
| Left front IR output | 7 |
| Right front IR output | 8 |

ENA and ENB remain jumper-enabled. Stage-1 does not allocate motor PWM pins or advertise variable speed.

## Power architecture

The 3S protected battery supply splits after the master switch. A dedicated LM2596 logic rail supplies approximately 5 V to ESP32 VIN, SG90, HC-SR04, suitable sensors, and L298N logic. A separate LM2596 motor rail supplies approximately 6 V to L298N motor Vs and the nominal 6 V motors. Grounds are common; the two regulated outputs must not be joined. The L298N 5V-EN jumper is removed.

## Stage-1 behavior

- Motor GPIOs are configured and driven to STOP before sensor, servo, Wi-Fi, or dashboard initialization.
- HC-SR04 acquisition remains cooperative and has a finite timeout.
- IR active polarity is centralized in `RobotConfig::IR_ACTIVE_LEVEL`.
- Manual forward motion is blocked by a front obstacle, while reverse, turn-away, and STOP remain available.
- Auto retains ultrasonic navigation. Left-only IR recovery favors a right turn; right-only IR recovery favors a left turn; both IR inputs stop on the next service loop and use the existing scan/recovery path.
- Dashboard telemetry includes `left_ir_raw`, `right_ir_raw`, `left_obstacle`, `right_obstacle`, uptime, product identity, and firmware identity.

## Disabled and reserved Stage-2 hardware

MQ-2 (GPIO4), DHT22 (GPIO5), flame sensing (GPIO6), future SPI (GPIO11-13), future nRF24 (GPIO14-15), and GPIO16 remain reserved and are not initialized. Display, buzzer, OLED, MQ-135, CPS communication, combined dashboard, and electrical power sensing are not installed. Disabled features report `DISABLED` or `NOT_INSTALLED`; they cannot create Stage-1 hazards.

## Verification on 2026-09-07

- `pio run -t clean`: PASS. PlatformIO emitted its known post-success telemetry-cache permission traceback; cleaning itself completed successfully.
- `pio run`: PASS with no compiler warnings.
- RAM: 50,200 / 327,680 bytes (15.3%).
- Flash: 896,789 / 6,553,600 bytes (13.7%).
- Generated partition table: two 6,400 KiB OTA app slots, 3,456 KiB SPIFFS, and 64 KiB coredump within 16 MB.
- ESP32-S3 SDK variant: `qio_opi`; its SDK configuration enables octal PSRAM boot initialization.
- Dashboard JavaScript: syntax PASS in Deno/V8 (30,933 characters).
- Static GPIO review: no active references to the old motor/sensor/display pins and no active use of reserved Stage-2 pins.

These are build and static verification results. They are not physical runtime or instrumented latency measurements.

## Live validation boundary

`pio device list` returned no serial devices, including with direct hardware access. Consequently, no flash erase, upload, serial monitoring, sensor stimulation, dashboard loading, or physical motor command was performed. The old CSI firmware replacement remains pending until the actual ESP32-S3 is visible.

When connected, the first Rev A deployment must perform one full flash erase, then upload and monitor at 115200. Physical checks still required are safe STOP boot, 16 MB flash and 8 MB PSRAM runtime reporting, servo centering, HC-SR04 updates, both IR polarity/state changes, Wi-Fi/HTTP startup, dashboard loading, controlled Manual commands ending in STOP, physical wheel direction, and supervised Auto recovery.

## Next Stage

Stage-2 remains future work: MQ-2, DHT22, flame sensing, OLED/display selection, nRF24, and CPS integration.
