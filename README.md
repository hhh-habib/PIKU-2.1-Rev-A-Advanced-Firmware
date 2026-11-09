Current narrow IMU/OLED update (2026-11-07): checked MPU6050 frames and cooperative idle-only reconfiguration; compact fixed OLED with priority environmental alerts and cached STA IP. Software checks/build passed; physical acceptance is pending. Navigation, environmental sensing/safety, networking and radio behavior remain preserved. See [verification and owner checklist](docs/IMU_OLED_SOFTWARE_VERIFICATION_20261107.md). The dated records below retain their historical scope.

Documentation: [organized documentation index](docs/README.md) and [five editable research documents](docs/PAPER_READY/README.md).

## CURRENT IMPLEMENTED FIRMWARE - 2026-10-18

This working tree contains the restored V3.2 navigation plus the later sonar-evidence, command/session, persistent Manual-control, diagnostics and dashboard-readability corrections. HEAD remains `fa375a7bf739fc306060048b500c2acf413db7a7`; the newer working source is uncommitted. See the [final comparison and cleanup report](docs/FINAL_RELEASE_REVIEW_20261018.md) and [file-level preservation/cleanup manifest](docs/FINAL_RELEASE_CLEANUP_MANIFEST_20261018.csv).

- **Manual:** select once, hold a fresh direction, release to stop, then press another direction. Browser refresh is 250 ms; the independent firmware deadman stops expired movement at 750 ms when serviced. Transport uncertainty retires the held command; confirmed STOP/RELEASE and valid current authority restore readiness for the next fresh press. Heartbeat alone cannot establish STOP success.
- **AUTO/recovery:** ordinary obstacles and IR use the restored reverse/scan/turn path. Boxed recovery commits a usable side, uses approximately 25-degree turns, at most **7 started turns**, **5 IR reverse pulses**, and a **30,000 ms elapsed mission timeout**. Exhaustion stops, then the **2,500 ms cooldown** permits a fresh scan/retry. These are per-mission bounds, not a lifetime escape guarantee.
- **Evidence and safety:** trusted sonar quorum/timestamps, STOP priority, one-use delayed-mode protection, known-obstacle and environmental guards remain. Missing DHT/MQ-2 monitoring alone does not inhibit motion; a confirmed hazard is not cleared by a failed read. Sonar/hazard interruptions discard old actuator work; unfinished environmental recovery requires fresh evidence.

**OWNER PHYSICALLY OBSERVED / OWNER-REPORTED:** the owner reports the latest firmware is performing well. This audit did not operate or flash the robot and did not independently identify its flashed binary. **SOFTWARE-ONLY VERIFICATION:** full pre/post host/browser tests and ESP32-S3 builds are recorded in the report. Measured latency, turn accuracy, reverse distance and RF reliability remain unverified. Future CPS integration is outside this standalone release.

## Historical development snapshots (preserved)

The version-labelled descriptions below record earlier development states. In particular, V4.5 six-turn/45-second/phase-resumption descriptions and old pending-physical labels are historical; the CURRENT section and source-backed report above describe this working tree. Dated experiments and their original claims remain intact.

## Current standalone restoration (2026-10-08)

The existing project now restores V3.2 ordinary AUTO and stuck/boxed-in recovery, with the owner-approved environmental decisions and existing V4.6 STOP/control infrastructure preserved. See [the source and software verification report](docs/V32_RESTORATION_SOFTWARE_VERIFICATION.md) for the current behavior, evidence, limitations, and physical test checklist. Earlier V4.5/V4.6 navigation descriptions below remain historical development records. Physical validation of this restored build is pending.

> Current collision-closure software evidence (2026-10-05): [verification, regressions and residual limits](docs/COLLISION_CLOSURE_SOFTWARE_VERIFICATION.md). Physical validation and RF PA/range engineering NOT RUN.

## Current V4.6 software verification

Owner correction (2026-10-06): DHT/MQ-2 unavailability alone is monitoring-only and does not inhibit AUTO/MANUAL/BOXED movement or change its phase/time/authority. Failed readings never renew last-success evidence or clear a confirmed hazard. Confirmed hazards and independent STOP/sonar/IR/IMU/authority gates remain effective. Monitoring can be unavailable while movement is active; this does not prove environmental safety.

[V4.6 NORMAL AUTO and mode continuity verification](docs/V46_SOFTWARE_VERIFICATION.md) records the current owner correction. [V4.5 software verification](docs/V45_SOFTWARE_VERIFICATION.md) remains the historical baseline for unchanged BOXED, sensing and dispatch contracts. Older V3/V4 reports, manifests and supplied architecture figures remain historical. Software checks do not replace the next independent source audit or physical validation.

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

NORMAL AUTO uses current trusted front evidence before a new action. A clear-front single IR starts the opposite approximately35-degree turn directly, then STOP/settle and a genuinely NEW front checkpoint. The same single source may repeat35 while each new front is clear; its own blocked post-turn front requires one bounded backoff then a NEW full L/F/R scan. An opposite single before fresh-clear normal FORWARD is actually re-admitted uses one backoff/full scan; only real escape or fresh mode admission clears that contact history.

An ordinary front obstacle (with zero, one or both IR active) takes priority: STOP -> one bounded backoff -> STOP -> NEW front. If still blocked, a second bounded backoff precedes a NEW full scan. If clear, current IR rules apply; held BOTH goes directly to full scan because the first backoff already satisfied it. Clear-front BOTH uses one backoff then full scan. There is no persistent once-only BOTH suppression: each new intervention must finish its required fresh evidence before another is admitted. The existing NORMAL pulse remains50ms prepare/220ms reverse/150ms settle at min(selected,75%). Invalid/stale/wrong-facing sonar cannot authorize motion.

A complete compatible all-finite L/F/R<30 transfers once to unchanged V4.5 BOXED recovery. Otherwise an eligible normal scan side is selected; no eligible side keeps AUTO active and takes one bounded reverse followed by fresh full scan, subject to existing safety. An accepted normal scan completes motor-off. MANUAL -> AUTO retires all old NORMAL intents/history and evaluates current trusted front/IR without an automatic initial full scan. The dashboard waits for outstanding STOP/RELEASE responses before sending mode; declared MANUAL maintenance STOP is retired after leaving MANUAL, while genuine operator and AUTO lifecycle STOP retain their fail-safe authority.

### Boxed recovery V4.5

BOXED admission requires a complete compatible trusted positive finite L/F/R scan with every range below 30 cm. The greatest usable side is committed once per round; a tie retains the recovery direction, with a stable initial default. Positive 4/6 cm sides remain eligible. Each round permits six started approximately 25-degree turns, with required fresh gyro evidence and the original 2500 ms turn deadline. After a turn, a newly requested REGULAR front checkpoint completes recovery at >=30 cm or trusted FRONT OPEN, even with IR held; otherwise only the chosen side is checked before another turn.

Completion retires outgoing recovery jobs before V4.6 NORMAL arbitration. Current trusted clear front with held single IR selects the opposite normal35; BOTH starts its bounded normal intervention. Without IR, a new normal front validation precedes forward. BOXED phase/accounting and its clear-completion rules are unchanged.

Ordinary BOXED IR causes one bounded approximately 200 ms reverse at min(selected speed,75%), STOP, then the SAME saved phase. An active turn resumes its verified remainder in the same slot and original deadline; interrupted regular FRONT or chosen-side sensing renews only that existing phase. No extra post-pulse front checkpoint, full scan, side selection, round reset or time refill is inserted. Serviced held IR does not immediately repulse the resumed turn; a later regular blocked checkpoint or new unserviced confirmed input can intervene. There is no five-pulse mission ceiling. IR clear and incidental sonar clearance cannot complete BOXED.

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

BOXED admission requires a complete compatible trusted positive finite L/F/R scan with every range below 30 cm. The greatest usable side is committed once per round; a tie retains the recovery direction, with a stable initial default. Positive 4/6 cm sides remain eligible. Each round permits six started approximately 25-degree turns, with required fresh gyro evidence and the original 2500 ms turn deadline. After a turn, a newly requested REGULAR front checkpoint completes recovery at >=30 cm or trusted FRONT OPEN, even with IR held; otherwise only the chosen side is checked before another turn.

Ordinary BOXED IR causes one bounded approximately 200 ms reverse at min(selected speed,75%), STOP, then the SAME saved phase. An active turn resumes its verified remainder in the same slot and original deadline; interrupted regular FRONT or chosen-side sensing renews only that existing phase. No extra post-pulse front checkpoint, full scan, side selection, round reset or time refill is inserted. Serviced held IR does not immediately repulse the resumed turn; a later regular blocked checkpoint or new unserviced confirmed input can intervene. There is no five-pulse mission ceiling. IR clear and incidental sonar clearance cannot complete BOXED.

A chosen recovery direction has a 45000 ms active-work ledger. Same-direction rounds carry spent time; a genuine opposite BOXED commitment starts a new ledger. Confirmed hazard/environmental, sonar, integration and required IMU/authority holds, plus independent normal IR maneuvers, pause without refilling it. Owned between-round reassessment counts. A sixth completed blocked checkpoint retires its round to an approximately 1000 ms nonblocking reassessment and NEW full scan. Explicit 45-second expiry also retires once; complete fresh boxed proof after reassessment may admit the same side with a new 45-second run. Qualified regular clear beats mere count/time boundaries; current STOP and safety still win. There is no old six-turn assistance hold, five-second forward handoff lock or lifetime escape guarantee.

EnvironmentalRecovery remains a separate safety owner: continuous hazard-clear wait, a complete fresh compatible L/F/R scan, clear environmental IR gates and any required NEW post-turn front proof precede completion. Sonar or integration interruption restarts the required evidence. Its work pauses the recovery direction ledger. Compatible fresh boxed geometry transfers once to BOXED after outgoing environmental work is retired; rotated pre-turn side readings cannot prove a current boxed set. STOP, temperature rearm, sonar and integration gates retain priority.