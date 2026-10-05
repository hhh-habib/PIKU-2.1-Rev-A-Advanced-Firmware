# Narrow collision closure software verification — 2026-10-05

Owner mission: PIKU / CPS Collision Closure V3. This report records software evidence only. Physical validation and RF PA/range engineering NOT RUN. Robot remains the movement/navigation/safety authority; operator commands travel through the chosen Control Link. Nothing pushed, merged, tagged, released, flashed or uploaded.

## Exact starting provenance

| Repository | Initial branch | Exact initial HEAD | Initial status |
| --- | --- | --- | --- |
| Standalone | codex/reva-state-collision-20261005 | ce0dbe7546c73353dfd1ba44ca5425d29ab8548b | Four owner PNGs untracked; tracked files clean |
| Stage 4 | codex/stage4-state-collision-20261005 | 7a09d996c9b8586a8854fa613d07a45c120359fd | Clean |
| Coordinator | codex/nodev1-state-collision-20261005 | 9b070857e9b7b329b7e19f6c69e12b803d73997e | Clean |

The branches/HEAD/status and protected hashes were checked before edits. Local correction branches are `codex/reva-collision-closure-20261005`, `codex/stage4-collision-closure-20261005`, and `codex/nodev1-collision-closure-20261005`. This report's containing commit is the local correction commit in the relevant changed repository; final exact commit SHAs are in the owner handoff and Research Continuity receipt. Coordinator remains at the exact starting HEAD with no file changes or manufactured commit.

Owner PNGs: `figures/Circuit_Diagram.png`, `figures/PIKU_2.1_Rev_A_Software_Architecture.png`, `figures/PIKU_2.1_Rev_A_System_Architecture.png`, `figures/PIKU_2.1_Rev_A_Technical_Blueprint_Black.png`. All remain untracked and byte-identical.

## Reproduction and root causes

All six findings were REPRODUCED before production correction. [Recorded runtime failures](COLLISION_CLOSURE_REPRODUCTION.json) preserve excerpts and log hashes; complete original logs remain in the local temporary evidence directory listed below.

| Finding | Reproduction | Root cause | Narrow correction |
| --- | --- | --- | --- |
| F1 | Stage4: current heartbeat then first low-uptime movement command; inverse current command then first low heartbeat | Unseen per-stream tracker admitted first packet without consulting accepted peer epoch | Wrap-safe peer uptime fence at first stream use; independent sequence spaces; coherent candidate confirmation/current-traffic cancellation |
| F2 | Stage4 actual RF batch: RELEASE→SPEED; Override-OFF→FORWARD | One pending command slot overwrote earlier accepted transition before handler visibility | Four-entry ordered bounded queue; backpressure before acceptance; source-qualified revocation; preserved original receive age; discard old movement on STOP/loss/reboot |
| F3 | Both Robot production callback + outer dispatch + NavigationManager reached environmental decision after accepted RELEASE | Early critical-stop environmental dispatch ran before the later controlStopThisPass fence | Add early fence after stationary active-hazard bookkeeping and before motion-capable environmental resume |
| SC-01 | Persistent left/right IR, post-turn and active-episode overlays reached false environmental completion | Completion branches did not consistently enforce current directional IR and post-turn trusted front proof | Environmental IR STOP/invalidation/full rescan; all completion overlays fenced; post-turn requested fresh front proof |
| SC-02 | Stage4 real flame classify/clear wholly within suppressed nominal integration hold, then restore | Outer hold skipped NavigationManager, losing transient local environmental context | Local context-only observation during hold; restored full clear wait and fresh environmental scan; retain STOP/mode/temperature causes |
| F4 | Both Robot class histories exhausted active-work watchdog then cooldown never reached clear-path provisional handoff | Every fresh reassessment redispatched already spent watchdog before it could complete; terminal hold effectively blocked autonomous continuation | Explicit exhausted state; stationary reassessment; retained provisional proof; only fresh blocked L/F/R can open a distinct bounded episode |

No correction was made for owner-accepted R1. No universal replay claim is made.

## Executed regressions and source audit

[Matrix guide and concrete enum inventory](COLLISION_CLOSURE_MATRIX_GUIDE.md) links the owner-filterable CSV. SOURCE/CONDITIONAL rows explicitly do not claim their entire history executed. NATIVE/BROWSER/BUILD rows are separately graded; N/A retains impossible or irrelevant combinations and explains why. Physical rows are PHYSICAL-PENDING. Counts describe evidence rows, not tests or exhaustive runtime coverage.

New Robot navigation histories execute current NavigationManager/policy bodies with hardware doubles: persistent/new left/right/both IR across full scan/decision and next dispatch; environmental turn/post-turn requested-front proof; EscapeScan/EscapeVerify/ForwardTrial/TrappedHold/exhausted overlays; IR→sonar→restore; stale scan veto; watchdog exact/over threshold STOP; frozen turn/pulse/work ledger through provisional 5000 ms proof; IR/sonar/hazard/obstruction interruption; three repeated distinct bounded episode renewals; full six-turn/five-pulse exhausted ledger; uint32 wrap; ordinary turn-limit no-renewal; STOP/mode re-arm. Stage4 adds integration pause and environmental→integration→IR→restore full scan.

Both outer-dispatch regressions cover environmental decision, turn and post-turn against RELEASE/STOP/mode. Stage4 executes real service, accepted RF RadioManager, real SafetyManager, callbacks, outer loop and NavigationManager together. Its 18-point Web/RF histories also prove no new turn/reverse after a pass winner; a separate active-episode environmental overlay is tested. Standalone executes actual Web callback, outer dispatcher and NavigationManager with a labeled service-delivery seam; it does not claim real sockets or the complete service body.

Stage4 SC-02 uses unchanged real 80 ms flame detect and 200 ms digital clear debounce, which fit inside the 2000 ms integration hold. After restore, the unchanged full environmental clear wait and fresh requested L/F/R complete before AUTO Forward. Real gas detect 600 ms / clear 2000 ms demonstrates gas can remain critical at nominal restoration; a separate explicitly classified `_gasCritical` seam tests transient remembered context without claiming gas timing reachability. STOP/Web+RF, mode, simultaneous temperature, sonar fault/restore and restored fresh environmental sensing retain precedence.

Stage4 RF histories execute real packet intake and handler-visible ordered effects: both cross-stream first-use inverses; duplicates/reorder/wrap; candidate cancellation by accepted current traffic; accepted revocations through capacity pressure; STOP preserving OFF/RELEASE while dropping directions; RELEASE/SPEED and OFF/FORWARD; deferred expiry/competing owner exclusion; reboot dropping pending movement while retaining accepted STOP/RELEASE/OFF; candidate→STOP→stale direction. Full queue intake pauses before progression; accepted entries cannot be overwritten. A MANUAL outer loop can service twice, each bounded to four entries. Hardware FIFO drops and delayed network-send age are unmeasured; original age means Robot reception time, never a fabricated fresh dispatch time.

Existing nested Recovery→hazard→sonar, TrappedHold→hazard→integration, candidate/STOP ordering, Web/RF owner exclusion, manual reverse, permission retention, STOP/mode/expiry same-pass fences, gateway freshness/reboot/expiry, telemetry-source independence and browser control races remain exercised by the full suites. No copied mock algorithm replaces NavigationManager, RadioManager, SafetyManager or the tested dispatcher bodies.

## Gates and resource deltas

| Repository | Before production edits | Final complete suite | Build RAM bytes before→after | Flash bytes before→after |
| --- | --- | --- | --- | --- |
| Standalone | 37 unittest tests, 3 expected CPS skips; browser; build; diff check | 67 unittest tests: 63 passed, 4 expected CPS skips; 12 policy executables; embedded JS parse | 58908→58908 (0) | 1041545→1042517 (+972) |
| Stage4 | 115 unittest tests, 3 expected gateway-only skips; browser; build; diff check | 175 unittest tests: 172 passed, 3 expected gateway-only skips | 54492→54580 (+88) | 1046345→1048361 (+2016) |
| Coordinator | 78 unittest tests, no skips; browser; build; diff check | 78 passed, no skips | 87184→87184 (0) | 1032133→1032133 (0) |

Sequence exception: the additional 12 Standalone policy executables were omitted from the initial pre-edit discovery invocation. All twelve were subsequently replayed successfully from the exact immutable starting Git objects (without Secrets or production replacement), and pass on final code with `-Wall -Wextra -Werror`. That is a supplemental exact-baseline replay after reproduction, not a pre-edit execution. Initial current-HEAD unittest/browser/build gates and all defect reproductions did run before production edits. No before/after claim hides this ordering deviation.

Standalone skips: three historical CPS integration-only histories and the new exhausted integration-pause history. Stage4 skips: three Coordinator-gateway-only tests. Coordinator has none. No required final software gate unavailable. Browser suites were run serially: each Robot checks 66 control and 44 page observations per frozen/baseline/current asset variant at 360/375/390/412/430/1180 widths; Coordinator checks 72 viewport and 6 resilience cases, plus existing client behavior groups. These observations are included within unittest methods, not extra unittest counts.

PlatformIO builds PASS; inspected captured baseline/final build logs contain no compiler warning lines. Native policy compilation treats warnings as errors. Native MSVC harness output is captured by existing tests; a separate current production compile probe passed and recorded one MSVC C4127 constant-expression warning at the unchanged `if (!RobotConfig::ENABLE_STUCK_RECOVERY_V2 && stuckEvidenceSatisfied())` branch, also present verbatim in the exact starting object; no warning suppression or production rewrite was added. Git LF→CRLF normalization notices are distinct from compiler diagnostics. `git diff --check` and exact reviewed production inventories must pass before staging.

Final commands: Standalone `python -B tests/run_host_checks.py`; Stage4/Coordinator `python -B -m unittest discover -s tests -p test_*.py -v`; each repository `pio run` and `git diff --check`. Parity/protocol/browser/source checks are part of those suites. Logs are retained locally in `C:/Users/User/AppData/Local/Temp/piku-collision-closure-20261005` (baseline/final tests, builds, exact policy replay, reproduction and targeted nested histories). Durable reproduction/protection summaries and matrix are committed here.

## Parity, protected scope and residual boundaries

Standalone→Stage4 shared core parity PASS. Shared pure-core files and constants are fingerprinted; NavigationManager permits only enumerated existing CPS seams plus the exact new suppressed-hazard declaration/body. Stage4↔Coordinator protocol compatibility PASS: schema/version unchanged, Command 12 bytes, Status 30 bytes, command enum values unchanged. `RadioTypes.h` and `RadioStatus.h` have no diff; required shared Status definitions remain identical. Coordinator needs no Robot intake counterpart or navigation logic.

No change to RobotConfig/PinConfig/WiFiConfig/Secrets/platformio, PA/channel/addresses, sensor classification/timing, sonar/IMU/servo/motor implementations, speed/turn/pulse constants, dashboard assets/backend API/telemetry/networking, Coordinator code/current docs, owner PNGs or Node reference. [Protection comparison](COLLISION_CLOSURE_PROTECTION.json) records 11 Standalone, 7 Stage4, 4 Coordinator and 44 reference file hashes compared with zero mismatches, without publishing secret hashes. Read-only Node stays `main` at `70ef949ce18ab81cbc207fed13bcc320d8b99964` with exactly its original modified `.vscode/extensions.json` and untracked `AGENTS.md`. Historical report bodies/releases/evidence are preserved; the prior collision report has only a historical/superseded pointer prepended.

Historical source inventories remain checked against immutable old objects. New strict raw-source production hashes enumerate only the approved closure layer; historical projections apply that layer before older projections. Runtime fixtures execute raw current production code. Existing exact-string assertions were updated only for the new queue STOP-discard call and exhausted-watchdog conditions; full behavior assertions remain. The old fixture's RadioManager queue fields and renamed-main return were adapted, not production logic weakened.

Universal recorded-packet replay resistance is NOT proven without a boot/session identity. Two plausible historically recorded progressing packets can still look like a quick reboot. R1 remains owner-accepted. Late first reboot observation beyond 10000 ms and 16-bit half-range sequence gaps retain conservative rejection; no stale resync or authenticated replay protection added.

## Actual F4 boundary and proof

[Field-level reset/preserve ledger](COLLISION_CLOSURE_F4_LEDGER.md) was established before production correction. Expiry cancels motor/gyro/pulse output immediately; freezes old accumulated work, episode start, committed-side history, six-turn and five-pulse resources. Existing cooldown does not refill them. Fresh clear front can enter provisional ordinary AUTO with those resources retained; interruption zeros safe-forward proof and returns to stopped fresh sensing, never exhausted old movement. Existing sustained 5000 ms safe-forward evidence alone completes that episode and clears resources.

Continued blockage requires complete newly requested, post-expiry, trusted L/F/R and current safety/IR/IMU/authorization gates. Only `renewAfterWatchdog` at that explicit boundary clears old avoidance/commit/turn/pulse/work and pending output state, starts a new timestamped episode, and uses the unchanged decision algorithm. Every episode has 30000 ms active work, six turns and five pulses; repeated episodes do not imply a lifetime bound. Ordinary turn/pulse/invalid-sensor hold causes do not acquire this exception. STOP/temperature/sonar/environment/integration gates continue to dominate.

## Physical boundary and handoff

The software closures are evidenced above, with source-only combinations and the supplemental baseline-order deviation disclosed. Physical stop latency, RF delivery/range, turn accuracy, sensor reliability, electrical safety and shared-SPI timing were not measured. Physical validation NOT RUN. RF PA/range engineering NOT RUN. No packet schema change. Nothing pushed/merged/tagged/released/flashed/uploaded. Stop for owner review of the local commits.

## Exact changed files in this repository

- `README.md`
- `SOFTWARE_EXPLANATION.md`
- `docs/COLLISION_CLOSURE_F4_LEDGER.md`
- `docs/COLLISION_CLOSURE_MATRIX_GUIDE.md`
- `docs/COLLISION_CLOSURE_PROTECTION.json`
- `docs/COLLISION_CLOSURE_REPRODUCTION.json`
- `docs/COLLISION_CLOSURE_SOFTWARE_VERIFICATION.md`
- `docs/COLLISION_CLOSURE_STATE_EVENT_MATRIX.csv`
- `docs/SYSTEM_STATE_COLLISION_VERIFICATION.md`
- `include/NavigationManager.h`
- `include/StuckRecoveryV2Policy.h`
- `src/NavigationManager.cpp`
- `src/main.cpp`
- `tests/collision_closure_reviewed_delta.json`
- `tests/native_collision_closure.cpp`
- `tests/native_state_collisions.cpp`
- `tests/test_collision_closure.py`
- `tests/test_collision_closure_dispatch.py`
- `tests/test_collision_closure_inventory.py`
- `tests/test_recovery_source_contract.py`
- `tests/test_state_collisions.py`
