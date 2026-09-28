# Approved v2 standalone execution record

## Baseline and scope

- Date: 2026-09-28; local baseline commit `e765ab756ad3faf6d6e83bf3b4b4eec14d1f76fd`; local annotated tag `approved-v2-baseline-20260928`. Git objects preserve the tracked baseline without copying the deployment secret header.
- Before edits: `master` at that commit, behind its remote by 3; `git diff --ignore-space-at-eol` and staged diff empty; only `figures/` untracked. The four untracked PNGs were left in place and not staged. Their SHA-256 values: Circuit_Diagram 14DC1F58406F4F455147AF7AB48082DACD1D239D2158706DDC6AB39024AA1356; Software_Architecture B154AE47020591E8CD4BBDF2F93C0016E7C81A86B69DD11888BD698FD4995C7F; System_Architecture 319D4AA6B65FE23D7979FD9F302641ADADE5F73405BA5A64E623D239DC9C541B; Technical_Blueprint_Black 66B1332FA3AB3DCEA9B16AEA13DDD055EA8F124A7F70853DBE65A4ED7382BA8C.
- `..` from this repository has no README despite the local AGENTS.md pointer. The repository README and Research root README were read. No source requirement conflict was found.
- Baseline build: `pio run -e esp32-s3-n16r8` PASS, 53,524/327,680 RAM (16.3%), 1,009,317/6,553,600 flash (15.4%), no compiler warnings in output. PlatformIO Espressif 32 7.0.1, Arduino-ESP32 package 3.20017.241212, ESP32Servo 3.2.1 and pinned libraries in `platformio.ini`. PlatformIO's generic board banner says N8 while project overrides specify 16 MB flash and OPI PSRAM; a physical board and PSRAM check remains required.
- Feature branch: `codex/approved-v2-standalone-20260928`. No remote action authorized.

## Source audit and baseline feature matrix

| Path | Implemented baseline | Source-level issue or limit |
| --- | --- | --- |
| `main.cpp` | priority `serviceLoop`, Manual command/dead-man, mode switching, early critical stop | Separate early critical and Manual motion gates require coordinated override work. |
| `SafetyManager` | flame, relative MQ-2, temperature, dual IR and ultrasonic classifications | `isCritical` groups gas, flame and temperature. |
| `NavigationManager` | non-blocking AUTO, forward validation, 170/90/10 scan, nominal 35-degree gyro spin, blocked retry, environmental auto-clear/rescan | Existing 220 ms automatic reverse lacks verified rear coverage. Blocked retry resets attempts; equal side choices can alternate. IMU STILL and fixed sonar are not position measurements. |
| `DriveCommandHandler` | Manual forward/reverse/pivot/spin/STOP | Explicit reverse is operator supervised but rear clearance is unverified. |
| `WebDashboard` and embedded JS | AP+STA, synchronous HTTP control, ~450 ms telemetry poll, ~250 ms held movement refresh, 750 ms movement dead-man | No transport authentication. Wi-Fi modem sleep is already disabled. |
| `SensorManager`, `ServoScanner`, `ImuManager` | DHT22, MQ-2, flame DO, dual IR, HC-SR04, servo scan, MPU6050 | No wheel odometry, rear sensor or battery voltage ADC is established. |
| `DisplayManager`, `RadioManager`, `DashboardPublisher` | SH1106 OLED, nRF24 secondary path, bounded presentation | No independent safety authority for rear motion. |

Pins: motors 39/40/41/42 and PWM 18; servo 21; ultrasonic 9/10; IR 7/8; IMU 1/2; MQ-2 4; DHT22 5; flame 6; SPI 11/12/13; nRF24 14/15; OLED 17/47/48. The fitted low IR zones face front left/right; the servo ultrasonic scans approximately left/front/right. None verifies the swept body envelope or rear corridor.

## Phase 0 diagnostics

Wi-Fi event callback now records STA disconnect reason code, last event code and AP transition counts with constant-time writes. The regular loop emits a transition record and a 30-second summary with firmware ID, uptime, STA state/RSSI, reconnect count, free/min heap, maximum service-loop gap, HTTP error/timeout counts. These counters are also in `/data`. Reset reason/brownout indication and the other service counters already existed in telemetry. Event codes and reset reasons are diagnostic evidence, not a root-cause conclusion. No battery voltage is inferred.

Reproducible supervised comparison: capture serial and `/data` for five minutes idle, five minutes held/manual movement, hazard activation/clearance, standalone SoftAP, and STA with each tested router; repeat near low battery while an external meter records supply voltage under load. Record timestamps, firmware, router/channel, distance, reset reason, disconnect reason, heap, RSSI, loop gap, HTTP counts and observed motor state. Keep wheels clear and physical STOP/power cutoff accessible. Compare counts and event ordering; do not convert qualitative responsiveness into latency measurements. The current code does not measure motor STOP latency.

## Bench checklist (pending physical validation)

1. Confirm 16 MB flash/8 MB PSRAM and stable supply with an external meter; keep wheels elevated.
2. Boot and verify STOP, reset reason, AP/STA access, OLED, nRF24 status, IMU, DHT22, MQ-2, flame DO and both IR zones.
3. Confirm serial diagnostic event/30-second rate and `/data` counters during idle, movement, both router modes and a controlled disconnect.
4. Verify Manual button release and dead-man stop motors; verify explicit STOP and mode changes. Measure latency externally if needed.
5. Test flame/gas and critical temperature separately, with real alarms visible throughout; verify AUTO continuous-clear/rescan behavior.
6. Test forward block, pivot clearance and operator-supervised reverse with direct human rear observation. Do not infer rear safety from front sensors.
7. If recovery V2 is later enabled, test ordinary obstacle, symmetric scans, narrow corridor, corner trap, invalid sonar, IMU outage, hazard/STOP preemption, and persistent trapped hold.
8. Preserve serial CSV and outcome notes without inventing position, gas ppm, or measured angles.
