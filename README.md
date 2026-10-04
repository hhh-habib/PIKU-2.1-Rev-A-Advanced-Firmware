> Current state-collision software candidate (2026-10-05): [production regression and system evidence](docs/SYSTEM_STATE_COLLISION_VERIFICATION.md). Physical validation and RF PA/range engineering NOT RUN.

# PIKU 2.1 Rev A - Advanced Firmware

PIKU 2.1 Rev A is a standalone ESP32-S3 mobile environmental-inspection research robot. This repository contains its firmware, embedded browser dashboard, host tests, and validation records. The later Stage 4 CPS integration is a separate project.

## Current software candidate

**PIKU 2.1 Rev A V3.3.1 SOFTWARE CANDIDATE - physical validation NOT RUN.** Standalone remains the Robot-brain authority. The historical physically validated V3.2 baseline is executable `41281d8325d88c70e074e008a2550a1164102132` (owner-reported supervised functional PASS, 2026-10-01), preserved in its [software record](docs/RECOVERY_V32_SOFTWARE_VERIFICATION.md) and [physical record](docs/PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md).

V3.3.1 adds only a 75% maximum on the existing calibrated user speed for RecoveryIrBackoff pulses: `min(selected,75)`, preserving selected speed and normal motion. See [focused hardening evidence](docs/V33_1_HARDENING_SOFTWARE_VERIFICATION.md).

The preserved V3.3 baseline adds deterministic episode budgets, sonar/hazard pause and safe continuation, and persistent Robot-level Manual Override permission. See [V3.3 verification](docs/RECOVERY_V33_SOFTWARE_VERIFICATION.md), [core difference audit](docs/STAGE5D_CORE_DIFFERENCE_AUDIT.md), and the [pending owner checklist](docs/PIKU_2.1_REVA_V33_PHYSICAL_VALIDATION.md).

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

### Boxed-in Recovery V3.3

A trusted complete all-finite L/F/R scan strictly below 30 cm begins one persistent Recovery Episode. Trusted OPEN prevents false boxed classification. Recovery commits a side and commands approximately 25-degree increments, with at most **six turn attempts per episode** (150-degree target sum). An attempt is consumed when started, including interrupted attempts. The 7 cm selected-side hard veto remains.

After each turn the motors stop for fresh front/side and IR evidence. FRONT >=30 cm or trusted OPEN permits a handoff candidate, preserving the episode until **5000 ms of continued trusted forward-safe motion** through the existing nonblocking forward steps. A brief clear reading cannot grant a new budget. While active, the episode also owns all body turns; ordinary 35-degree avoidance cannot bypass its accounting.

Either IR input can interrupt motion for reverse pulses at `min(selected speed,75%)` on the calibrated user scale: at most five approximately 200 ms pulses for the entire episode. Cooldown, scan restarts, hazards and sonar reconnection preserve both budgets. Exhaustion stops the motors in assistance HOLD; cooperative stationary front sensing after the 2500 ms cooldown can find a safe handoff without more turns. A new budget requires genuine stable progress, explicit mode restart/reset, or reboot. STOP terminates motion and latches AUTO stopped until an explicit mode command.

### Sonar fault and reacquisition

DistanceValid is finite trusted range; ClearNoEcho is operational OPEN. SensorFault/Stale in AUTO immediately stop the motors and cancel active turn/pulse execution, center sonar and enter SONAR_REACQUIRE. Episode counts and committed intent survive. Two new consecutive trusted front samples restart a fresh EnvironmentalRecovery L/F/R scan when that job is incomplete, retain TrappedHold when applicable, otherwise restore a fresh checkpoint for the same episode or a normal front decision. No interrupted turn/reverse command is replayed. The 1000 ms fast window is diagnostic only; the robot remains stopped and keeps sampling if fault persists.

### Persistent Manual Override

Override is explicit permission to bypass only eligible flame/gas inhibition in MANUAL. A legitimate live session and confirmation are required to enable it; each hazardous movement still needs current live authorization. RELEASE, STOP, speed, turn/head interactions and temporary link/session loss preserve permission. Session replacement stops old movement and requires a fresh direction press. Disable Override, leaving MANUAL, reboot, stable hazard clear, or non-overridable critical safety revoke it. Ordinary hazard-free MANUAL remains usable with Override OFF.

## Dashboard and communication

The Control page places mode, turn mode, speed, hazard override when applicable, alarm controls, movement buttons, and servo-head controls before diagnostics. Ultrasonic badges read **VALID** with centimetres, **OPEN** for no-echo, **FAULT**, or **STALE**.

The ESP32-S3 supports direct SoftAP access and provisioned STA/Home Wi-Fi with reconnect handling. The browser uses the existing HTTP control path, `/ping` heartbeat, and `/data` telemetry; nRF24 remains a secondary experimental path. STOP and safety handling take priority over display and telemetry work. Operate the browser on a trusted, supervised network.

## Validation and evidence boundaries

V3.3 software checks execute the policies and actual NavigationManager implementation with hardware doubles; they do not measure physical angle, reverse displacement, radio behavior, or reliability. The historical V3.2 build used 58,740 bytes RAM and 1,038,217 bytes flash. Current candidate build/test results are recorded in [V3.3 verification](docs/RECOVERY_V33_SOFTWARE_VERIFICATION.md).

This robot has **no rear obstacle sensor or encoder-confirmed reverse distance**. Automatic IR clearance reverse is time-based. The HC-SR04 cannot perfectly distinguish a disconnected LOW echo line from a true no-return, so operational `CLEAR_NO_ECHO` is not proof of infinite free space. IR observes local obstacles, not the complete swept volume. MQ-2 is not an industrial calibrated gas instrument. Turn-angle error distributions, response-latency distributions, reverse displacement, and RF reliability statistics were not instrumented for V3.2. This is a research prototype, not certified industrial safety equipment.

## Build and repository map

```powershell
pio run -e esp32-s3-n16r8
```

- `src/`, `include/`, `platformio.ini`: current candidate firmware and embedded dashboard.
- `tests/`: host regression and source-contract tests.
- `docs/RECOVERY_V32_SOFTWARE_VERIFICATION.md`: reproducible V3.2 software evidence.
- `docs/PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md`: owner-reported supervised physical confirmation.
- `docs/PIKU_2.1_REV_A_TESTING.md`: dated earlier Rev A physical-test history.
- `SOFTWARE_EXPLANATION.md`: plain-language execution and safety walkthrough.
- `CHANGELOG.md`: concise release history.

Current `architecture_source/` diagrams describe V3.3 recovery and permission boundaries. No deterministic tracked-figure regeneration workflow exists here, so rendering remains separate; historical tracked figures and the four pre-existing untracked figure PNGs are preserved.

The repository includes deployment-specific configuration. Do not disclose credentials or copy `include/Secrets.h` into public reports.

## Next stage

Stage 4 will synchronize this verified V3.3.1 cap while preserving its CPS wrappers. Physical validation remains pending.

## Current interruption and time accounting

Recovery retains six started 25-degree turns, five 200 ms IR backoff pulses at min(selected speed,75%), the committed side, fresh front checks, and the 2500 ms cooldown. Handoff is provisional until 5000 ms of trusted forward progress; it does not grant a new episode budget.

The secondary 30000 ms watchdog accumulates active scanning, deciding, turning, and backoff work. Sonar, hazard/clear-wait, integration holds, and retry cooldown exclude waiting time while preserving spent time. Temporary holds preserve the original TrappedHold deadline. Explicit AUTO STOP terminates the episode and latches AUTO until an explicit mode command; ordinary MANUAL STOP/RELEASE preserve eligible Override permission and require fresh movement authority.

EnvironmentalRecovery stays incomplete across temporary sonar or integration loss. After restoration, it restarts all L/F/R evidence; every decision sample is post-restoration. Two trusted front reacquisition samples permit that fresh scan, not Forward. A trapped episode finishes the environmental scan stationary and retains its cooldown; other active episodes return to their existing bounded decision after the full trusted scan. A watchdog exhausted during that scan enters policy HOLD and cannot start another recovery movement.
