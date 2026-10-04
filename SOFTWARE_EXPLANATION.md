> Current state-collision software candidate (2026-10-05): [production regression and system evidence](docs/SYSTEM_STATE_COLLISION_VERIFICATION.md). Physical validation and RF PA/range engineering NOT RUN.

# How the PIKU 2.1 Rev A firmware works

This guide describes the collision-hardened V3.3.1 software candidate. The V3.2 physically validated baseline remains historical evidence at `41281d8325d88c70e074e008a2550a1164102132`. See [current verification](docs/RECOVERY_V33_SOFTWARE_VERIFICATION.md) and the [pending owner checklist](docs/PIKU_2.1_REVA_V33_PHYSICAL_VALIDATION.md).

## The control path

PIKU repeatedly services browser commands, current sensors, safety decisions, and the selected mode. Motion is owned by `MotorController`; the dashboard displays state but does not drive GPIO itself.

```text
Sensors -> current evidence -> safety and operator STOP
                                 |
                                 +-> MANUAL: supervised held command
                                 |
                                 +-> AUTO: normal navigation
                                           |-> locked-side recovery
                                           +-> sonar reacquisition
                                 |
                                 +-> MotorController -> wheels
```

`main.cpp` starts the components and runs the cooperative loop. Accepted STOP/RELEASE and mode changes stop motors promptly; the loop also services Wi-Fi, sensors, telemetry, the OLED, and the secondary radio. Navigation and recovery use timestamps and phases so the service loop can keep running. This is priority-aware cooperative firmware, not a hard real-time safety controller.

## How evidence reaches a decision

`SensorManager` schedules ultrasonic, IR, flame, gas, temperature/humidity, and other samples. The HC-SR04 can face front or move with the SG90 head for a left/front/right scan. The ultrasonic state carries meaning separately from the centimetre field:

| State | Meaning in V3.3 |
| --- | --- |
| `DistanceValid` | Fresh trusted finite distance; compare the numeric range |
| `ClearNoEcho` | No useful echo; operationally OPEN, without inventing a distance |
| `SensorFault` | Untrusted fault evidence |
| `Stale` | Old/untrusted evidence |

`ClearNoEcho` is useful for path choice, but it is not a proof that all space beyond the sensor is empty. A disconnected LOW echo line may resemble a true no-return. The dashboard presents these states as VALID with centimetres, OPEN, FAULT, and STALE.

`SafetyManager` combines environmental hazards, low front IR zones, and near-obstacle evidence. It preserves immediate STOP and critical-hazard authority. IR covers a different low chassis zone from the higher servo-mounted HC-SR04; neither sensor gives a complete swept-volume guarantee.

## Manual: the operator holds authority

The browser offers Forward, Backward, Left, Right, STOP, Pivot/Spin selection, and speed. A held movement command is refreshed; release, STOP, mode change, command timeout, and applicable control-session loss stop motion. Backward has no rear-clearance certificate, and turns are supervised by the operator with the existing near/side vetoes.

The front ultrasonic Forward latch uses a **fresh finite** front result. At **20 cm or less**, Forward blocks or stops; at **25 cm or more**, it clears; between these values it keeps the prior state. Trusted no-echo clears the sonar latch. SensorFault or Stale is unknown and does not, by itself, remove Manual movement authority. The two front IR sensors still block Forward independently, including with unavailable sonar. Known close-obstacle guards and higher-priority safety conditions still apply.

Flame/gas override is an intentional persistent Manual permission. RELEASE/STOP/configuration actions and session loss preserve it, while every hazardous movement still needs a current live session. Explicit disable, AUTO, reboot, stable clear, and non-overridable safety revoke it. It bypasses the local flame/gas motion lock only while its other control conditions hold. Flame/gas evidence remains visible. STOP, dead-man/session rules, critical temperature, known obstacles, and other independent protections remain authoritative. The override never certifies the environment as safe.

## AUTO: ordinary navigation first

`NavigationManager` starts with current front validation. Trusted no-echo or a finite clear front permits normal Forward steps; a finite front reading at or below **20 cm** calls the ordinary obstacle path. Normal handling is roughly:

```text
Obstacle -> STOP -> short bounded reverse -> fresh L/F/R scan
         -> safe-side choice -> ~35-degree relative turn
         -> STOP -> fresh obstacle decision
```

Normal AUTO also arbitrates low front IR evidence and can stop/retry if neither side is safe. The ordinary turn target and obstacle thresholds are separate from boxed-in recovery. No second AUTO press is needed to clear an ordinary recoverable obstacle.

`MotorController` owns PWM and direction primitives. `ImuManager` provides fresh gyro-Z samples for short-term relative turns. The gyro does not give a compass heading, localization, or wheel odometry. Normal obstacle turning retains its existing bounded fallback and timeout; recovery requires a usable relative-turn path and has its own safety bounds.

## Boxed-in recovery: one side, then return to AUTO

After a **trusted complete L/F/R scan**, all three directions must be finite and **strictly below 30 cm** to trigger immediate boxed-in recovery. A trusted `ClearNoEcho` makes that scan not fully boxed. Less obvious oscillation can still be detected by the existing history policy. The initial recovery decision reuses the scan when fresh and complete.

The recovery mission chooses LEFT or RIGHT once and locks it. It does not re-rank the opposite side after every turn:

```text
Choose and lock side -> ~25-degree turn -> STOP -> front-only check
    front >=30 cm or ClearNoEcho -> handoff candidate -> stable forward proof
    front still blocked -> inspect locked side only -> next bounded turn if safe
```

A selected-side finite range of 7 cm or less vetoes the pivot. At most six 25-degree turn attempts belong to the entire episode. Interruptions consume an attempt; the 2500 ms cooldown preserves counts and commitment. Fresh stationary front evidence can find a handoff in HOLD without another turn. Handoff alone does not reset the episode: the existing 5000 ms trusted-forward window must complete without IR/sonar re-block. Every body turn during that situation stays inside episode accounting. A fresh complete scan may replace a side only when the old side is proved invalid and another is safe.

### IR clearance within a recovery mission

Either front IR sensor can stop recovery motion, including an active pivot. IR is a close-obstacle veto, not a direction planner. The interrupted gyro target is discarded, but the locked side remains.

```text
IR active -> STOP -> backward pulse, timed 200 ms
          -> STOP and check IR
          -> another pulse only if still blocked
IR clear -> STOP -> fresh front check -> continue same side only if needed
```

The software caps commanded reverse at **five pulses per episode**; each pulse uses min(current selected motor speed,75%). The robot has no rear sensor, so reverse distance is not verified. STOP, mode change, hazards, and sonar fault preempt backoff.

## Sonar fault: stop, reacquire, decide fresh

AUTO is not operator-supervised in the same way as Manual. `SensorFault` or `Stale` in AUTO/recovery therefore stops motors, cancels the current gyro target and IR backoff, preserves recovery commitment and episode budgets, centers the sensor, and enters `SONAR_REACQUIRE`. `ClearNoEcho` does not enter this state.

During reacquisition the robot stays stopped, takes normal-cadence **front-only** samples, and does not spin, reverse, or perform full directional sweeps. The first new trusted front result keeps it stopped. A **second consecutive fresh trusted front result**, finite or no-echo, restarts a fresh L/F/R scan when EnvironmentalRecovery is incomplete, retains TrappedHold when applicable, otherwise enters a fresh episode checkpoint or normal front decision. Current Robot safety and remaining episode budget determine whether motion is possible. It never replays an interrupted turn.

The **1,000 ms** fast window changes diagnostic status; it is not a one-second blocking wait or a deadline to drive blind. If the fault persists, front sampling and Wi-Fi/control service continue with motors off. Trusted sensing later resumes AUTO automatically without reboot or another AUTO click.

## Environmental hazards and priority

Flame and gas can stop local AUTO motion. After a recoverable hazard remains continuously clear for about two seconds, AUTO always completes a fresh EnvironmentalRecovery L/F/R scan before returning to the existing episode decision or normal operation. A recurring hazard resets that clear wait. Critical temperature and other nonrecoverable conditions retain their separate stop/re-arm authority.

In practical order, explicit STOP and mode changes, critical safety, and required sensor validity outrank navigation commands. This prevents a stale forward, turn, or backoff command from being replayed after an interruption.

## Web dashboard, radio, and traces

`WebDashboard` provides the embedded browser page and HTTP endpoints. SoftAP allows direct local access; STA joins a provisioned network and has bounded reconnect handling. The browser separates `/ping` heartbeat from `/data` telemetry. `DashboardPublisher` updates telemetry without increasing control-loop polling. The Control page keeps operational buttons before diagnostics. The SH1106 OLED reports local status. nRF24 is a secondary experimental heartbeat/status path, not the browser control channel.

An optional RAM-only navigation trace can be enabled for supervised diagnostic capture. It does not write continuous flash logs. Its unchanged format is documented in [NAV_TRACE_SCHEMA.md](docs/NAV_TRACE_SCHEMA.md).

## What validation establishes

The project owner reported a supervised functional physical PASS for final V3.2 behavior on **2026-10-01**. That baseline software evidence is historical; the V3.3 software candidate is verified separately. Earlier dated trials are kept as history in [PIKU_2.1_REV_A_TESTING.md](docs/PIKU_2.1_REV_A_TESTING.md).

Neither the owner report nor host tests provide a latency distribution, turn-angle error distribution, reverse displacement, RF reliability statistic, or industrial safety certification. The no-rear-sensor, partial IR coverage, HC-SR04 no-return ambiguity, and uncalibrated MQ-2 remain evidence boundaries for this research prototype.

## Current watchdog and interruption contract

Recovery retains six started 25-degree turns, five 200 ms IR backoff pulses at min(selected speed,75%), the committed side, fresh front checks, and the 2500 ms cooldown. Handoff is provisional until 5000 ms of trusted forward progress; it does not grant a new episode budget.

The secondary 30000 ms watchdog accumulates active scanning, deciding, turning, and backoff work. Sonar, hazard/clear-wait, integration holds, and retry cooldown exclude waiting time while preserving spent time. Temporary holds preserve the original TrappedHold deadline. Explicit AUTO STOP terminates the episode and latches AUTO until an explicit mode command; ordinary MANUAL STOP/RELEASE preserve eligible Override permission and require fresh movement authority.

EnvironmentalRecovery stays incomplete across temporary sonar or integration loss. After restoration, it restarts all L/F/R evidence; every decision sample is post-restoration. Two trusted front reacquisition samples permit that fresh scan, not Forward. A trapped episode finishes the environmental scan stationary and retains its cooldown; other active episodes return to their existing bounded decision after the full trusted scan. A watchdog exhausted during that scan enters policy HOLD and cannot start another recovery movement.
