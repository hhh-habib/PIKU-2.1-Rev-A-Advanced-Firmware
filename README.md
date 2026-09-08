# PIKU V2.0.1 Advanced Firmware

## 1. Project overview

PIKU is a developing cyber-physical environmental inspection robotic platform. This repository contains the ESP32 firmware that coordinates locomotion, autonomous obstacle handling, directed ultrasonic inspection, environmental sensing, local visualization, alarms, and browser-based operation.

The firmware targets controlled research, education, and prototype demonstrations. It is not a certified industrial, fire, gas, or life-safety system.

## 2. Current version and status

- Version: PIKU V2.0.1 Advanced
- PlatformIO environment: `esp32doit-devkit-v1`
- Framework: Arduino for ESP32
- Status: prior Manual/Auto physical test record preserved; corrective firmware clean-build verified
- Release branch: `main`

The finalized project builds successfully without compiler warnings. The corrective changes were not flashed or physically retested during repository finalization; see [docs/TESTING.md](docs/TESTING.md) for the exact distinction.

Release provenance is intentionally non-destructive: annotated tag `v2.0.1` preserves the historical release, annotated tag `v2.0.1-advanced` identifies the audited Advanced baseline (`4432affa`), and current `main` contains the subsequent corrective finalization.

## 3. Major firmware capabilities

- Manual and autonomous drive modes
- L298N four-input motor direction control
- Pivot and spin turning modes
- Servo-mounted HC-SR04 ultrasonic scanning
- Manual LEFT, FRONT, and RIGHT inspection
- Interactive browser radar and live telemetry
- DHT22 temperature and humidity monitoring
- MQ-2 filtered gas monitoring with warm-up state
- Flame and close-range IR obstacle inputs
- Physical buzzer, TFT alerts, and browser warnings
- SoftAP dashboard plus Home Wi-Fi station operation
- Central safety classification and motion blocking
- Autonomous obstacle scanning, route selection, and recovery

## 4. Manual and autonomous operating modes

In `MANUAL`, dashboard commands are routed through `DriveCommandHandler` to `MotorController`. While a direction is held, the dashboard refreshes it every 250 ms. A firmware deadman stops motion if no refresh arrives for 750 ms, so a lost browser/network path cannot leave the last Manual command energized. Releasing a direction or pressing STOP still sends an explicit STOP. Critical hazards block motion, and front obstacle checks reject unsafe forward movement.

When a drive command is requested while the ultrasonic head is facing sideways, the motors stay stopped, the head returns to FRONT, the existing 700 ms settling interval completes, and a fresh front sample is evaluated before the pending command is accepted or rejected.

In `AUTO`, `NavigationManager` acquires front distance samples, advances when the route is clear, scans LEFT/FRONT/RIGHT when obstructed, selects a valid clearer route, and performs the established near-obstacle recovery sequence. Dashboard STOP is available in every mode and promptly interrupts Auto forward, scan, reverse, turn, and settling operations. An Auto STOP remains latched until the operator deliberately issues another mode command. A Manual/Auto mode change also interrupts the active Auto sequence before any later step can re-energize the motors.

## 5. Manual LEFT / FRONT / RIGHT ultrasonic inspection

The Manual sensor-head controls use logical directions from PIKU's own forward-facing perspective:

- LOOK LEFT inspects PIKU's physical left side.
- LOOK FRONT centers the head.
- LOOK RIGHT inspects PIKU's physical right side.

Each operation stops the drive motors, moves the existing servo, waits for settling, requests a fresh ultrasonic sample, and publishes a confirmed inspection orientation. The mechanically mirrored Manual installation keeps physical servo actuation separate from the logical direction used by the dashboard, radar, and warning text.

A valid sample at or below 7 cm produces a directional inspection warning and activates the physical hazard buzzer. Side inspection warnings remain separate from Auto navigation and are not classified as front obstacles.

## 6. Interactive ultrasonic radar

The web dashboard renders the active scan orientation, logical servo angle, fresh distance samples, and short-lived radar points. In both Manual and Auto modes, LEFT, FRONT, and RIGHT presentation follows PIKU's physical perspective. The mechanically mirrored installation therefore maps physical servo commands separately from the logical radar angles; Auto route selection still uses the established physical scan mechanics.

The radar consumes the existing `/data` telemetry stream; it does not control the servo directly.

## 7. Environmental and safety monitoring

`SensorManager` schedules and caches:

- HC-SR04 ultrasonic distance
- DHT22 temperature and humidity
- MQ-2 raw and filtered ADC values
- Flame sensor state
- IR obstacle state

`SafetyManager` classifies caution, warning, near-obstacle, and critical states. Critical flame, high-risk gas, or critical-temperature hazards stop and block motion. Near front obstacles and ultrasonic warnings block unsafe forward movement according to the existing safety rules.

The established HC-SR04 policy is deliberate: a clean timeout/no-echo result is treated as clear/out-of-range and permits motion, while malformed or otherwise abnormal acquisition states remain sensor faults. DHT22 read failures retain the last known temperature/humidity values; telemetry freshness is not presently validated, so retained environmental readings must not be mistaken for a fresh sample.

Alarm truth is independent from sound control. Muting silences the physical/browser alarm output but does not clear hazard state, dashboard warnings, TFT warnings, or motor blocking.

## 8. SoftAP and Home Wi-Fi dashboard access

The ESP32 starts its local SoftAP and serves the dashboard directly. Connect to the configured PIKU access point, then open:

```text
http://192.168.4.1/
```

Home Wi-Fi can be scanned, configured, saved, reconnected, and cleared through the dashboard. Station credentials are stored at runtime using ESP32 Preferences. The compile-time Home Wi-Fi fallback values in `include/WiFiConfig.h` are intentionally empty.

Do not hard-code personal Home Wi-Fi credentials. Configure them through the supported dashboard provisioning flow. Because this repository is private and is the owner's competition/recovery source, `include/Secrets.h` is intentionally tracked with the operational SoftAP configuration used by the physically tested firmware. An authorized fresh clone already contains this file; `include/Secrets.example.h` remains only a safe reference/template. Never make the repository public without first reviewing, removing, or rotating private credentials. Earlier commits and preserved historical snapshots can retain the former bootstrap credential because history was intentionally not rewritten; treat historical access as sensitive. The dashboard uses unauthenticated HTTP, so operate it only on a trusted, supervised network and do not expose it to an untrusted LAN or the public Internet.

## 9. Software architecture and major classes

- `main.cpp`: top-level setup, service loop, Manual coordination, telemetry, and mode transitions
- `NavigationManager`: Auto navigation, scanning, obstacle recovery, and route selection
- `MotorController`: L298N motor direction and motor-state ownership
- `DriveCommandHandler`: Manual command validation and dispatch
- `ServoScanner`: the single servo abstraction for center/left/right/arbitrary positioning
- `SensorManager`: scheduled sensing and coherent sensor snapshots
- `SafetyManager`: hazard classification and motion-blocking decisions
- `BuzzerManager`: non-blocking physical alarm patterns and mute/test behavior
- `DisplayManager`: ST7789 telemetry and warning presentation
- `DashboardPublisher`: telemetry synchronization for dashboard, TFT, and serial output
- `WebDashboard`: SoftAP/station networking, HTTP routes, JSON telemetry, provisioning, and command intake
- `WebDashboardAssets`: compiled dashboard HTML, CSS, JavaScript, and radar interface

## 10. Hardware platform

- ESP32 DOIT DevKit V1
- L298N motor driver and mobile chassis
- Servo-mounted HC-SR04 ultrasonic sensor
- DHT22 temperature/humidity sensor
- MQ-2 gas sensor through a voltage divider
- Digital flame sensor
- Digital IR obstacle sensor
- GPIO-driven active buzzer
- ST7789 TFT display

## 11. GPIO overview

| Function | GPIO |
| --- | ---: |
| TFT CS | 5 |
| TFT DC | 22 |
| TFT RST | 4 |
| TFT SCK | 18 |
| TFT MOSI | 23 |
| Right motor IN1 / IN2 | 26 / 27 |
| Left motor IN1 / IN2 | 16 / 17 |
| Servo signal | 13 |
| HC-SR04 trigger / echo | 32 / 34 |
| DHT22 data | 21 |
| MQ-2 analog input | 35 |
| Buzzer output | 33 |
| Flame input | 36 |
| IR obstacle input | 39 |
| Reserved future motor PWM | 25 / 14 |

GPIO34, GPIO35, GPIO36, and GPIO39 are input-only on the ESP32. GPIO25 and GPIO14 remain reserved and are not used by the present firmware.

## 12. PlatformIO build instructions

Install PlatformIO Core or use the PlatformIO IDE extension, then run from the repository root:

```sh
pio run
```

For a clean verification:

```sh
pio run --target clean
pio run
```

The Espressif32 platform, Arduino-ESP32 framework package, and all declared libraries are version-pinned in `platformio.ini` for reproducible resolution. An authorized fresh clone of this private repository already includes the tracked operational `include/Secrets.h`; it does not need to be recreated. `include/Secrets.example.h` is retained only as a safe reference/template.

## 13. Firmware upload instructions

Connect the correct ESP32 board, verify the serial port and wiring, keep the chassis safely elevated, and run:

```sh
pio run --target upload
```

Uploading is intentionally separate from building. No firmware was uploaded as part of this repository publication.

## 14. Dashboard access instructions

1. Power the PIKU controller safely.
2. Connect a phone or computer to the configured PIKU SoftAP.
3. Open `http://192.168.4.1/`.
4. Use Manual mode for directed driving and sensor-head inspection.
5. Use the Wi-Fi panel to provision Home Wi-Fi when needed.
6. Switch to Auto only in a controlled test area with an accessible power cutoff.

The browser must remain connected to the robot's network path to receive telemetry and issue commands.

## 15. Safety and wiring notes

- Verify the GPIO map against the installed hardware before power-up.
- Use the required voltage divider on the MQ-2 analog path.
- The physical-system diagram documents an HC-SR04 5 V-to-3.3 V echo divider, but the available physical record does not electrically verify its installation or ratio. Confirm the actual divider and GPIO34 voltage before power-up.
- The installed buzzer is documented as active hardware; firmware drives it with simple ON/OFF alarm windows rather than a generated tone.
- Tune the IR module physically for the installed chassis.
- Keep grounds common and use a power arrangement suitable for motors, servo, sensors, and ESP32.
- Keep a manual power cutoff available during motion tests.
- Do not rely on this prototype as certified safety equipment.

## 16. Testing status

The Advanced baseline was reported physically tested in both Manual and Auto modes, including motor control, autonomous navigation, servo scanning, three-direction Manual inspection, radar direction agreement, continuous 7 cm inspection alarms, recenter-before-driving safety, environmental warning paths, TFT presentation, buzzer operation, and SoftAP/Home Wi-Fi dashboard access.

Testing was functional prototype testing, not formal certification or calibrated performance validation. No reset was observed during the reported final test session. The later deadman, Auto interruption, radar presentation, active-buzzer, credential, and dependency corrections passed a clean compile but were not uploaded or claimed as newly physically tested.

## 17. Repository structure

```text
.
|-- include/              Firmware headers and configuration
|-- src/                  Firmware and compiled dashboard sources
|-- docs/TESTING.md       Final physical-test record
|-- platformio.ini        Board, framework, and dependencies
|-- CHANGELOG.md          Development history
|-- .gitignore            Publication exclusions
`-- README.md             Project and operating documentation
```

Generated builds, Git metadata, editor state, ignored local archives, and firmware binaries are excluded. The operational `include/Secrets.h` is intentionally tracked only because this is the owner's private competition/recovery repository; review, remove, or rotate private credentials before any public release. Historical snapshots and recovered release documents are retained locally under ignored `archive/` paths with provenance records.

## 18. Future development direction

- Calibrate MQ-2 response with the installed divider and target environment.
- Electrically verify the installed HC-SR04 echo divider and update the physical record with measured values.
- Evaluate optional ENA/ENB PWM using the reserved pins.
- Add host-side tests for deterministic safety and navigation logic.
- Add authenticated transport/control if the dashboard must ever operate beyond a trusted supervised network.
- Expand documented environmental inspection experiments without overstating sensor accuracy.

## 19. Author and research role

**Md Habibur Rahman Habib**  
Research Assistant  
First Author and Chief Robotics Engineer

PIKU is being developed as a cyber-physical environmental inspection robotics research platform.
