# Collision closure state and evidence inventory

This matrix is a source audit grid plus separately graded executed histories. A SOURCE/CONDITIONAL cell is not an executed full-history PASS. N/A cells retain the combination and give the structural reason. Filter the CSV by repository, state, event and evidence kind. Background applicability is explicit in every row; physical scheduling/latency remains pending.

[Owner-reviewable state/event matrix](COLLISION_CLOSURE_STATE_EVENT_MATRIX.csv) | [Field-level F4 boundary ledger](COLLISION_CLOSURE_F4_LEDGER.md)

## Current concrete enums

- NavigationState (27): Idle, FrontSettle, FrontSample, Forward, ObstacleReverse, ScanLeft, ScanFront, ScanRight, Recenter, SpinLeft, SpinRight, InvalidStop, SonarReacquire, ObstacleRecovery, ObstacleScan, ObstacleSpin, ObstacleReevaluate, BlockedRetryWait, HazardActive, HazardClearWait, HazardRecoveryScan, StuckSuspected, StuckRecovery, RecoveryIrBackoff, OperatorAbort, CriticalAbort, TrappedHold.
- AutoPhase (24): Idle, FrontSettle, FrontSampleWait, Forward, ForwardSettle, ReversePrepare, Reverse, ReverseSettle, ScanServoSettle, ScanSampleGap, ScanSampleWait, RecenterSettle, Turn, PostTurnSettle, HazardActive, HazardClearWait, InvalidStop, SonarReacquire, OperatorStop, BlockedRetryWait, RecoveryFrontSettle, RecoveryFrontWait, RecoveryIrBackoff, TrappedHold.
- ScanPurpose (7): Obstacle, IrSingle, IrBoth, EnvironmentalRecovery, StuckRecovery, EscapeV2, RecoverySide.
- StuckV2State (7): Normal, Suspected, EscapeScan, EscapeTurn, EscapeVerify, ForwardTrial, TrappedHold.

NavigationState is externally reported state; AutoPhase is actual dispatcher phase; ScanPurpose is a sensing context, and StuckV2State is retained policy history. Their rows are not claims of independent reachable phase combinations. The grid includes all 24 named major contexts, every concrete enum member, all 22 named events, Standalone Web, Stage4 Web/RF, and ten gateway contexts on two chosen links. Telemetry source change is represented independently from Control Link change.

## Evidence totals

- BROWSER / PASS: 3 rows.
- BUILD / PASS: 3 rows.
- NATIVE / N/A: 1 rows.
- NATIVE / PASS: 93 rows.
- PHYSICAL / PHYSICAL-PENDING: 6 rows.
- SOURCE / CONDITIONAL: 4870 rows.
- SOURCE / N/A: 1444 rows.
- SOURCE / PASS: 2 rows.

Counts describe rows, not tests or a runtime Cartesian product. Runtime methods with multiple iterations establish only those asserted histories. Source cells trace the actual current outer dispatch, class and gateway paths; unexecuted combinations stay CONDITIONAL. Required nested sequences have dedicated named native rows/tests, including restoration through full scans, authority precedence, exhausted handoff interruption and distinct repeated episode renewal.

## Environmental completion callsites

1. `updateFrontValidation`: post-turn completion only after the newly requested, fresh, oriented, trusted clear front proof and current clear IR; unresolved front blockage starts a fresh environmental scan.
2. `selectPathAfterScan`: active-episode overlay only after complete trusted fresh L/F/R and the top-level environmental IR veto; preserved TrappedHold/exhausted policy is stationary.
3. `selectPathAfterScan`: ordinary front choice only after complete trusted L/F/R and current clear IR.
There is no direct `PostTurnSettle` completion. The top-level environmental veto applies to EscapeScan, EscapeVerify, ForwardTrial entry, TrappedHold and watchdog-exhausted overlays, as well as ordinary scan/turn/post-turn. Persistent IR invalidates all L/F/R, retains incomplete context and prohibits ordinary reverse.

## Bounds and limits

The four-entry Stage4 queue processes at most four RX packets per poll and four handler entries per service pass. A MANUAL outer loop may service twice; each pass remains bounded. Full-queue intake stops before packet read/progression acceptance, preserving already accepted revocations. Unread packets retain no original network-send timestamp; reception age begins when received, and is never refreshed by deferred dispatch. Hardware FIFO drops/delivery during overload are unmeasured.

Universal recorded-packet replay resistance is NOT proven without a boot/session identity. Late first reboot evidence beyond 10000 ms and 16-bit half-range gaps retain conservative rejection. Two plausible historical packets remain owner-accepted R1; no schema change or stale resync was added.

Physical validation and RF PA/range engineering NOT RUN.
