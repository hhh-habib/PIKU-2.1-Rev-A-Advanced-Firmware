## V3.3.1 software candidate - 2026-10-04

- Cap RecoveryIrBackoff reverse output at 75 on the calibrated user-speed scale. Preserve selected preference, Manual Backward, forward/turn semantics, five-pulse budget and 200 ms policy.
- Actual production motor/navigation software regressions added. Physical validation NOT RUN.

# Release history

## V3.3 software candidate - 2026-10-03

- Persistent six-attempt recovery episode and five bounded IR pulses across cooldown, sonar and hazard pauses.
- Trusted 30 cm/OPEN handoff retains budgets until the existing 5000 ms forward-validation window completes.
- Two new trusted sonar samples resume a fresh checkpoint without replaying motor commands.
- Override survives ordinary RELEASE/STOP/configuration and session loss; live authorization and safety gates still govern movement.
- Audited gyro/scan cleanup, rich episode diagnostics, pure-policy and production-navigation regressions.
- Physical validation pending. No release, hardware flash or RF PA/range engineering.


## V3.2 - 2026-10-01

Frozen executable source: `41281d8325d88c70e074e008a2550a1164102132`.

- Finalized locked-side boxed recovery with 25-degree increments and immediate 30 cm/open front handoff.
- Bounded IR recovery backoff at 200 ms per pulse, up to five pulses per mission.
- Applied `ClearNoEcho` = operational OPEN consistently across navigation, recovery, Manual, reacquisition, and dashboard.
- Set Manual Forward finite-distance hysteresis to 20 cm stop / 25 cm clear while preserving operator authority on sonar fault/stale.
- Added motor-off, front-only autonomous sonar reacquisition and automatic fresh AUTO resume.
- Presented ultrasonic dashboard states as VALID, OPEN, FAULT, and STALE.
- The project owner reported final supervised functional physical validation PASS on 2026-10-01; see [physical validation](docs/PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md).

## V3.1 - 2026-10-01

Added IR-triggered bounded clearance during locked recovery and made the all-finite L/F/R below-30-cm boxed classification exact. [Software record](docs/RECOVERY_V31_SOFTWARE_VERIFICATION.md).

## V3 - 2026-10-01

Simplified stuck recovery to a locked side, bounded 25-degree turns, front-only success check, and prompt normal AUTO handoff. Reordered Control operations before diagnostics. [Software record](docs/RECOVERY_V3_SOFTWARE_VERIFICATION.md).

## Earlier Rev A development

Stage-1, Stage-2, V2, communication, and physical-test records remain in `docs/` as dated engineering history. The earlier `v2.1-revA-final` tag identifies a prior standalone freeze, not the V3.2 executable.
