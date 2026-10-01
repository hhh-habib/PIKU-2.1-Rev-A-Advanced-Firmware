# PIKU 2.1 Rev A - Advanced Firmware

PIKU 2.1 Rev A is a standalone ESP32-S3 mobile environmental-inspection research robot. This repository contains its firmware, embedded browser dashboard, host tests, and validation records. The later Stage 4 CPS integration is a separate project.

## Current release

**Status:** physically validated standalone research firmware, 2026-10-01, as reported by the project owner.

**Frozen V3.2 executable source:** `41281d8325d88c70e074e008a2550a1164102132`. The release documentation adds no firmware or test behavior changes. The compiled firmware still reports `2.1.0-revA-stage2` internally; V3.2 identifies this validated source and its recovery behavior, not a change to that frozen version string.

The owner reported a supervised functional physical PASS for normal AUTO, boxed recovery, locked direction, IR-assisted clearance, Manual controls and override, sonar fault recovery, dashboard status, and operator STOP. This was functional validation, not a statistically instrumented benchmark or safety certification. See the [V3.2 physical-validation record](docs/PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md) and [software verification](docs/RECOVERY_V32_SOFTWARE_VERIFICATION.md).

## Hardware and software at a glance

| Part | Role |
| --- | --- |
| ESP32-S3 N16R8 | Runs the state machines, control service loop, dashboard, and Wi-Fi |
| L298N and four-wheel chassis | Drive direction and user-selected PWM speed |
| SG90 and HC-SR04 | Front ranging and left/front/right directional scans |
| Two low front IR sensors | Immediate close-chassis obstacle evidence |
| MPU6050 | Short-term relative turn measurement and motion telemetry |
| DHT22, MQ-2, flame module | Environmental monitoring and local hazard response |
| SH1106 OLED and nRF24 | Local status and secondary experimental radio path |

Pin assignments and executable thresholds are defined in `include/PinConfig.h` and `include/RobotConfig.h`. The firmware serves its own dashboard; no external application server is required.

## Operating modes

### Manual: supervised movement

Forward, Backward, Left, Right, and STOP are available with Pivot or Spin turning and a selected speed. Held controls refresh their command; release, STOP, mode changes, loss of a required control session, and the 750 ms Manual command timeout stop motion. Backward and turns remain operator-supervised; the robot has no rear sensor or complete swept-volume view.

Manual Forward uses **fresh finite front sonar** with hysteresis: **20 cm or less blocks**, **25 cm or more clears**, and readings between them retain the previous block state. A trusted no-echo result is open. SensorFault or Stale is unknown, not a sonar-only Manual movement inhibit. Either active front IR sensor still blocks Forward independently, including when sonar is unavailable. Known turn obstructions, critical temperature, and other higher-priority stops remain in force.

The intentional Manual hazard override bypasses only the local flame/gas movement lock. It does not bypass STOP, applicable dead-man and control-session rules, critical temperature, known obstacle guards, or other safety conditions. Flame/gas detection remains visible; override does not mean the environment is safe.

### AUTO: ordinary obstacles

Normal AUTO reacts to current front sonar and IR evidence. A finite front reading at or below **20 cm** starts ordinary obstacle handling: stop, a short bounded reverse, a fresh left/front/right scan, safe-side selection, an approximately **35-degree** gyro-assisted turn, and fresh re-evaluation. A trusted `CLEAR_NO_ECHO` is operationally open. If no path is safe, AUTO stops and uses its bounded retry behavior; it does not require repeated AUTO button presses for an ordinary recoverable obstacle.

### Boxed-in Recovery V3.2

A trusted complete scan enters recovery immediately when **all three finite** left, front, and right distances are **strictly below 30 cm**. Any trusted `CLEAR_NO_ECHO` prevents that boxed-in classification. Recovery reuses the completed scan when possible, chooses LEFT or RIGHT once, and locks that side for the mission.

Recovery turns about **25 degrees** toward that side, stops, and checks **front only**. A valid front distance of **at least 30 cm**, or trusted `CLEAR_NO_ECHO`, ends recovery immediately and hands control to normal AUTO. If front remains blocked, recovery checks only the locked side before another increment. A selected-side reading of **7 cm or less** vetoes the pivot. There is no repeated full scan after every turn and no recovery-specific forward trial.

Either IR sensor can interrupt recovery motion. It stops the robot and starts bounded reverse clearance: up to **five 200 ms pulses per mission**, stopping and checking IR between pulses. IR never changes the locked side. When IR clears, recovery checks front and continues on the same side only if still needed. With no usable heading after at most **seven turns**, the mission stops, waits about **2,500 ms**, then starts a fresh scan and a new mission.

### Sonar fault and reacquisition

`DistanceValid` is a trusted finite range. `ClearNoEcho` means no useful return and is treated operationally as **OPEN**, not as a fault. `SensorFault` and `Stale` are untrusted.

AUTO and recovery stop on SensorFault or Stale, discard an active turn or IR backoff, and enter `SONAR_REACQUIRE`. Motors stay off while normal-cadence **front-only** samples continue. Two consecutive fresh trusted front results (finite or no-echo) trigger a fresh normal AUTO decision automatically. The **1,000 ms** fast observation window does not cause blind motion or a reboot; if sensing remains faulty, the robot stays stopped and keeps sampling until it recovers.

## Dashboard and communication

The Control page places mode, turn mode, speed, hazard override when applicable, alarm controls, movement buttons, and servo-head controls before diagnostics. Ultrasonic badges read **VALID** with centimetres, **OPEN** for no-echo, **FAULT**, or **STALE**.

The ESP32-S3 supports direct SoftAP access and provisioned STA/Home Wi-Fi with reconnect handling. The browser uses the existing HTTP control path, `/ping` heartbeat, and `/data` telemetry; nRF24 remains a secondary experimental path. STOP and safety handling take priority over display and telemetry work. Operate the browser on a trusted, supervised network.

## Validation and evidence boundaries

The frozen source passes `pio run -e esp32-s3-n16r8`, **10/10 native C++ tests**, **3/3 Python tests**, and embedded-dashboard JavaScript parsing. The build uses **58,740 / 327,680 bytes RAM** and **1,038,217 / 6,553,600 bytes flash**. The physical PASS above is the project owner's report for the final supervised test; earlier dated observations remain in the [historical physical testing record](docs/PIKU_2.1_REV_A_TESTING.md).

This robot has **no rear obstacle sensor or encoder-confirmed reverse distance**. Automatic IR clearance reverse is time-based. The HC-SR04 cannot perfectly distinguish a disconnected LOW echo line from a true no-return, so operational `CLEAR_NO_ECHO` is not proof of infinite free space. IR observes local obstacles, not the complete swept volume. MQ-2 is not an industrial calibrated gas instrument. Turn-angle error distributions, response-latency distributions, reverse displacement, and RF reliability statistics were not instrumented for V3.2. This is a research prototype, not certified industrial safety equipment.

## Build and repository map

```powershell
pio run -e esp32-s3-n16r8
```

- `src/`, `include/`, `platformio.ini`: frozen executable firmware and embedded dashboard.
- `tests/`: host regression and source-contract tests.
- `docs/RECOVERY_V32_SOFTWARE_VERIFICATION.md`: reproducible V3.2 software evidence.
- `docs/PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md`: owner-reported supervised physical confirmation.
- `docs/PIKU_2.1_REV_A_TESTING.md`: dated earlier Rev A physical-test history.
- `SOFTWARE_EXPLANATION.md`: plain-language execution and safety walkthrough.
- `CHANGELOG.md`: concise release history.

Existing `architecture_source/` diagrams and corresponding figures preserve earlier normal AUTO and control-flow views; they do not show every V3.2 locked-recovery or sonar-reacquisition state. Use this README, the software explanation, and the frozen source for current behavior. No figure was altered for this documentation release.

The repository includes deployment-specific configuration. Do not disclose credentials or copy `include/Secrets.h` into public reports.

## Next stage

Stage 4 CPS integration can build on this frozen standalone reference in its own repository. This documentation release makes no change to Stage 4 or the validated executable firmware.
