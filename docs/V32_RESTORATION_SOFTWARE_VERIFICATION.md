# V3.2 standalone AUTO and recovery restoration: source and software verification

Date: 2026-10-08 (Asia/Dhaka). This report supersedes the historical preflight stop and earlier V4.5/V4.6 navigation candidate reports. Results concern the existing standalone working tree. No robot was flashed or moved.

## A. Overall result

**COMPLETE at the source and modeled software level; physical validation is pending.** Six authorized production files now implement the reference ordinary AUTO and stuck/boxed-in recovery algorithms with preserved current control, sensing and environmental infrastructure. Both required policy approvals preceded dependent implementation. No unresolved software blocker or failing final regression remains.

Development directory: `E:\Development\Research\Meo\The PIKU\PIKU 2.1 Rev A\PIKU 2.1 Rev A Software Development`.

Branch: `codex/reva-collision-closure-20261005`. HEAD: `cbbf3599d0a6192976f40d991250cd38a02c7abb`. Both are unchanged. The initial shell was in CPS Node Stage 4; development and test outputs stayed in the standalone project. Stage 4, Node V1 and Coordinator source were not modified.

Read-only reference: `C:\Users\User\Downloads\PIKU-2.1-Rev-A-earlier github code`. Its 61 non-secret production/test/configuration files match frozen executable commit `41281d8325d88c70e074e008a2550a1164102132` after CRLF normalization. All 90 initially inventoried reference files remain byte unchanged. Prior V3.2 physical success is owner evidence, not physical evidence for this build.

Preflight found 28 modified tracked files, untracked V4.5/V4.6 fixtures/reports, four owner PNG figures and existing caches. Exact initial status and 147 baseline file hashes are in [the baseline](V32_RESTORATION_PREFLIGHT_BASELINE.json). Production callers and uncommitted changes were reviewed before editing. Identified navigation policies conflicting with the contract were deliberately replaced; unrelated existing work was preserved. The task changed six production files, 23 existing test/support files and an additive README note. Of 147 baseline files, 117 are byte unchanged. All prior README content after the new prefix is byte unchanged. The [source audit](V32_RESTORATION_SOURCE_AUDIT.json) reports zero unexpected/protected/reference changes.

The target's requested parent README was absent. Available target and required workstation/continuity guidance were read; target identity was established independently through Git, source, configuration and reference provenance.

Owner decisions applied:

1. **Restore V3.2 hazard abort and mission reset.** Hazard/critical arbitration stops the motor, aborts the stuck mission and resets committed side/turn and recovery pulse state. Reference `abort()` retains the attempts diagnostic; it does not behave like `rearm()`.
2. **Preserve V4.6 environmental-job ownership and fresh environmental scan across sonar reacquisition.** An unfinished environmental job remains owned and cannot be falsely marked complete when sonar fails. This is the explicit approved parity exception.

## B. Files modified

Paths are relative to the exact development directory in A. Existing changes were inspected. Approved restoration supersedes conflicting navigation deltas, while retaining identified unrelated content/interfaces. NavigationManager was modified by coherent function/branch changes, not whole-file replacement from the reference or HEAD.

### Production and current documentation

| Exact path | Task change and restored behavior | Preserved functionality |
|---|---|---|
| `src/NavigationManager.cpp` | Restore ordinary AUTO, full scan, history admission, committed recovery, seven-turn mission, bounded IR pulses and retry; integrate both approvals. Remove normal checkpoint/saved-turn/ledger paths. | Current STOP/cancellation strength, request freshness/wrap guards, environmental job/IR/front-proof handling, public services and dashboard diagnostics. |
| `include/NavigationManager.h` | Restore reference phases/purposes/state members; remove obsolete NORMAL checkpoint and BOXED continuation/mask bookkeeping. | Constructor/public control/environment interfaces, main's `observeIrEpisodes()` interface and current request metadata. |
| `include/RobotConfig.h` | Restore seven started turn slots and original mission/cooldown values in the starting working tree. | Unrelated hardware, sensing, network and telemetry configuration. HEAD-relative diff shows only turn count because other starting values were already uncommitted. |
| `include/StuckRecoveryV2Policy.h` | Restore active decision history and per-mission state/reset/cooldown. Remove direction/active-time/lifetime ledger and ForwardTrial requirements. | Two telemetry-only episode getters derived from reference mission/state. |
| `include/V2EscapeDecision.h` | Restore strict finite chosen-side `> hardCollisionCm` gating using 7 cm, commitment/tie/OPEN decisions. | Current finite/positive geometry validation for sensed inputs. |
| `include/RecoveryIrBackoffPolicy.h` | Restore five-pulse per-mission admission and nonblocking pulse/check/front-check transitions, including early IR clear. | Existing actuator cancellation interface `cancelPulse()`. |
| `README.md` | Add current-restoration entry linking this report and marking earlier navigation descriptions historical. | Every previous README byte retained after the prefix. |

### Existing test/support files

These fixtures execute current production bodies with hardware doubles. Expectations encoding rejected V4.6 policies were replaced; unrelated STOP, Manual, sensing and environmental coverage remains.

| Exact path | Reason and preservation |
|---|---|
| `tests/native_collision_closure.cpp` | Replace obsolete direction-ledger admission/reset expectations; preserve environmental hold/rescan/post-turn/front proof, IR and stale completion sequences. |
| `tests/native_navigation_v33.cpp` | Reuse production-navigation harness for restored canonical scenarios. |
| `tests/native_state_collisions.cpp` | Replace active-time/hazard-retained-mission expectations with approved reset; retain STOP, sonar/environmental collisions. |
| `tests/native_v45_availability.cpp` | Adapt removed fields and exact producer debounce/timers; retain actual SensorManager/SafetyManager/Navigation unavailable-producer integration. |
| `tests/native_v45_navigation.cpp` | Reuse fixture entrypoint for canonical recovery; replace positive-only side, six-turn, unlimited-pulse and saved-turn assertions. |
| `tests/native_v45_standalone_dispatch.cpp` | Adapt reverse setup to restored APIs; add AUTO session-loss continuity; preserve actual main STOP/RELEASE/mode ordering. |
| `tests/native_v46_backend.cpp` | Change obsolete direct single-IR spin expectation to reverse preparation; preserve actual HTTP callbacks and genuine STOP/manual-retirement guards. |
| `tests/native_v46_normal.cpp` | Reuse fixture infrastructure for restored ordinary AUTO/full scan/no-path scenarios. |
| `tests/recovery_active_time.cpp` | Replace active-time ledger with elapsed mission, cooldown and wrap boundary assertions. |
| `tests/recovery_episode_v33.cpp` | Verify seven turns, five-pulse mission bounds, reset/retry without direction ledgers. |
| `tests/recovery_ir_backoff_policy.cpp` | Restore reference pulse-policy assertions. |
| `tests/stuck_recovery_v2_policy.cpp` | Restore reference active-history policy assertions. |
| `tests/test_collision_closure.py` | Rename obsolete navigation cases consistently with retained runtime behavior. |
| `tests/test_state_collisions.py` | Name approved hazard reset correctly; retain compilation and unrelated cases. |
| `tests/test_v45_audit_dispatch.py` | Preserve extracted actual main bodies; add session-loss case. |
| `tests/test_v45_availability.py` | Retain producer/Manual cases; rename recovery timing assertions. |
| `tests/test_v45_inventory.py` | Exclude only six restored files from historical V4.5 locks; preserve unrelated hashes. |
| `tests/test_v45_navigation.py` | Map obsolete V4.6 policies to restored production scenarios. |
| `tests/test_v46_inventory.py` | Validate restoration hashes and preserved infrastructure/environmental sonar ownership; retain STOP backend checks. |
| `tests/test_v46_normal.py` | Replace direct-spin/front-only/no-path-reverse expectations with restored full scan. |
| `tests/v2_recovery_integration.cpp` | Restore reference pure policy integration tests. |
| `tests/v45_approved_delta.py` | Recognize explicit restoration manifest before historical static-source projection. |
| `tests/v46_approved_delta.py` | Recognize restoration before existing historical projection; no production source mutation. |

### New tests and evidence

| Exact path | Purpose |
|---|---|
| `tests/native_v32_restoration.cpp` | Shared chronological actual-production scenarios for reference/restored builds; nine preservation scenarios including 54 interruption combinations. |
| `tests/test_v32_restoration.py` | Compile/execute both sources, compare 34 whole traces, run nine preservation scenarios and emit summary. |
| `tests/v32_restoration_approved_delta.py` | Explicit normalized-hash recognition for six reviewed production deltas. |
| `tests/v32_restoration_reviewed_delta.json` | Current hashes, preserved infrastructure hashes and approval metadata; original historical manifests unchanged. |
| `docs/V32_RESTORATION_PREFLIGHT_BASELINE.json` | Initial Git, protection and provenance evidence. |
| `docs/V32_RESTORATION_PREFLIGHT_REPORT.md` | Historical comparison/resolved conflicts, now explicitly superseded. |
| `docs/V32_RESTORATION_BASELINE_TESTS.log` | Executed starting Python suite. |
| `docs/V32_RESTORATION_BASELINE_BUILD.log` | Comparable starting firmware build/resources. |
| `docs/V32_RESTORATION_FIRST_TESTS.log` | Retained intermediate failures. |
| `docs/V32_RESTORATION_PARITY.log` | Earlier parity development run; final host log supersedes it. |
| `docs/V32_RESTORATION_AVAILABILITY.log` | Earlier producer/mission fixture failures retained; final host results supersede them. |
| `docs/V32_RESTORATION_TESTS.log` | Intermediate regression run; final host log supersedes it. |
| `docs/V32_RESTORATION_HOST_CHECKS.log` | Authoritative final native/Python/runtime/browser/JavaScript results. |
| `docs/V32_RESTORATION_BUILD.log` | Authoritative final configured ESP32-S3 build. |
| `docs/V32_RESTORATION_TRACE_SUMMARY.json` | All 34 trace comparisons, pass/transaction counts and trace hashes. |
| `docs/V32_RESTORATION_SOURCE_AUDIT.json` | Exact task deltas, preservation/provenance checks and function comparison. |
| `docs/V32_RESTORATION_FINAL_SOURCE.diff` | Six-production-file HEAD diff; includes identified pre-existing deltas. |
| `docs/V32_RESTORATION_FINAL_DIFF_STAT.txt` | Final HEAD-relative statistics. |
| `docs/V32_RESTORATION_FINAL_GIT_STATUS.txt` | Exact final porcelain status including owner assets. |
| `docs/V32_RESTORATION_CONTINUITY_UPDATE.json` | Authorized factual changes, approvals and software verification for Research Continuity V1. |
| `docs/V32_RESTORATION_SOFTWARE_VERIFICATION.md` | This current A-K handoff. |

Ordinary build/harness outputs use existing ignored `.pio`. Existing caches and unrelated owner assets remain. No project duplicate, new project folder or dependency was created.

## C. Behavioral parity matrix

`MATCH` concerns ordinary production behavior and the modeled inputs, not whole-firmware identity or physical equivalence. Anchors below refer to current `src/NavigationManager.cpp` unless another file is named. The same reference functions were inspected.

| Behavior | Result | Source and executed evidence |
|---|---|---|
| Normal AUTO | MATCH | Lines 497/506/562/573; 20 cm boundary, 300 ms forward, original settle/fresh front. `normal`, `front`. |
| Single LEFT/RIGHT IR including clear front | MATCH | 392/430/438/625; STOP/reverse/full scan before eligible side turn. `left`, `right`, `clear_single`, `single_preference`. |
| BOTH IR | MATCH | Unified reverse-first transaction; `both`. |
| Front blocked | MATCH | 438/625; 50 ms preparation, 220 ms reverse, 150 ms settle/full scan. `front`. |
| Full scan | MATCH | 645/668/705/753/768/816; LEFT 170°, FRONT 90°, RIGHT 10°, three samples/direction, original aggregation/recenter. Whole traces. |
| Side selection | MATCH | 825/950; finite side strictly >20 cm, corresponding IR veto, preference/clearance/tie/history. `choice_left/right`, `tie`, `single_preference`, `side20`. |
| Normal turn | MATCH | 967/1001/1066/1075; 35° plus original 1° comparison, gyro/deadline/fallback/front proof. `normal_fallback`, `normal_deadline`, obstacle traces. |
| No safe side | MATCH | 477/487/825; STOP/1200 ms wait/fresh scan without new reverse. `no_path`, `side20`. |
| Active stuck detection | MATCH | `include/StuckRecoveryV2Policy.h`, callers 573/825/967; 15 s window, three cycles plus two reversals/two short steps/three blocked scans. `history`, `short`, policy tests. |
| Boxed admission/commitment | MATCH | 825/1314/1335; trusted finite positive L/F/R each <30 cm, original IMU and direction commitment. `boxed`, policy integration. |
| Chosen-side <=7 cm veto | MATCH | `include/V2EscapeDecision.h:25`, callers 1335/1363; `side4`, `side6`, `side7`, `side_above`, `side_update`. |
| Incremental recovery/fresh front and side | MATCH | 1001/1075/1354/1363/1392/1401/1512; original 25° gyro turn and reassessment. `increment`, `front_after`, `side_update`. |
| Recovery IR | MATCH | 1449/1476 and pulse policy; five nominal 200 ms pulses, original early clear/front check, no resumed turn remainder. `ir_turn`, `pulse_limit`. |
| Bounds/cooldown/retry | MATCH | 44/1299/1547 and policy; seven started slots, elapsed 30 s mission, 2500 ms cooldown, automatic fresh mission retry. `exhaustion`, `timeout`, `retry`, wrap tests. |
| Recovery exit | MATCH | 1401 and escape decision; finite FRONT >=30 cm or OPEN then ordinary arbitration. `exit`, `open_exit`. |
| OPEN / No Echo | MATCH | 768/1585/1595; unchanged SensorManager, original aggregation/decisions. `open`, `open_exit`, sensor tests. |
| Invalid/stale sonar in ordinary/recovery | MATCH | 44/1119/1149; stopped reacquisition aborts old stuck mission. `invalid`, `stale`, collision tests. |
| STOP/mode | INTENTIONAL NON-NAVIGATION DIFFERENCE | Stronger current cancellation/fences retained, 208/231/315/340 and `main.cpp:163,583,988,1024,1057`. STOP cases, `mode`, actual main/HTTP/Chromium races. |
| Environmental job across sonar loss | INTENTIONAL NON-NAVIGATION DIFFERENCE | Approved ownership/fresh scan retained at 1119/1149; `environment_sonar`, collision tests. |
| Environmental IR/front/post-turn decisions | INTENTIONAL NON-NAVIGATION DIFFERENCE | Current hold/fresh scan/front proof at 44/825/1075 retained; environmental collision tests. Approved V3.2 hazard abort/reset: `hazard`, `critical`. |
| Request freshness/wrap guards | INTENTIONAL NON-NAVIGATION DIFFERENCE | Current request/time/facing checks 506/705/1401/1729 and arrival-order timestamp at 753; no added ordinary transactions. Availability/stale completion tests. |
| Physical motion/RF/latency | UNVERIFIED | No hardware actions. |

The source audit finds 52 whitespace/comment-normalized NavigationManager function bodies equal to the reference and 16 integration-different bodies. Every reference function remains present; two additional interfaces are explained below. Structural comparison supplements runtime evidence.

## D. Removed V4.6 navigation complexity

Removed separate NORMAL backoff/checkpoint decisions that could advance after temporary front clearance, direct single-IR turns and repeated no-path reverse/rescan. Ordinary events now use the reference unified transaction and blocked retry.

Removed BOXED saved scan/phase/remaining-turn continuation, direction masks/time ledgers, six-slot rounds, active-time mission accounting and ForwardTrial progress/admission. Restored compact `Normal/Suspected/EscapeScan/EscapeTurn/EscapeVerify/TrappedHold` and original elapsed per-mission bounds. IR interruption clears turn integration; fresh front/chosen-side assessment precedes another newly counted turn.

Retained useful current request freshness/wrap guards, full STOP/mode/critical cancellation, environmental ownership/hold/rescan/front proof and diagnostics. `observeIrEpisodes()` at line 193 is an intentional no-op compatibility interface for unchanged main callers; confirmed IR is arbitrated in `updateAuto`. `requestedSample()` is the current freshness helper. Episode getters only project reference state/timestamp into telemetry.

MotorController's capped recovery actuator API/configuration remains byte unchanged. Restored navigation calls actual V3.2 `backward()` at operator-selected calibrated speed for recovery pulses instead of the newer capped navigation command. This is authorized restoration of navigation motor output; native actuator tests independently verify the retained cap API.

Reference legacy `StuckRecovery`/forward-evidence helpers remain as part of the reference call chain. Reference environmental fallback branches in ordinary `selectPathAfterScan` remain; normal active environmental ownership is handled by the earlier preserved job branch. They add no control pass. Redundant nested active-job guards and a post-settle tail unreachable because both branches return were removed. No unrelated refactoring or second recovery authority was added.

## E. Preserved existing functionality

Every production file outside the six approved files, plus `platformio.ini`, matches its starting working-tree hash. Secret-bearing configuration was checked by hash without exposing content.

| Functionality | Evidence and limitation |
|---|---|
| Manual control | TESTED: native gates, actual DriveCommandHandler/main/HTTP, unavailable-sensor profiles. Physical movement unverified. |
| Manual override/rearm | TESTED: policy and actual main/SafetyManager producer integration; lifecycle source preserved. |
| STOP/RELEASE/mode | TESTED: same-pass dispatch, ready front/gyro, genuine old-sequence STOP, retirement guards and Chromium races. Browser/backend byte preserved. |
| Wi-Fi AP/STA | SOURCE-REVIEWED/byte preserved; STA policy tests pass. Actual main session-loss double verifies AUTO continuity and STOP. Real reconnect/radio conditions UNVERIFIED. |
| Dashboard/HTTP | TESTED: actual callbacks and actual embedded JS/Chromium races; parse passes. Appearance/assets preserved. |
| Radio | SOURCE-REVIEWED/byte preserved; compile/link passes. RF/application packet transport physically UNVERIFIED. |
| Environmental sensors | TESTED: actual SensorManager/SafetyManager validity/age/debounce/unavailable producer profiles under web servicing. Electrical measurements UNVERIFIED. |
| Safety monitoring | TESTED: mission reset, critical fence, environmental IR/post-turn/rescan/sonar ownership. Physical safety UNVERIFIED. |
| Telemetry/logging | SOURCE-REVIEWED; publisher/trace services preserved. Trace ring/checksum/schema pass; navigation fields retained. Hardware transport timing unverified. |
| IMU | TESTED modeled gyro completion/deadline/fallback/availability; interface source preserved. Physical angle/bias/noise unverified. |
| Display/buzzer | SOURCE-REVIEWED/byte preserved and compiled. Physical behavior unverified. |
| Firmware/pins/power/secrets | BYTE PRESERVED outside authorized navigation constants; ESP32-S3 and native motor checks pass. No hardware/credential change. |

## F. Performance and latency

All 34 paired cases match complete chronological output: time, phase, motor action/command counts, requested sonar count, servo command/angle, phase start, mission turn/pulse counts and integrated angle. Aggregate traces contain 3,161 control passes and 481 sonar requests in each version. [Per-case summaries](V32_RESTORATION_TRACE_SUMMARY.json) provide counts/hashes. These are modeled workloads, not real-time benchmarks or exhaustive input coverage. No unexplained extra request/pass occurs in the compared cases.

| Timing | Reference and restored ordinary path |
|---|---|
| Obstacle/IR to STOP | Same navigation update; current main STOP fence still wins same outer pass. Input debounce/detection latency unmeasured. |
| STOP/reverse/scan | 50 ms preparation, 220 ms reverse, 150 ms stopped settle, scan. |
| Forward cycle | 300 ms movement, 100 ms STOP settle, 120 ms front settle/fresh request. |
| Directional scan | LEFT/FRONT/RIGHT, 300 ms servo settle each, three readings with 45 ms gaps, 300 ms recenter. Sensor completion time remains input dependent. |
| Turn/front proof | Original STOP, 300 ms post-turn settle, 120 ms front settle/request. |
| Normal turn | 35° target plus original 1° comparison; 2500 ms deadline, 1500 ms original non-gyro fallback. |
| Recovery turn | 25° target and original gyro/tolerance/IMU requirements; 2500 ms deadline. |
| IR recovery | Nominal 200 ms pulses, original early clear except required first pulse; five per mission. |
| No-path/retry | 1200 ms no-path wait; 30 s elapsed mission, 2500 ms cooldown then automatic retry. |

No new production `delay`, sleep, busy wait, dynamic allocation, networking, polling loop or protocol was added. Removed checkpoint/continuation/ledger processing reduces complexity. Main retains web/sensor/radio/background services. Environmental fresh scan is preserved behavior and the approved exception, not an added ordinary scan.

| Same configured build | RAM bytes | Flash bytes |
|---|---:|---:|
| Starting working tree | 58,956 | 1,043,933 |
| Final restored tree | 58,908 | 1,042,541 |
| Delta | -48 | -1,392 |

Both builds use unchanged configuration/dependencies, environment `esp32-s3-n16r8`. Build duration is not navigation latency. Static resources do not measure peak stack/heap. **PHYSICAL LATENCY NOT VERIFIED.** Smoothness, stopping distance, wheel output and escape need owner testing.

## G. Tests and build

Final `python tests/run_host_checks.py`: exit **0**. **12 native C++ policy executables passed** under C++11 `-Wall -Wextra -Werror`; **172 Python tests: 168 passed, four skipped, zero failed**, 57.468 s; embedded dashboard JS parse passed. Browser tests execute actual embedded JS in Chromium through the existing Deno fixture. [Final host log](V32_RESTORATION_HOST_CHECKS.log).

Four skips are explicit standalone exclusions: one absent CPS integration authority fixture and three CPS seams. No standalone failure was skipped. Legacy aliases do not represent additional distinct reference comparisons.

The new comparator compiles actual read-only reference/current NavigationManager separately with compatible shared declared types/hardware doubles: **34 whole-trace comparisons plus nine preservation cases pass**. One preservation case contains the **nine state-context x six event matrix, 54 combinations**. This executes production algorithms under modeled inputs, not complete reference firmware on ESP32.

| Required contract scenario | Executed modeled coverage |
|---|---|
| 1 Normal AUTO | `normal` |
| 2 Front obstacle | `front` |
| 3 LEFT IR | `left` |
| 4 RIGHT IR | `right` |
| 5 BOTH IR | `both` |
| 6 Clear front with single IR | `clear_single` |
| 7 Full scan | Obstacle/boxed traces, exact servo/request sequence |
| 8 Safe-side selection | `choice_left/right`, `single_preference`, `side20` |
| 9 Equal clearance | `tie`, policy history |
| 10 No safe side | `no_path` |
| 11 Repeated avoidance | `history` |
| 12 Short forward failure | `short` |
| 13 Boxed admission | `boxed` |
| 14 Side 4 cm | `side4` |
| 15 Side 6 cm | `side6` |
| 16 Side 7 cm | `side7` |
| 17 Side >7 cm | `side_above` |
| 18 Incremental recovery | `increment` |
| 19 Fresh FRONT after turn | `front_after` |
| 20 IR during recovery turn | `ir_turn` |
| 21 Pulse limit | `pulse_limit`, pure policy |
| 22 Exhaustion | `exhaustion`, `timeout` |
| 23 Cooldown/retry | `retry`, wrap/cooldown policy |
| 24 Front-clear exit | `exit`, `open_exit` |
| 25 OPEN / No Echo | `open`, `open_exit`, aggregation/availability |
| 26 Invalid/stale sonar | `invalid`, `stale`, stale-completion collisions |
| 27 STOP reverse | `stop_reverse`, actual dispatch |
| 28 STOP turn | `stop_turn`, matrix |
| 29 STOP recovery | `stop_recovery`, ready front/gyro, HTTP/main races |
| 30 AUTO to Manual | `mode`, actual main `v46_mode`, backend/browser |
| 31 Manual to AUTO | `mode`, actual main `v46_mode`, backend/browser |
| 32 Wi-Fi interruption | Actual main `wifi_auto` with session double; physical Wi-Fi unverified |
| 33 Hazard AUTO/recovery | `hazard`, `critical`, matrix, actual producer/SafetyManager |
| 34 Manual commands | Gates/policy, actual main/DriveCommandHandler/HTTP |
| 35 Dashboard/control | Actual backend and Chromium STOP/mode ordering |

Matrix contexts are idle/front start, reverse preparation, reverse, reverse settle, scan, normal turn, recovery turn, IR recovery and recovery front proof. Events: STOP, Manual, critical, environmental hazard, sonar fault/stale. Ready gyro/front/pulse progress is present at boundaries; two subsequent updates must remain stopped without new movement. Broader preserved tests cover environmental scan/IR/reacquisition/front proof and same-pass HTTP STOP/RELEASE at recovery completion.

Updated test expectations are explained in B. Hash-pinned historical projections verify historical static contracts/protected inventories; they are not current runtime proof. Original historical manifests remain unchanged. The current comparator plus collision/dispatch/backend/browser fixtures supply current runtime evidence.

Starting Python baseline: 128 tests, 124 passes/four scope skips; starting build passed. Intermediate failures revealed obsolete V4.6 assertions, missing restoration wiring, fixture debounce/timer assumptions and a real environmental post-turn request cancellation introduced during integration. The environmental branch now returns after starting its front proof, preventing tail cancellation. Final full host/build checks were rerun after the last source cleanup and passed. Historical logs are retained and superseded.

Final `platformio run -e esp32-s3-n16r8`: exit **0**, **SUCCESS**, 11.84 s. [Final build log](V32_RESTORATION_BUILD.log). No compiler warning/error appears in final compile output. PowerShell's host-log `NativeCommandError` wrapper surrounds normal unittest stderr progress; actual exit/results confirm success. Existing Git LF/CRLF advisories are informational; `git diff --check` passes.

## H. Final independent review

This is a separate final inspection by the implementing agent, not a claim of an independent second reviewer. Source diff/evidence are ready for independent owner review.

| Concern | Finding with current source location |
|---|---|
| Behavioral mismatch | No ordinary mismatch in reconstruction or 34 comparisons; approved infrastructure differences listed in C. `NavigationManager.cpp:392,438,625,825,1299,1314,1449,1512`. |
| Stale state/replay | Complete STOP/mode/critical/sonar resets preserve appropriate ownership and clear old maneuver/mission state. `:208,231,340,1119`; collision tests pass. |
| Duplicate recovery | Suspected/startEscape/commit guards remain one stuck-mission authority. Environmental boxed handoff completes job before restored admission. `:825,1314,1335`. |
| Motor continuation | Operator/critical/sonar precede timeout/IR/phase completion. `:44,315,340`; outer fences `main.cpp:1024,1057`; ready-event matrix passes. |
| STOP/Manual collision | Existing callbacks/ordering unchanged and reachable. `main.cpp:163,198,583,988`; actual dispatch/HTTP/browser pass. |
| Sensor collision | Fault/stale stops precede completion; request identity/time/facing prevent stale proof. `:44,705,1401,1729`; actual sensing/stale-completion pass. |
| Hazard mission | Approved V3.2 abort/reset after safety arbitration, `:44,340,1184`; no old stuck mission retained. |
| Environmental ownership | Sonar preserves unfinished job, then fresh scan; IR hold/rescan retained. `:44,825,1119,1149`; front proof persists at `:1075`; tests pass. |
| Background service | Source preserved; actual main/SensorManager/SafetyManager under web servicing passes. `main.cpp:583,988`; real scheduling/RF latency unmeasured. |
| Dead paths/complexity | Removed conflicting checkpoint/continuation/ledger and unreachable post-turn tail; retained reference fallback/compatibility interface documented in D. |
| Latency | No added hot-loop blocking/network/heap work; paired counts equal and static resources reduced. Physical timing/peak runtime memory unverified. |

No unexpected source change or unapproved policy conflict was found. Reviewed source and software models do not establish exhaustive correctness, RF behavior or physical safety.

## I. Physical validation checklist

Owner testing follows review of this build, then independent review of physical observations. Record firmware/configuration/speed, sensor/IR and motor/phase timelines, power, environment and repeatability separately from software evidence.

- [ ] Repeat prior V3.2 forward/front <=20 cm obstacle scenarios: STOP/reverse/settle/full scan/post-turn front proof.
- [ ] Repeat LEFT/RIGHT/BOTH IR, including clear front; observe scan-based turn decisions.
- [ ] Repeat eligible sides/ties/no-safe-side; measure scan cadence and 1200 ms stopped reassessment.
- [ ] Repeat short failed forward, alternating/repeated avoidance and chair-leg trapping; confirm history/boxed entry.
- [ ] Check 4/6/7/>7 cm chosen sides and finite L/F/R <30 cm admission/commitment.
- [ ] Check 25° recovery increments, fresh FRONT/chosen side, >=30 cm and OPEN exit.
- [ ] Interrupt recovery with either IR: five nominal 200 ms selected-speed pulses per mission, no old turn remainder.
- [ ] Observe seven started turns, 30 s timeout, 2500 ms cooldown and automatic retries.
- [ ] Exercise STOP/RELEASE/mode/sonar/hazard at completion boundaries; confirm hazard reset and environmental ownership/fresh scan.
- [ ] Verify Manual override/permission, dashboard, AP/STA/reconnect, radio, environmental sensing, telemetry/display/buzzer on hardware.
- [ ] Measure obstacle-to-STOP, stopping distance, wheel output, smoothness, scan/turn/recovery latency against comparable owner-tested V3.2 conditions.

No checklist item was physically run. Stage 4 is future work after owner testing and independent review.

## J. Explicit final answer

**Yes, at the reviewed source and executed modeled software level.** The existing standalone project now runs V3.2's approved ordinary AUTO and stuck/boxed-in recovery, applies both owner decisions and preserves unrelated current production infrastructure. No extra ordinary sensing/pass count appeared in the 34 comparisons and no new hot-path blocking work was introduced.

**SOURCE VERIFIED:** actual reference provenance, reachable algorithms/bounds, removal of conflicting navigation, protected hashes and explicit integration differences.

**TEST VERIFIED:** 34 whole-trace comparisons, nine preservation scenarios including 54 matrix combinations, 12 native policies, 172-test suite with four explicit CPS exclusions, actual backend/browser ordering and configured ESP32-S3 build.

**PHYSICALLY UNVERIFIED:** this build's motion, sensing/electrical/RF behavior, safety, timing and smoothness. Earlier V3.2 physical success motivates restoration but does not validate this build. Complete whole-system V3.2 identity is not claimed: current STOP/freshness/environmental infrastructure is intentionally retained.

## K. Git and handoff

Directory/branch/HEAD are in A and the [source audit](V32_RESTORATION_SOURCE_AUDIT.json). [Complete final porcelain status](V32_RESTORATION_FINAL_GIT_STATUS.txt) retains owner dirty/untracked state. Six task-modified production files are in B. [Source diff](V32_RESTORATION_FINAL_SOURCE.diff) and [statistics](V32_RESTORATION_FINAL_DIFF_STAT.txt) are against HEAD, so include identified pre-existing owner deltas. The baseline-to-final hash audit is the exact task-change inventory; pre-edit hashes alone cannot reconstruct a working-tree-text patch and the HEAD diff is not mislabeled as task-only.

All final applicable tests/build pass. Non-navigation production/configuration, four owner figures and all 90 reference files are byte unchanged. Of 147 target baseline files, 117 are byte unchanged; prior README body is intact. No unexpected changes. Evidence remains in the project; Research Continuity V1 records the authorized facts/approvals/software handoff.

No branch creation/switch, destructive Git operation, commit/push/merge, upload, robot connection/movement or Stage 4/Node V1 development occurred. Ready for owner source review and physical validation.

Research Continuity receipt: `mission-20261008-192558-44c8a53e`, outcome `completed`; store validation PASS (event sequence 295, 88 receipts). The receipt explicitly separates software completion from pending owner physical validation.
