# Recovery V3.1 software verification - 2026-10-01

**Status: Historical Recovery V3.1 software gate. V3.2 later changed Manual sonar authority and AUTO fault reacquisition; see [V3.2 software verification](RECOVERY_V32_SOFTWARE_VERIFICATION.md).**

Scope: standalone PIKU 2.1 Rev A repository, continuing Recovery V3. This records software evidence only; physical behavior and response latency were not instrumented or validated.

## Baseline

- Branch: `codex/recovery-v3-locked-side-20261001`
- HEAD: `af4bf0fac27448617e6c4fb76a65d6f9c022c075`
- No substantive tracked changes. Four pre-existing untracked diagram PNGs were preserved.
- Unmodified `pio run -e esp32-s3-n16r8`: PASS; RAM 58,716 / 327,680 bytes; flash 1,036,681 / 6,553,600 bytes.
- All eight existing native C++ tests and both Python tests: PASS.

## Focused change

A complete trusted scan is immediately boxed-in only when all three readings are finite valid and strictly below 30 cm. Any trusted ClearNoEcho or invalid reading excludes the scan from this geometric classification. The completed scan is reused for the initial recovery side decision. Normal AUTO already recognizes trusted ClearNoEcho as open for front driving and side ranking; its implementation is unchanged.

During a locked-side recovery mission, either debounced IR sensor stops motion and starts a fixed-state reverse policy. Each commanded pulse uses MotorController's current selected speed and lasts at most 200 ms, with a motor stop and IR check between pulses. IR clearance stops reverse immediately, retains the selected side, and starts a front-only check. If front is still blocked, only the selected side is checked before a new 25-degree turn. An interrupted gyro target is discarded. A selected-side sonar value at or below 7 cm uses one forced bounded reverse pulse before front revalidation. The reverse budget is five pulses for a mission, which also bounds repeated close-sonar events without a rear sensor. Persistent obstruction or exhausted budget stops motors and enters the existing 2,500 ms failed-mission cooldown.

A trusted front reading at least 30 cm or ClearNoEcho ends recovery and hands control to normal AUTO without a recovery forward trial or full scan. STOP, mode change, and critical hazard paths cancel the backoff state. Normal AUTO's 35-degree turn, thresholds, IR arbitration, communications, Manual override, and dashboard source remain unchanged.

## Software gates

- Final `pio run -e esp32-s3-n16r8`: PASS; RAM 58,724 / 327,680 bytes; flash 1,037,277 / 6,553,600 bytes.
- Nine native C++ tests compiled with `-std=c++11 -Wall -Wextra -Werror`: PASS.
- Both Python tests: PASS.
- Embedded dashboard JavaScript parsed in isolated headless Edge: PASS.
- `git diff --check`: PASS.
- No push, merge, flash, or physical test was performed.

## Remaining supervised tests

Check actual IR debounce and stopping latency, selected-speed 200 ms reverse travel and rear clearance, front and selected-side sonar readings after motion, interrupted pivot angle, no-echo behavior in the environment, STOP/Manual/hazard preemption under motion, and boxed-in retries. The software checks above are qualitative, not instrumented latency or collision measurements.
