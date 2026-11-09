> Historical preflight record. Both requested decisions were subsequently approved and implementation completed. The current result and A-K handoff are in [V32_RESTORATION_SOFTWARE_VERIFICATION.md](V32_RESTORATION_SOFTWARE_VERIFICATION.md). Statements below describe the preflight stage only.

# V3.2 restoration development report ? blocked preflight

Date: 2026-10-08 (Asia/Dhaka). Evidence describes the current working tree, not HEAD. No restoration source edits have been applied.

## A. Overall result

**Historical preflight stop resolved by owner decisions: restore V3.2 hazard abort/reset; preserve V4.6 unfinished environmental-job ownership across sonar reacquisition. Implementation is now authorized.**

Development target: `E:\Development\Research\Meo\The PIKU\PIKU 2.1 Rev A\PIKU 2.1 Rev A Software Development`.
Initial shell directory was CPS Node V1 Stage 4; all task-created artifacts are in the standalone target. Stage 4 and Node V1 were not modified.
Branch: `codex/reva-collision-closure-20261005`.
HEAD: `cbbf3599d0a6192976f40d991250cd38a02c7abb`.
Reference: `C:\Users\User\Downloads\PIKU-2.1-Rev-A-earlier github code`.

61 non-secret source/test/configuration files in the reference exactly match frozen executable commit `41281d8325d88c70e074e008a2550a1164102132` after CRLF/LF normalization. The matching comparison excluded secret-bearing files. No reference file was modified.

Preflight recorded 28 modified tracked files, untracked V4.5/V4.6 evidence and fixtures, four owner figure PNGs, and existing Python caches. The complete status and byte hashes are in `V32_RESTORATION_PREFLIGHT_BASELINE.json`. Existing deltas in NavigationManager, policies, configuration, SensorManager, SafetyManager, main, and dashboard ordering were inspected. Their roles are identifiable as prior V4.5/V4.6 navigation and sensing/control work; no unidentified region was overwritten. The target parent README requested by local AGENTS.md does not exist; the target README and required workstation guidance were read. This does not prevent positive target identification.

Completed: target/reference identification, Git preflight, non-secret reference provenance, relevant existing-diff review, source call-chain comparison, protected hashes, identification of mandatory policy conflicts. Checkpoints 2?6 remain pending.

### Approval 1: environmental hazard interrupts stuck recovery ? RESOLVED

Reference `src/NavigationManager.cpp:54` calls `handleCriticalSafety()` before navigation. When recovery is non-NORMAL, lines 57?59 call `_v2.abort()` and `_recoveryIrBackoff.resetMission()`. Reference `include/StuckRecoveryV2Policy.h:35` clears history, committed side, turn slots, and cooldown; it does not explicitly zero `attempts_` in `abort()`.

Target `src/NavigationManager.cpp:55` instead checks an active episode, pauses at line 60, sets `_recoveryHazardPaused`, cancels the actuator pulse, and preserves mission accounting. `enterEnvironmentalHazard():1225` stops motors, pauses policy, and hands ownership to environmental recovery. Target `include/StuckRecoveryV2Policy.h:25` pauses accumulated active work; `escapeTimedOut():120` uses active work. Reference `escapeTimedOut():90` uses elapsed wall time since the mission start.

**Call chain:** confirmed environmental hazard -> `src/main.cpp:988` AUTO hazard dispatch -> `updateAuto()` -> `handleCriticalSafety()` -> `enterEnvironmentalHazard()` -> reference abort/reset versus target pause/retain -> environmental clear wait/full scan -> subsequent navigation/recovery arbitration.

Minimal options:
1. Preserve V4.6 environmental preemption, retained accounting, and ownership as an explicit non-navigation exception to exact V3.2 parity. The restored seven-turn/five-pulse/30-second mission policy must be integrated with this expressly approved exception; exact clock and interruption expectations need to be stated in implementation evidence.
2. Approve the environmental-policy change to restore V3.2 abort/reset semantics. This removes retained recovery mission work across a hazard interruption while preserving immediate STOP and the dashboard ordering.

Owner explicitly selected option 2 during this task: restore V3.2 hazard abort and mission reset; this hazard-policy change is approved. No source implementation has been applied because Approval 2 remains pending.

### Approval 2: sonar failure interrupts unfinished environmental recovery ? RESOLVED

Reference `beginSonarReacquire():1047` resets the pulse mission (1056), aborts recovery (1058), marks an active environmental job completed (1063), and clears its active flag (1064).

Target `beginSonarReacquire():1151` cancels the actuator pulse without resetting mission counts, pauses recovery, explicitly leaves environmental ownership active, and clears the completed flag. Target `updateSonarReacquire():1182` restarts a fresh environmental full scan before other navigation when that job remains active. `src/main.cpp:1035` consumes environmental completion to release the critical-stop latch.

**Collision:** V3.2's cancellation/completion branch would release unfinished environmental ownership; V4.6's preserved branch requires fresh environmental evidence. Choosing between them changes a protected policy. The contract sections 8 and 14(5,6) require owner approval.

Minimal options:
1. Preserve V4.6 unfinished-job ownership and required fresh environmental scan as an explicit parity exception.
2. Approve restoring V3.2 environmental cancellation/completion semantics, with focused dispatcher and hazard/sonar chronology tests before handoff.

Owner selected option 1: preserve V4.6 environmental job and fresh scan; the parity exception is approved. No claim is made that either option has physical safety validation.

## B. Files modified by this task

No existing source, header, test, configuration, README, figure, or secret file changed.
New local evidence only:
- `docs/V32_RESTORATION_PREFLIGHT_BASELINE.json`: branch/HEAD/status and raw SHA-256 inventory; no source or credential contents.
- `docs/V32_RESTORATION_PREFLIGHT_REPORT.md`: this report and precise owner decisions.
- `docs/V32_RESTORATION_CONTINUITY_UPDATE.json`: structured receipt metadata.

All 28 pre-existing tracked modifications remain owner work. Their full paths are preserved in the baseline JSON. Research Continuity records are the metadata recording expressly required by applicable AGENTS instructions, not firmware changes or updates to other projects.

## C. Behavioral parity matrix

References below are `src/NavigationManager.cpp` unless a different file is named. R = read-only V3.2 reference; T = current standalone working tree. All classifications are SOURCE VERIFIED only; none are runtime parity test evidence.

| Behavior | Classification | Source evidence / current difference |
| --- | --- | --- |
| Normal AUTO, no obstacle/IR | MATCH (bounded cadence only) | R `updateForward():539`, T `updateForward():690`; configured 300 ms forward and STOP/front evaluation. Full cross-state parity remains unverified. |
| Single IR / clear front with single IR | MISMATCH | R `beginIrRecovery():401` enters unified reverse/full scan; T `beginIrRecovery():459` directly starts opposite normal spin. |
| Finite front blocked | MISMATCH | R `updateReverseSequence():584` leads directly to full scan after reverse/settle; T normal backoff logic at 494/508 permits new-front clearance before scan. Both use <=20 cm obstacle threshold. |
| Full directional scan | MATCH (physical sequence and aggregation), partial UNVERIFIED | `commandCurrentScanDirection`, `finalizeCurrentScan`, `storeCurrentScan`, `beginRecenter` function bodies match exactly. Sample request acceptance and accumulator input guards differ; these need chronological comparison. |
| Ordinary side eligibility / ties | MATCH (shared selector), MISMATCH (single-IR preference caller) | R `selectClearestSide():878`, T `:987` bodies identical: corresponding IR restriction, strictly >20 cm, 1 cm tie tolerance, alternating tie. R scan selector applies source-specific single-IR preference; current caller differs. |
| No safe side | MISMATCH | R `selectPathAfterScan():787` -> blocked retry STOP/wait, R `updateBlockedRetryWait():458` uses 1200 ms then scan; T `selectPathAfterScan():951` -> another normal reverse/full scan. |
| Active stuck history | MISMATCH | R scan selector calls `noteAvoidance`, `noteBlockedScan`; R ordinary turn admission calls history. T scan selector lacks those calls; policy `noteAvoidance():72`, `noteBlockedScan():99`, `noteTurn():104` retain diagnostic counters without `evaluate()`. |
| Direct boxed admission | MATCH (geometry predicate), MISMATCH (mission semantics) | Trusted finite L/F/R<30 is present in both active selectors. T also admits BOXED from environmental scans, whereas R excludes environmental scans from V2 ordinary admission. |
| Recovery selected side | MISMATCH | R `V2EscapeDecision.h:21` requires >7 cm; T `:22` ignores hardCollisionCm and allows any positive finite side. |
| Incremental recovery gyro core | MATCH (integration/completion body), MISMATCH (admission/bounds) | `updateSpinTurn` function body is identical. R turn slots=7; T=6. Both targets 25 degrees and retain 2500 ms turn deadline. |
| Recovery IR intervention | MISMATCH | R `beginRecoveryIrBackoff():1369` clears interrupted turn execution, up to five pulses then new front; T `:1443` saves phase and `resumeRecoveryPhase():1706` resumes remaining turn/phase. T policy counts do not inhibit at five. |
| Retry/cooldown/mission timeout | MISMATCH | R configuration: seven slots, 30000 ms mission wall time, 2500 ms cooldown, five pulses; T: six slots, 45000 ms direction active time, 1000 ms reassessment, unbounded diagnostic pulse count. |
| Recovery front-clear exit | MATCH (>=30/OPEN predicate), MISMATCH (ordering/handoff) | R `updateRecoveryFrontCheck():1320` and T `:1419` accept >=30/OPEN. T prioritizes ready regular clearance over timeout and immediately dispatches current IR through `completeBoxed`; R top-level timeout/IR arbitration and Idle handoff differ. |
| Sonar OPEN/no echo | MATCH (operational interpretation) | Both accept ClearNoEcho as OPEN in front, scans and recovery; scan aggregation body identical. This is not proof of sensor integrity or measured free space. |
| Invalid/stale sonar interruption | MISMATCH / protected integration conflict | R reacquisition aborts mission and clears environmental job; T retains accounting and environmental ownership. Approval 2 above. |
| STOP and mode transitions | INTENTIONAL NON-NAVIGATION DIFFERENCE | Preserve T same-pass fences `src/main.cpp:1024,1057`, existing callbacks and dashboard promises. `handleOperatorInterruption` body matches R. End-to-end production behavior has not been executed in this task. |
| Environmental-hazard interruption | MISMATCH / protected integration conflict | Approval 1 above: R abort/reset, T pause/retain; no decision silently selected. |

## D. V4.6 navigation complexity

Nothing removed or replaced. Identified replacement candidates within approved navigation scope: NORMAL backoff purpose/history/checkpoint policy, direct single-IR spins, no-path reverse/rescan, BOXED saved phase/turn continuation, six-turn rounds, direction time ledger, positive-only side rule, diagnostic-only stuck history, unlimited pulse admission.

Existing dashboard, main dispatcher, sensing/freshness, communication and environmental-job authority are preservation requirements. Environmental preemption and sonar-job ownership require the explicit decisions above. No dead path was deleted, and no duplicate authority was added.

## E. Preserved existing functionality

| Feature | Status in this task |
| --- | --- |
| Manual control / override | SOURCE-REVIEWED at dispatcher and interface boundaries; byte-preserved; runtime unverified |
| STOP authority / mode cancellation | SOURCE-REVIEWED; same-pass main fences and callbacks preserved; runtime unverified |
| Dashboard / control ordering | Byte-preserved; existing diff reviewed; browser/backend tests not run |
| Wi-Fi/AP/STA / radio | Byte-preserved; runtime/RF unverified |
| Environmental sensing / safety monitoring | Pre-existing SensorManager/SafetyManager/main changes reviewed and preserved; runtime unverified |
| Telemetry / IMU / display / buzzer | Byte-preserved; IMU turn core compared; physical behavior unverified |
| Pins / motor hardware / firmware configuration / secrets | Byte-preserved; no new dependency or credential operation |

## F. Performance and latency

No control-loop change, sensing operation, blocking wait, network request, dynamic allocation or motor operation introduced.
Reference timings verified from source: 300 ms forward, 50 ms reverse preparation, 220 ms ordinary reverse, 150 ms reverse settle, 120 ms front center settle, 300 ms each directional servo settle, three samples/direction separated by 45 ms gaps, 300 ms recenter/post-turn settle, 1200 ms no-path wait. Normal turn target=35 degrees with the original target+1-degree completion rule, 2500 ms hard timeout and 1500 ms timed fallback. Recovery target=25 degrees; seven started slots, five nominal 200 ms IR pulses, 30000 ms elapsed mission timeout and 2500 ms cooldown.

Scan aggregation identical does not establish identical request timing across interruptions. Control-pass counts, motor-command histories, sensor-request counts and resource differences were not measured. No current build was run; current RAM/flash unavailable. Historical reference build numbers in repository documentation are historical evidence only and not a comparable current build. **PHYSICAL LATENCY NOT VERIFIED.**

## G. Tests and build

No builds or host/browser/runtime tests executed after finding mandatory stop conditions. No test expectation changed. Only read-only source/reference comparisons and byte-hash verification ran.

Pending after owner decisions: production-backed chronological V3.2/current parity harness; all 35 contract scenarios; preserved STOP/Manual/environmental dispatcher histories; sensing producer and availability checks; actual backend and browser mode/STOP checks; configured `pio run -e esp32-s3-n16r8`; diagnostic/resource review; final authorized diff/hash audit.

Tests encoding rejected policy were identified before editing: `tests/native_v46_normal.cpp` / `test_v46_normal.py` (direct single IR, front backoff/new-front, no-path reverse); `tests/native_v45_navigation.cpp` / `test_v45_navigation.py` (low finite sides, six turns, saved-turn continuation, direction ledger and ten pulses); `test_v46_inventory.py` (exact BOXED lock). Preserve their infrastructure and unrelated STOP/sensing/environmental coverage when replacing obsolete navigation expectations. Historical manifests/reports are evidence, not current parity proof.

## H. Final source review at stopped preflight

1. Environmental hazard x active recovery: abort/reset versus retained budget conflict at R 54?59 / T 55?64. Approval required.
2. Environmental scan x sonar failure x main critical-stop latch: R 1063?1064 marks unfinished job completed; T 1182+ resumes environmental scan and main consumes explicit completion. Approval required.
3. Ordinary IR x clear front: active T direct spin contradicts R reverse/full scan (459 versus 401).
4. Front obstruction x temporary reverse clearance: T normal backoff/front transaction can move without R-required scan (508 versus 584).
5. Small recovery side x gyro readiness: T positive-only gate allows 4?7 cm; R veto rejects <=7 (`V2EscapeDecision.h:22` versus R 21).
6. Recovery turn x IR interruption: T saved-turn resumption remains reachable (1443 -> 1458 -> 1706); reference clears execution and takes front/side evidence.
7. Exhaustion x retry: current six-slot/45-second/1-second ledger differs from reference seven-slot/30-second/2.5-second automatic missions.

No newly introduced stale-state, motor continuation, duplicate recovery or latency regression exists because no implementation occurred. These findings describe existing source, not measured physical outcomes. Runtime cross-state review remains pending.

## I. Owner physical validation checklist (after software completion)

- Normal bounded AUTO steps and obstacle STOP responsiveness.
- Finite front obstruction, each single IR, clear-front single IR and BOTH: observe reverse/full scan and eligible side choice.
- Symmetric/asymmetric side clearances, no-path STOP/wait/reassessment.
- Repeated short failed steps/alternating avoidance and chair-leg trapping: history admission and escape.
- Recovery selected-side boundaries at 4?6, exactly 7 and above 7 cm, with supervised clearance.
- Incremental turns, post-turn front check, IR interrupt pulses and no old-turn replay.
- Seven-slot/five-pulse/time exhaustion, 2500 ms cooldown and automatic fresh retry.
- >=30 cm/OPEN recovery exit and ordinary AUTO continuation.
- STOP during reverse/normal turn/recovery; AUTO?MANUAL transitions; connection interruption.
- Hazard/sonar interruptions according to the explicitly approved policy, plus preserved Manual/dashboard/sensor behavior.

Use existing supervised V3.2 scenarios. No hardware was connected, flashed, or moved by this task.

## J. Explicit answer

**No.** The current existing V4.6 standalone project does not yet reproduce the approved V3.2 AUTO/Recovery behavior. It remains the untouched pre-existing working-tree implementation.

SOURCE VERIFIED: reference provenance, major policy differences, exact environmental conflicts and preserved baseline bytes.
TEST VERIFIED: no runtime restoration parity tests or current build evidence.
PHYSICALLY UNVERIFIED: all proposed restoration behavior, latency and current firmware robot performance.

## K. Git and handoff

Same branch and HEAD; no branch creation/switch, commit, push, merge, stash, reset, clean, deploy or hardware upload. Only the three local evidence files above were added by this task. No source diff introduced; all tracked diffs predate this task. Final byte verification and exact porcelain status are recorded in the receipt update. No implementation is authorized to silently decide the two conflicts; the remaining sonar/environmental owner response is the next dependency.

Research Continuity mission: `mission-20261008-192558-44c8a53e` (preflight scope; both mandatory decisions resolved; subsequent implementation evidence supersedes the historical blocked status above).
