# Recovery V3 software verification - 2026-10-01

**Status: Historical Recovery V3 software gate. V3.1 and V3.2 changed later behavior; see [V3.2 software verification](RECOVERY_V32_SOFTWARE_VERIFICATION.md) and the [current README](../README.md).**

Scope: standalone PIKU 2.1 Rev A repository only. This record covers source, host-test, dashboard parse, and firmware-build evidence. It does not claim physical validation, calibrated clearance, or measured control latency.

## Baseline

- Branch: `master`
- Commit: `845de015e59c0d07812c1a9931d4c7cfbcb87538`
- Tracked working tree: clean. Four untracked diagram PNGs were present and preserved.
- `pio run -e esp32-s3-n16r8`: PASS; RAM 58,724 / 327,680 bytes; flash 1,037,013 / 6,553,600 bytes.
- Eight existing native C++ tests (`-std=c++11 -Wall -Wextra -Werror`) and the Python trace test: PASS.

## Changed behavior

A completed normal L/F/R scan can trigger immediate recovery when front is blocked and both finite sides are below 30 cm. Subtle stuck evidence continues to use the existing history detector. The initial recovery decision reuses that scan when trusted. The chosen side stays locked for a mission. Each gyro-guided 25-degree increment ends with a front-only sample. A finite front reading of at least 30 cm or ClearNoEcho hands control back to normal AUTO immediately. If front is blocked, recovery samples only the selected side. Unsafe side sonar or selected-side IR receives one bounded same-side recheck before motor-off cooldown. A mission permits at most seven increments; failure waits 2,500 ms, then starts a new full scan and mission.

The previous recovery-only 50 cm requirement, repeated post-turn full scans, side alternation, browser-link retry gate, and forward trial were removed. Normal AUTO's 35-degree target and obstacle thresholds were retained. The dashboard Control page places alarm and movement/servo controls before readouts and radar.

## Final software gates

- `pio run -e esp32-s3-n16r8`: PASS; RAM 58,716 / 327,680 bytes; flash 1,036,681 / 6,553,600 bytes.
- Eight native C++ tests, Python trace test, and focused recovery/UI source-contract test: PASS.
- Embedded dashboard JavaScript parsed in isolated headless Edge with `new Function(source)`: PASS.
- `git diff --check`: PASS.
- No hardware flash or physical test was performed.

## Remaining supervised tests

Verify actual 25-degree rotation, pivot sweep and rear clearance, selected-side IR interruption, ultrasonic no-echo interpretation in the environment, cooldown/retry in a box, and STOP/Manual/hazard preemption under motion. Responsiveness has not been instrumented as a latency distribution.
