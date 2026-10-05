> Historical report for the state-collision starting candidate. Its original evidence and claims below are preserved. Current narrow corrections and residual boundaries are recorded in [collision closure verification](COLLISION_CLOSURE_SOFTWARE_VERIFICATION.md).

# PIKU / CPS system state collision verification

2026-10-05, Asia/Dhaka. Local software handoff. PHYSICAL-PENDING.
Research Continuity V1: mission-20261005-013114-7924802b.

Evidence labels: CODE-REVIEWED = source inspection; REGRESSION-TESTED = host execution of production classes/bodies with hardware doubles; BUILD-TESTED = ESP32-S3 compilation; BROWSER-TESTED = actual embedded assets in headless Edge with fixture responses; CROSS-REPO-PARITY-TESTED = exact core/protocol checks. None establish physical timing, clearance or RF reliability.

## A. Starting repository state

| Repository | Approved starting branch | Exact approved HEAD |
| --- | --- | --- |
| Standalone | codex/reva-v331-hardening-20261004 | 8f294741cb736fa77274250007f77429570ab978 |
| Stage 4 | codex/stage4-v331-radio-hardening-20261004 | 75d3ecb4e27ea8551ebcdf7dc7951725256d74dc |
| Coordinator | codex/nodev1-stage5d1-hardening-20261004 | 7112c24d6f89df59df658b5360166b76cdfd88ed |
| Frozen Node | main | 70ef949ce18ab81cbc207fed13bcc320d8b99964 |

- E:\Development\Research\Meo\The PIKU\PIKU 2.1 Rev A\PIKU 2.1 Rev A Software Development
- E:\Development\Research\Meo\The PIKU\PIKU 2.1 Rev A\PIKU 2.1 Rev A Stage 4 CPS Integration
- E:\Development\Research\Meo\CPS Node V1\CPS Node V1 Stage 4 Coordinator Integration
- E:\Development\Research\Meo\CPS Node V1\Node V1 Software development

Exact branches, HEADs, porcelain state and protected byte hashes were captured before edits. Frozen Node was read only. Standalone's four untracked owner PNGs were excluded from editing and staging. Frozen Node's modified .vscode/extensions.json and untracked AGENTS.md were owner-confirmed pre-existing. Secrets.h, WiFiConfig.h, PinConfig.h and platformio.ini were protected by byte hashes; no credentials appear here.

## B. Exact changed-file inventory

Repository-specific inventory, including this report:

- README.md
- SOFTWARE_EXPLANATION.md
- architecture_source/auto_recovery_state_machine.mmd
- docs/SYSTEM_STATE_COLLISION_VERIFICATION.md
- docs/TESTING.md
- include/StuckRecoveryV2Policy.h
- src/NavigationManager.cpp
- src/main.cpp
- tests/dashboard_browser_check.js
- tests/native_state_collisions.cpp
- tests/recovery_active_time.cpp
- tests/recovery_episode_v33.cpp
- tests/stuck_recovery_v2_policy.cpp
- tests/test_control_pass_collisions.py
- tests/test_recovery_source_contract.py
- tests/test_state_collision_browser.py
- tests/test_state_collisions.py
- tests/test_v32_source_contract.py
- tests/test_v33_contracts.py
- tests/v2_recovery_integration.cpp

Production scope: three Standalone files, four Stage 4 files and six Coordinator files. Historical releases/evidence reports, physical checklists, screenshots, owner assets and frozen files are excluded. Test projections now apply a separately fingerprinted collision layer before older layers; historical object hashes and older inventories remain checked. Fixtures reflect fresh application Status, two-packet reboot confirmation and timestamped handoff without removing motor-stop, old-hash, sensor or packet-layout assertions.

## C. Reproduced defects and provenance

Before shared-core changes, production NavigationManager regressions failed for false EnvironmentalRecovery completion on sonar loss; incomplete environmental work falling into ordinary IR recovery; active-episode hazard clearance skipping full L/F/R; watchdog resets/counting at pause/retry/checkpoint; and pause erasing TrappedHold. Seventeen original isolated cases produced eight Standalone failures with three CPS-only skips. Stage 4 produced eleven failures including both sonar/integration nesting orders and integration restoration choosing the wrong checkpoint.

A production progression regression demonstrated the obvious-reboot shortcut accepted one lower-uptime packet. Stage 4 production callbacks/control reproduced STOP followed by lower movement in the same pass. Later pre-fix reproductions found Web dead-man expiry followed by renewal and Coordinator lease expiry followed by movement.

The active environmental scan regression exposed a second clock issue after the initial accounting patch: the safety dispatcher resumed scanning, then immediately paused the policy again. Temporary tracing existed only in generated native translation units. The final regression asserts hazard wait exclusion, fresh scan time accumulation and watchdog HOLD during scanning.

The Coordinator gateway fixture initially omitted required PIKU constants; its compile error was a fixture error, not a runtime reproduction. The corrected fixture was replayed against immutable approved 7112c24d production objects without altering a checkout: application-evidence admission, STOP overwrite and cached movement through Robot reboot each failed runtime assertions. Current code passes. This later baseline replay is not represented as an original pre-patch runtime run. Standalone's actual callbacks independently exercise its STOP/RELEASE/mode fence.

## D. NOT REPRODUCED / preserved behavior

AUTO STOP already terminates recovery and requires explicit mode rearm. Critical safety already beats pending Forward; sonar invalidation beats pending turn completion. New hazard restarts stable-clear waiting; mode exit cancels context; boot objects start stopped. Existing ownership tests retain competing-owner exclusion and stale browser RELEASE protection. No alternate product behavior was introduced for these passing baseline cases.

No second dataset ring, Coordinator recovery brain, duplicate transport Override policy or NTP job-time mutation was found. Background architecture is source/native/browser tested; actual STOP latency is unmeasured.

## E. Implementation and actual state map

Robot remains the sole motor/navigation/safety brain. Wi-Fi browser control reaches Robot directly. Selected nRF browser control uses Coordinator's transport gateway. Independent telemetry selection never switches movement authority.

MANUAL idle/head inspection/moving states retain directional gates, ownership and leases. Ordinary STOP/RELEASE stop motion and preserve eligible saved Override permission; every hazardous movement needs fresh live authority. AUTO front settle/sample, bounded Forward, ordinary reverse/L/F/R/35-degree turn, InvalidStop/retry and front revalidation remain.

V3.3 retains episode scan, committed side, started 25-degree turns, front/side rechecks, capped backoff, provisional handoff and 5000 ms trusted forward proof. Six turns and five 200 ms pulses span pauses/retries; backoff remains min(selected speed,75%). Ordinary Manual backward remains intentional/operator-supervised.

HazardActive -> continuous clear wait -> fresh EnvironmentalRecovery L/F/R -> trusted decision/completion. SonarReacquire cancels execution and requires two new trusted front samples. CriticalAbort and the existing outer critical rearm latch remain higher authority. No separate operator Global E-stop was found. AUTO operator STOP and critical rearm remain distinct latches.

Stage 4 retains its existing temporary 2000 ms known-Coordinator-loss hold. Local AUTO does not permanently depend on Coordinator. Incomplete environmental or sonar work resumes its own sensing path. The clock accumulates active work with wrap-safe uint32_t subtraction/saturation, excludes waiting/cooldown/provisional normal-forward validation, and preserves spent time. A per-outer-loop fence protects a winning STOP/HOLD from lower movement later in the same pass.

## F. State x interruption x transport/background matrix

Each row explicitly answers Q1-7 and Q10-11. Shared Q8-9 and Q12-16 follow the table, with row exceptions. Local AUTO applies to both Robots independently of selected telemetry/control link; integration rows are CPS only.

| ID / pre-state Q1 | Event/routes/background Q2 | Immediate motor / authority Q3-4 | New state Q5 | Preserve / reset Q6-7 | Exact resume / fresh validation Q10-11 | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| E1 environmental scan/decision | sonar fault/stale, AUTO, polling | STOP / sensor validity | reacquire; environmental incomplete | intent/budgets / all old L/F/R and gyro/pulse execution | two new trusted front samples then all-new L/F/R; no Forward before trusted completion | REGRESSION-TESTED both |
| E2 environmental scan/decision | integration loss/restore, CPS | STOP / Robot authorization | hold; job incomplete | intent/used time / scan/execution | fresh L/F/R after authorization; reacquire first if sonar still faulty | REGRESSION-TESTED CPS |
| E3 environmental job | directional IR or new hazard | STOP / directional/environmental safety | fresh environmental scan or HazardActive | job/side/budgets / unsafe execution and scan | IR cannot substitute ordinary reverse; recurring hazard repeats stable-clear wait and scan | REGRESSION-TESTED both |
| R1 recovery turn/backoff/front/side | sonar/hazard/integration | STOP / highest active Robot gate | stopped same episode | counts/side/spent time / execution and stale samples | same episode checkpoint or full environmental scan by retained context | REGRESSION-TESTED, parity |
| R2 TrappedHold/cooldown | repeated sonar/hazard/integration | STOP / stronger hold | policy remains TrappedHold | original deadline/counts/time / stale execution | other holds clear; environmental scan completes stationary; original 2500 ms deadline remains | REGRESSION-TESTED |
| R3 active recovery work | accumulated 30000 ms watchdog | STOP / bounded policy | TrappedHold | spent time/counts/side / motion step | fresh sensing still uses spent budget; environmental evidence may finish stationary, no new recovery movement | REGRESSION-TESTED |
| A1 normal front/forward/obstacle/reverse/scan/turn/invalid retry | STOP/mode/hazard/critical/sonar, AUTO | STOP / operator and Robot safety | latch/mode exit/hazard/critical/reacquire | context for temporary hold / terminal episode reset | AUTO STOP needs mode command; temporary resume needs fresh sensing, never old turn/reverse | CODE-REVIEWED, REGRESSION-TESTED |
| M1 MANUAL idle/moving, Override off/on | STOP/RELEASE/speed, Web/RF/poll | STOP wins; speed preference only / Robot | idle stopped; no ordinary Manual latch | eligible permission / held motion | fresh direction next pass; current temperature and directional gates apply | REGRESSION-TESTED, CODE-REVIEWED |
| M2 held MANUAL movement | command-session/lease loss, reconnect | STOP / owner/dead-man | no live held authority | permission/preference / held command | reconnect alone never moves; fresh accepted direction/current live authority | REGRESSION-TESTED, browser |
| O1 Web or RF owner | competing owner, stale RELEASE, link/queued action | current owner or transition STOP / arbitration | retained owner or stopped transition | independent Web authority on RF reboot / obsolete action | official browser cancels held/speed work, release/stop before switching; new link needs fresh request | REGRESSION-TESTED, BROWSER-TESTED |
| RF1 accepted epoch | duplicate/reordered/stale packet | no new authority / progression | previous epoch or candidate | accepted timestamp / rejected action | valid current epoch cancels candidate; two progressing plausible early-boot packets confirm reboot | REGRESSION-TESTED |
| RF2 moving/candidate | STOP, confirmed reboot, old packet | STOP / fail-safe and epoch fence | idle, old RF ordering fenced | Web/local AUTO/preferences / pending non-STOP, applied sequence, RF lease | fresh accepted application evidence and legitimate new command; no cached gateway renewal | REGRESSION-TESTED |
| RF3 long stale | delayed lower uptime >10000 ms or half-range gap | no authority / conservative rejection | stale unavailable | replay fence / ambiguous candidate | no speculative stale resync; limitation retained | REGRESSION-TESTED, PHYSICAL-PENDING |
| B1 reset | Robot or Coordinator reboot | Robot startup STOP / initialization | empty runtime ownership/recovery | provisioned settings / volatile motion, lease, Override, execution | fresh mode/command/sensors; accepted Robot Status for selected nRF readiness | CODE-REVIEWED, host boot |
| BG1 History/CSV/time/TFT/SPI/telemetry | STOP/speed/RF/time upgrade | control before optional Robot work / Robot | normal service, immutable job time | captured TimeSnapshot/one ring / no history-time rewrite | captured anchor remains; telemetry fallback cannot authorize motion or change Control Link | REGRESSION-TESTED, CODE-REVIEWED, BROWSER-TESTED |

Q8: episode rows retain six turns/five pulses/used active time; waiting and cooldown do not reset accounting. AUTO STOP/mode/reset retain terminal semantics. Manual/transport-only states do not own recovery budgets.
Q9: interrupted gyro/pulse/scan execution is discarded; covered rejected/duplicate/reordered RF actions cannot replace accepted ordering. Universal authenticated replay protection is not proven (S).
Q12: no early retry, extra turns/pulses, critical/sensor bypass or lower same-pass move after winning stop. A renewal arriving before expiry is adjudicated may extend an existing valid lease; expiry race tests cover a stop already won.
Q13: shared policies and navigation are exact after only declared CPS seams; integration differences are deliberate.
Q14: Web/RF single owner and source-qualified RELEASE remain; RF reboot clears RF-owned movement but preserves independent Web/local AUTO.
Q15: hardware ACK proves transport only; accepted progression proves application evidence. A candidate never refreshes accepted time; Coordinator suppresses movement readiness while candidate pending.
Q16: Robot STOP callback stops motor before response/background work; optional work is skipped during control. Coordinator retains existing task isolation, short record-copy locks and one ring; SPI initialization has one owner. Physical scheduling bounds remain pending.

## G. Nested interruptions

Production navigation exercises environmental -> sonar -> integration and environmental -> integration -> sonar with fresh post-restoration L/F/R; repeated sonar; recovery -> hazard -> sonar; and TrappedHold -> hazard -> integration with original cooldown and stationary environmental completion. No Forward occurs during restarted scan and no turn/pulse budget is duplicated.

RF candidate -> STOP -> old packet and candidate -> valid current epoch remain in progression/radio/control tests. Mode/STOP/lower movement and RELEASE/new owner use the pass fence. Sonar restoration alone cannot clear a current hazard. CODE-REVIEWED, REGRESSION-TESTED, CROSS-REPO-PARITY-TESTED.

## H. Same-loop findings

Standalone actual callbacks and Stage 4 callbacks/control bodies exercise STOP then direction, mode then direction, Web expiry then renewal, RF expiry then movement, RELEASE then new owner, hazard then Forward and critical then Backward. Coordinator controller tests STOP and lease expiry then direction. Navigation tests sonar versus turn and critical versus pending front result.

STOP plus speed uses production motor preference/cap tests and source ordering: stopped speed adjustment commands no motion; RF pending STOP cannot be overwritten by speed in its receive batch. STOP plus mode may update selection but cannot move in that pass; explicit mode counter controls AUTO rearm. Browser link switch cancels queued held/speed actions, stops/releases before transition and retains request fences. Authorization is checked before navigation dispatch and sonar handoff; denial cancels current execution. Structural conclusions are CODE-REVIEWED, not physical simultaneous-event injection.

## I. Invariants

Environmental work stays incomplete across temporary holds and all restored decision samples postdate clearance. TrappedHold retains its original deadline. Active time accumulates while waits do not: 18 s active + 2.5 s cooldown leaves 12 s; 12 s active + 20 s sonar hold leaves 18 s. Policy regressions cover multiple pauses, near-threshold pause, threshold, retry, wrap, handoff, reset and repeated hold.

Manual STOP/RELEASE permission and AUTO STOP latch stay distinct. Six turns/five pulses, 25/35-degree targets, sensor thresholds, lease/cooldown constants, intentional reverse, selected-link architecture and packet layouts are unchanged. No RTOS task, sensor, extra dataset ring, framework or blocking wait was added.

## J. Parity

Stage 4 v33_core_parity.json pins sixteen pure-core files, constants and projected navigation/header. Only explicitly reviewed CPS seams are stripped before exact hashes are checked against Standalone. Stage 4 and Coordinator RadioStatus.h are byte-identical. Protocol checks retain schema 1, 12-byte Command, 30-byte Status, Release=10, SonarHold=5, channel/addresses/data rate and PA LOW. CROSS-REPO-PARITY-TESTED.

## K-N. Results by subsystem

K: one Robot movement owner and Override policy; no silent control fallback. Saved permission survives eligible loss but hazardous motion needs live authority.
L: sonar/integration cannot skip incomplete environmental recovery; fresh full L/F/R precedes existing decision/completion.
M: accumulated clock survives retries/holds; original cooldown survives interruptions. Active scans count, waiting does not. Environmental stationary evidence remains required if watchdog enters HOLD.
N: single-packet obvious reboot acceptance removed; two-packet plausibility strengthened; gateway cached RF movement cancelled on stale/uncertain Status/reboot. Delayed resync remains unavailable; absolute replay security is not claimed.

## O. Boot/reset

MotorController initializes PWM/direction stopped before communications. Ownership, held command, Override and recovery execution are runtime state, default empty. Coordinator starts inactive; initialization/ACK cannot admit movement without advancing Robot Status. Robot peer-reboot handling fences RF command ordering while preserving unrelated Web/local AUTO.

Constructors/boot/control/reset cases are REGRESSION-TESTED; startup ordering is CODE-REVIEWED; browser session behavior is BROWSER-TESTED. Actual Robot reboot during Web/RF/recovery, Coordinator reboot during Robot AUTO/RF Manual and physical reconnect remain PHYSICAL-PENDING.

## P. Background subsystem result

History/CSV capture one immutable TimeSnapshot per job. Native CSV receives NTP during export and retains its captured anchor; source priority/wrap tests pass. Existing worker separation/short record copies retain one ring. Bounded RF polling and existing serialized display/radio service retain DisplayManager as SPI initialization owner. Alarm precedes clock drawing. SNTP remains one eligible initialization without blocking wait/new task.

Actual STOP latency under CSV/History, TFT contention, radio retries and real Wi-Fi polling is unmeasured. Software architecture is CODE-REVIEWED/REGRESSION-TESTED/BROWSER-TESTED, not proof of real-time bounds.

## Q. Gates

Baseline gates passed: original five Standalone unittest cases plus policy/source checks; Stage 4 88; Coordinator 71. Final full suites: Standalone 37 with three CPS-only skips; Stage 4 115 with three gateway-only skips; Coordinator 78. Skips are non-applicable repository roles.

Standalone also passes twelve policy executables, three source/capture scripts and Deno embedded JavaScript parse. Both Robot browser gates render 66 control and 44 other-page cases per asset variant at six viewport widths; actual current Standalone assets are compared to the approved starting Git object. Coordinator retains 72 viewport and six resilience scenarios plus JS transport/source checks. BROWSER-TESTED using fixture responses; no hardware endpoint.

All three PlatformIO esp32-s3-n16r8 builds PASS, git diff --check PASS and parity/protocol checks PASS. Incremental firmware build logs emitted no warnings. Host hardware doubles test current production logic; physical timing/displacement remain pending.

## R. Resources, bytes

| Repository | RAM before -> after | Flash before -> after | RAM / flash delta |
| --- | --- | --- | --- |
| Standalone | 58892 -> 58908 | 1040653 -> 1041545 | +16 / +892 |
| Stage 4 | 54484 -> 54492 | 1045121 -> 1046345 | +8 / +1224 |
| Coordinator | 87184 -> 87184 | 1031869 -> 1032133 | 0 / +264 |

BUILD-TESTED static linker use, not peak stack/heap. Dataset layout/capacity unchanged.

## S. Remaining limitations

Delayed reboot beyond 10000 ms and long outage with 16-bit half-range ambiguity can leave nRF unavailable. No speculative stale resync or boot/session schema change was added. Quick reboot is two-packet plausibility, not authentication: arbitrary recorded traffic cannot be universally distinguished from a new boot without authenticated session identity. Covered old-epoch/duplicate/reordered packets are rejected; universal replay security is not proven.

Reboot detection requires application evidence or expiry. Requests lack authenticated physical-press identity; new held renewals and delayed current-session requests are not universally distinguishable across an unobserved reboot. Existing request/lease fences and browser held-state cancellation remain. No backend Control-Link redesign was added.

No rear sensor, partial IR, no-echo ambiguity, uncalibrated MQ-2, gyro drift/fallback, network scheduling and RF delivery remain prototype limits. Named tests/source reasoning are not exhaustive formal verification or safety certification.

## T. Physical requirements

Physical validation NOT RUN. RF PA/range engineering NOT RUN. Existing physical checklists remain unchanged/unchecked for this candidate.

Supervised checks must cover interrupted/nested environmental scans, sample freshness, cooldown restoration before/after expiry, six turns/five capped pulses, accumulated watchdog, STOP/speed/link/ownership races under polling/export, Robot/Coordinator reboot, Override/critical/directional vetoes, shared SPI and delayed/outage availability. Instrument latency, displacement, angle error, RF delivery/range before publishing measurements.

## U. Final local branches and identities

| Repository | Final branch | Final commit identity |
| --- | --- | --- |
| Standalone | codex/reva-state-collision-20261005 | creating commit of this report in codex/reva-state-collision-20261005 |
| Stage 4 | codex/stage4-state-collision-20261005 | creating commit of this report in codex/stage4-state-collision-20261005 |
| Coordinator | codex/nodev1-state-collision-20261005 | creating commit of this report in codex/nodev1-state-collision-20261005 |
| Frozen Node | main | 70ef949ce18ab81cbc207fed13bcc320d8b99964 |

A report cannot embed its own creating commit SHA. An earlier commit also cannot embed later dependent SHAs with exactly one commit per repository. Resolve creating-commit identities using git log -1 --format=%H -- docs/SYSTEM_STATE_COLLISION_VERIFICATION.md in the indicated repo. The owner handoff and immutable Research Continuity receipt record all three exact resolved SHAs; Stage 4's parity manifest pins the resolved Standalone authority.

## V. Final status and protection

One focused local commit per editable repository. Standalone retains exactly four pre-existing untracked owner PNGs. Stage 4/Coordinator clean after commit. Frozen retains main/70ef949, modified .vscode/extensions.json and untracked AGENTS.md. Protected byte hashes, frozen HEAD/branch/status, configuration/packet hashes and explicit staging inventory are rechecked at handoff; no secret values displayed.

Nothing pushed, merged, tagged, released, flashed or uploaded. No remote configuration change, reset/clean/stash, owner-file move/rename/staging, physical validation or RF PA/range engineering.

## SYSTEM ACTUAL BEHAVIOR / RESEARCH / USABILITY

Software demonstrates local Robot sensing/safety, bounded reactive AUTO recovery, intentional supervised MANUAL/Override, selected transport control, independent telemetry and Coordinator monitoring/history/export with job-time consistency. Coordinator supplies visibility, transport, data and dashboard; Robot chooses navigation and applies motor safety. Wi-Fi is browser-to-Robot; nRF is browser-to-Coordinator-to-Robot. Ordinary telemetry failure does not take local AUTO authority away, subject to sensing/STOP and existing temporary integration boundary.

Faulty sonar stops AUTO for sensing; MANUAL retains approved supervised unknown-sonar behavior. Environmental hazards require stable clear and fresh scans; temperature critical cannot be overridden. Lost held authority stops Manual; ACK alone cannot make selected nRF ready; ambiguous epochs may remain unavailable. Maps/localization/odometry/SLAM, autonomous swarm planning, industrial hazard certification and automatic control fallback are future work, not demonstrated claims.

A paper/demo can state architecture and named software-tested invariants with evidence grades. Historical owner physical results remain attached to their earlier exact baselines. Current recovery success, stopping distance/latency, RF range/reliability, electrical performance and hardware usability require new physical evidence.
