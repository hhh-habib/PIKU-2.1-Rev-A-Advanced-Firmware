# Stage 5D core audit (software evidence)

Compared Standalone `93a44eff1445fb675fdf737645988436b1883cbc` with Stage 4 `d3db4b6b59622b18a34297c24d5c7f66ce8f7724` before changes.

| Class | Differences | Decision |
| --- | --- | --- |
| Robot core | STOP/mode cancellation in Stage 4 clears gyro flag, relative angle, turn start and integration sample. Sonar acquisition clears request/accumulator/direction and L/F/R cache. | Backport both cleanups. |
| Integration | Navigation motion-authorization callback and post-sonar authorization; Web/Radio owner-bound override; identity, coordinator registration/loss policy, RF retry count 3 rather than 8; shared API/control leases. | Keep wrappers in Stage 4. Replace owner-bound permission only with the authorized V3.3 model after authority verification. |
| Diagnostics | Stage 4 uses EscapeCooldown and compact stuck/recovery reasons; Standalone exposes v2 state/attempt/reason/assistance. | Synchronize truthful V3.3 episode diagnostics through existing telemetry. |
| Documentation | Different standalone release and CPS integration histories. | Preserve history; update current candidate descriptions separately. |

The pure recovery/IR/sonar/escape/ManualMotionGate policies, MotorController, SafetyManager, SensorManager, ImuManager and ServoScanner have identical semantic content. RobotConfig shared recovery/safety values match; communication/identity-only constants differ. ManualOverridePolicy differs in API and owner binding, not additional obstacle protection.

Source confirms cooldown restarts clear turn count and backoff budget, sonar acquisition aborts recovery, and a clear front handoff immediately resets budgets in both baselines. These are software mechanisms consistent with the owner's observation; they do not prove an RF or physical root cause.

V3.3 will reuse the existing 5000 ms AUTO_OBSTACLE_RETRY_RESET_FORWARD_MS evidence window with a trusted handoff and continued fresh forward-safe sensing. Physical validation is pending.
