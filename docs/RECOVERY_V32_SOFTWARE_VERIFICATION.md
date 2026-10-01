# Recovery V3.2 software verification ? 2026-10-01

Scope: standalone PIKU 2.1 Rev A firmware on the locked-recovery branch. This is a software review and build record, not physical validation or an instrumented latency measurement.

## Baseline

- Branch: `codex/recovery-v3-locked-side-20261001`
- HEAD: `a8d2753601a5b7650409f534a41e3a2130a2193f`
- Tracked tree clean; four pre-existing untracked diagram PNGs preserved.
- Unmodified build: PASS; RAM 58,724 / 327,680 bytes; flash 1,037,277 / 6,553,600 bytes.
- All nine existing native tests and two Python tests: PASS.

## Findings and correction

Manual Forward was inhibited by sonar fault/stale through the Manual gate, the override enable/tick path, the ultrasonic warning in SafetyManager's Manual forward guard, and the Manual front latch. That latch also used 25/30 cm thresholds and only latched a close finite sample while already moving. Manual now uses fresh finite front evidence with 20 cm stop and 25 cm clear hysteresis. ClearNoEcho and unknown sonar clear only the ultrasonic latch. The independent front IR guard, STOP/deadman, control-session, temperature, and flame/gas override gates remain.

AUTO's previous invalid-stop/trapped-hold paths could impose a retry/cooldown or preserve recovery context after a transient sonar fault. The new SONAR_REACQUIRE phase stops motors, cancels gyro and IR backoff state, centers the head, and observes normal-cadence FRONT samples. Two consecutive new trusted front results, including ClearNoEcho, immediately enter the existing normal front decision. The 1,000 ms window changes diagnostic status only; a persistent fault remains stopped and sampling until recovery. No full scan, spin, reverse, or blocking wait runs during this phase.

The dashboard shows VALID/numeric, OPEN/OPEN, FAULT, or STALE for ultrasonic state. Normal 35-degree navigation, locked 25-degree recovery, boxed-in detection, 30 cm recovery handoff, 7 cm veto, 200 ms/five-pulse IR backoff, seven-turn limit, communications, and Control page layout are otherwise unchanged.

## Final software gates

- Firmware build: PASS; RAM 58,740 / 327,680 bytes; flash 1,038,217 / 6,553,600 bytes.
- Ten native C++ tests compiled with `-std=c++11 -Wall -Wextra -Werror`: PASS.
- Three Python tests: PASS.
- Embedded dashboard JavaScript parsed in isolated headless Edge: PASS.
- `git diff --check`: PASS.
- No push, merge, hardware flash, or physical test was performed.

## Supervised physical follow-up

Verify IR stopping distance in Manual mode, held-command/deadman behavior with disconnected HC-SR04, actual 20/25 cm hysteresis, two-sample reacquisition latency and motor-off behavior, no stale pivot replay, no-echo interpretation in the real environment, and hazard/STOP/Manual preemption while moving. Source and host tests do not measure these physical latencies or clearances.
