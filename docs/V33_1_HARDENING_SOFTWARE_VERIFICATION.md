# V3.3.1 focused recovery-speed hardening - 2026-10-04

**Physical validation NOT RUN. RF PA/range engineering NOT RUN.**

Starting branch `codex/reva-v33-deterministic-recovery-20261003`, HEAD `304e7b7c7d9196ed5658ed2645295b8bcca8137e`; only the four owner PNGs under figures were untracked. Candidate branch `codex/reva-v331-hardening-20261004`. Historical [V3.3 verification](RECOVERY_V33_SOFTWARE_VERIFICATION.md) is preserved.

## Exact output contract

`RobotConfig::RECOVERY_BACKOFF_MAX_SPEED_PERCENT=75`. Both navigation recovery reverse actions call `backwardWithSpeedCap(75)`. The motor retains its selected preference and applies `min(selected,75)` through the original calibrated duty formula, including the minimum-drive offset. The cap persists on backward output refresh or a selected-speed edit during the pulse; normal `backward()` explicitly selects an uncapped 100 maximum, and forward/turn use the unchanged selected preference. No asynchronous restore, slider change, extra delay, episode state machine, pulse-duration or pulse-budget change was added.

Actual production MotorController tests compare output to normal calibrated 0/40/60/75/80/100 choices, prove 100 caps to normal 75 duty rather than raw 75% PWM, preserve the preference through refresh and repeated pulses, and restore normal Backward/forward/turn output. Actual NavigationManager tests exercise the two capped call sites; existing tests retain six turns, five pulses and 200 ms policy behavior. Intermediate active-pulse policy `None` does not issue a reverse refresh; motor refresh safety is tested separately.

## Software verification

Baseline: 11 policy executables, 3 retained Python scripts, 4 unittest cases, embedded JS parse, 66 browser control and 44 page/viewport cases, build/diff PASS. Final: 11 policy executables + 3 retained Python scripts + 5 unittest cases (including actual motor and navigation), embedded JS, current browser 66 control/44 page scenarios, build and whitespace PASS. RAM 58,892 -> 58,892 (+0); flash 1,040,581 -> 1,040,653 (+72) bytes.

## Owner physical addendum - prepare only

- [ ] Selected 40%: recovery reverse remains 40% on the calibrated scale.
- [ ] Selected 75%: recovery reverse remains 75%.
- [ ] Selected 100%: recovery reverse caps at 75%.
- [ ] During/after backoff the selected user speed still reports 100%; normal Manual Backward remains at selected speed.
- [ ] Maximum five approximately 200 ms pulses remains unchanged.

Owner approval of the maximum is an input to this mission, not physical verification of this candidate. Stop for owner review; no flash, upload, push, merge, tag or release.
