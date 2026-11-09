# Standalone reliability and dashboard candidate - 2026-10-18

**FIRMWARE + DASHBOARD CANDIDATE READY FOR INDEPENDENT REVIEW**

Phases A-C and E-G are complete. Phase D's unmeasured scheduling, sensor, motor
and networking optimizations were skipped. This is a software-verified candidate,
ready for independent review and then supervised physical A/B evaluation. It has
not been flashed or physically validated. Stage 4 migration remains PAUSED.

## A. Source preservation

Only target: `Research/Meo/The PIKU/PIKU 2.1 Rev A/PIKU 2.1 Rev A Software Development`.
Branch `codex/reva-collision-closure-20261005`; HEAD
`fa375a7bf739fc306060048b500c2acf413db7a7` remains unchanged.
Frozen annotated tag `revA-standalone-v32-restored-physical-validated-20261008`
and tag object `a9063a409ace73c1d2abeb7fa2c09e271f639455` are unchanged.

The fresh pre-edit 48-file non-secret production/configuration fingerprint matched
`3c27ffb727cd1577ca9ba70ae01f20762bd094c88df7753cdb9c1cb5f49aeef3`.
The index was empty. Original dirty inventory: two deleted tracked architecture
figures and 37 untracked entries, including documentation, figures and existing
Python caches. Those original bytes and absent paths remain as found; the checkout
is still dirty.

One private archive was created outside all repositories:
`Research/Continuity/preservation/PIKU_RevA_PreReliability_20261018_062543.zip`.
SHA-256: `d8cb1e1344f94fd3afb691d4b09e0c518d996c0ed62ac6c07cfb2824d167199a`.
All 168 archived working-tree files matched recorded raw hashes and byte sizes.
The embedded recovery metadata preserves original status, paths and Git identity.
ACL inheritance is disabled; current user and SYSTEM have access. Private
configuration, Git internals, build output and Python caches were excluded.
Local private configuration remains byte-identical and is not included here.

The earlier Downloads reference has no Git metadata; none is invented. Its
captured source files remain unchanged. Captured file hashes, branches, HEADs,
staging and porcelain status also remain unchanged in PIKU Stage 4, Coordinator
Stage 4, Node V1 standalone and PIKU Physical Development.

Historical frozen physical status remains **OWNER-REPORTED PHYSICAL PASS**.
The owner's newer observation that the earlier GitHub firmware performs better is
also retained as owner evidence. Neither establishes measured latency or an
independently verified flashed binary identity.

## B. Firmware corrections and diagnostic visibility

### Phase A: trusted scan timestamp

`NavigationManager::recordScanReading()` preserves the newest trusted timestamp
when a synthetic timeout follows useful readings. A finite positive valid distance
or accepted ClearNoEcho reading can update provenance; an invalid reading cannot.
Trusted-count initialization permits a real timestamp zero, and signed 32-bit
modular ordering handles millis wrap. `scanSampleTrusted()` retains completed
aggregation, quorum, age and result checks without rejecting numeric zero alone.

The original source failed five of the first 18 new live-source scan cases:
two-good/final-timeout, OPEN quorum/final-timeout, actual timeout path, clock wrap
and older trusted arrival. All now pass. The final fixture has 19 cases including
forward-untrusted diagnostics. Existing request generation, orientation, age,
late completion, sensor-fault precedence and environmental ownership remain.

### Phase B: bounded STOP and mode lifecycle

The owner explicitly authorized a minimal backend freshness/transaction fence on
the existing endpoints after the delayed-AUTO-after-newer-STOP hazard was
reproduced in a host interleaving. Browser abort alone cannot cancel a transmitted
request. This authorization resolves the original contract's protocol restriction.

`ModeTransitionFence.h` adds a fixed-size, one-use, 2,000 ms ticket with boot nonce
and revision. An accepted STOP revokes the previous ticket; after the existing
synchronous STOP callback, its OK response issues a fresh ticket. Accepted RELEASE
and applied safety/deadman/interlock STOP reasons revoke tickets. A newer STOP
therefore rejects an older pending AUTO transaction. Parsing is capped at ten
decimal digits per component with overflow checks; age arithmetic handles wrap.

Existing mode paths remain. Compatibility change: clients must first obtain
`X-PIKU-Mode-Fence: boot.revision` from successful STOP, then send it as `fence`
to `/mode?set=AUTO`, `/mode?set=MANUAL` or existing mode aliases before expiry.
Missing, expired, consumed, revoked or previous-boot tickets receive HTTP 409.
STOP requires no ticket; its original stale-sequence fail-safe acceptance remains.
This ticket is ordering/freshness protection, not authentication or a physical
motor-stop sensor. The updated embedded dashboard supplies it automatically;
cached old dashboards and custom mode clients must reload/update.

Browser `setMode()`, `boundedRequest()`, `issueCmd()`, `flushMoveCommand()`,
`explicitStop()`, `updateModeButtons()` and `loadData()` now provide:

- STOP deadline 1,800 ms, mode deadline 1,500 ms, movement deadline 600 ms.
  These are application bounds, not measured network guarantees. The original
  manual deadman remains 750 ms. Existing ping/data cadence and limits remain.
- All pending raw STOP/RELEASE promises must acknowledge before mode submission.
  A failed, aborted, rejected or never-resolving STOP cannot release that barrier.
- Only one mode transaction; repeated clicks do not queue future mode changes.
- Failure retires held/pending movement, inhibits controls and submits STOP.
  Current telemetry plus a fresh operator mode action is needed for recovery.
- Late responses cannot revive obsolete intent. Telemetry generations are retired
  both by STOP and by successful mode acknowledgment; the latter fixes a separately
  reproduced stale-telemetry UI race.
- Explicit progress, acknowledgment and uncertainty messages; STOP remains enabled.
  Firmware acknowledgment is explicitly distinguished from physical stopping.

`handleCommand()` and `applyMode()` remain exact frozen bodies.
`applyCommand()` retains its original validation and authority with only the
four-line STOP/RELEASE fence addition. The real main callback still stops PWM
before response formatting, logging or background work. Same-pass fences remain.

### Phase C: useful bounded evidence

Existing STA status, disconnect count/reason, reconnect attempts, RSSI, uptime,
reset reason, available/minimum heap and maximum service-loop gap are retained.
Browser fixed-size request statistics now separate ping, data, direction, STOP
(including RELEASE) and mode: successes, failures/aborts, timeouts, last RTT and
outcome. Diagnostics are updated from existing requests; no extra polling exists.

Additive `navigationDiagnostics` JSON contains `scans`, `scanTimeouts`,
`scanRejected`, `forwardUntrustedStops`, `sonarReacquisitions` and
`lastScanRejection`. Five uint32 counters and a static literal reason add no
navigation-loop allocation, sensor acquisition, blocking calls or log streams.
`beginScan()`, `updateScan()`, `updateForward()` and
`beginSonarReacquire()` only gain counters/reasons. Publisher comparison includes
these fields. Existing JSON keys/readers remain; the System panel displays them.

| Observed evidence | Supported interpretation |
|---|---|
| STA disconnected plus reason/count change | Association loss recorded by firmware |
| Uptime restarts / reset reason changes | Reboot evidence; brownout only if supported by reset/power evidence |
| STA connected while ping/data/control stall | HTTP/service/network path delay; cause still needs measurement |
| HTTP succeeds while heartbeat badge is Offline | Browser heartbeat classification differs from HTTP availability |

An Offline badge keeps its original heartbeat meaning and thresholds. Failed
requests and intentional aborts do not prove STA loss. No home-router reliability
improvement is claimed.

### Phase D: optimization decision and retained safety

Reachable DHT acquisition, interrupt masking during sonar, synchronous IMU/I2C,
HTTP parsing/service, JSON construction and additional control-active sensing are
possible timing costs. Their physical contribution was not measured. No scheduling,
sensor, I2C, HTTP, Wi-Fi, motor PWM or calibration policy was changed to guess at a
cause. Additional strict forward sonar checks remain safety responsibility.

Robot retains navigation/motor/safety authority. Ordinary STOP/reverse/fresh L-F-R
scan/turn/front-validation behavior remains. Normal finite obstacle threshold is
20 cm, ordinary finite side eligibility >20 cm, recovery veto <=7 cm and boxed
finite admission <30 cm with >=30 cm/accepted OPEN exit. Approximate 25-degree
increments, seven-turn/five-pulse/30-second budgets and cooldown/retries remain.
Hazard ownership, sonar fault/reacquisition, IR guards, STOP and override rules remain.

## C. Dashboard typography: implemented

`src/WebDashboardAssets.cpp` contains the actual HTML/CSS/JavaScript changes.
Dark/cyan theme, all five sections, original DOM IDs, control/navigation listeners,
sensor values and radar behavior remain. New diagnostic/status elements are additive.

Computed CSS pixels at 390 px, default text size (actual Chromium comparison):

| Selector(s) | Original | Candidate |
|---|---:|---:|
| .metric-label, .metric-note | 12 | 15 |
| .metric-value | 22 | 26 (up to 34 on desktop) |
| .mini-metric span, .detail-item span | 11 | 14 |
| .mini-metric strong, .detail-item strong | 14 | 18 |
| .control-readouts span, .system-summary span | 11 | 14 |
| .status-badge | 10 | 14 |
| .panel-badge | 12 | 15 |
| .alarm-label / .alarm-detail | 11 / 11 | 16 / 15 |
| .control-note | 12 | 16 |
| .sensor-head-btn, .sensor-head-status, .sensor-head-availability | 11 | 15 |
| .nav-btn | 9 | 14 |
| .dpad button | 12 | 16 |
| .eyebrow | 11 | 14 |
| .wifi-form label, .system-note | 11 | 15 |

Body is 16-18 px via relative units; system-ui/Segoe UI/Arial fonts, stronger
weights, tabular numbers, increased line height and wrapping improve hierarchy.
Muted text is brighter (#b7c8d8). On the sampled solid panel, its contrast increases
from about 6.99:1 to 10.15:1. White STOP text over the darkened gradient is about
5.99:1-8.05:1; active-mode contrast about 6.42:1. These sampled calculations are
not a complete accessibility certification; normal-text guidance is 4.5:1
([W3C contrast minimum](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html)).

Mobile navigation uses two rows with the Communication label readable, touch
targets >=44 px, adequate bottom clearance, and a visible mode badge. Short
landscape header/navigation positioning avoids STOP occlusion. STOP remains
accessible within Control by scrolling, rather than being always on screen.
Main values' line height was corrected after a real 200% text overflow was found.

Radar canvas fonts scale to approximately 14 CSS px for range labels and 16 px for
L/F/R. Staggered label placement keeps all seven labels within bounds. Detection
geometry, distance mapping, sweep and update cadence are unchanged.

All five panels passed at widths 320, 360, 390, 430, 768, 1024, 1440 and 1920;
landscapes 740x320, 800x360, 844x390 and 932x430; 200% root text at 320 and 1440;
and 125% CSS zoom at 1440. These 75 combinations checked horizontal/vertical text
bounds, metric overlap, minimum fonts, navigation targets and STOP center hit-test.
No real-phone browser-zoom equivalence is claimed.

Actual Chromium captures with synthetic telemetry, not physical robot evidence:

- [Original Control, 390 px](../.pio/reliability-candidate-evidence/baseline-control-390.png)
- [Candidate Control, 390 px](../.pio/reliability-candidate-evidence/candidate-control-390.png)
- [Original STOP](../.pio/reliability-candidate-evidence/baseline-stop-390.png)
- [Candidate STOP](../.pio/reliability-candidate-evidence/candidate-stop-390.png)
- [Candidate desktop overview](../.pio/reliability-candidate-evidence/candidate-overview-1440.png)

## D. Software verification and independent self-review

| Fresh execution | Pre-edit baseline | Final candidate |
|---|---:|---:|
| Native C++ policy executables | 12 passed | 13 passed |
| Python suite | 172 run: 168 passed, 4 CPS skips | 206 run: 195 passed, 11 skips |
| Failures / errors | 0 / 0 | 0 / 0 |
| Chronological scenarios | 34 matched | 34 identical summaries |
| Modeled control passes | 3,161 | 3,161 |
| Candidate actual-browser transaction scenarios | N/A | 21 passed |
| Candidate panel/viewport combinations | N/A | 75 passed |
| ESP32-S3 PlatformIO build | PASS | PASS |
| Static RAM | 58,908 bytes | 58,996 bytes (+88) |
| Flash | 1,042,541 bytes | 1,055,193 bytes (+12,652) |

Commands: `python -B tools/verify_reliability_candidate.py` and
`pio run -e esp32-s3-n16r8`. The runner writes its own test log; redirect console
output to a different file. The initially conflicting console redirection was
corrected and the complete run then passed. Compiler/browser caches and generated
test evidence stay in ignored .pio. A legacy restoration summary write is redirected
in memory into .pio; historical documentation is not overwritten.

The 11 skips are FOUR existing CPS-only cases plus SEVEN explicit substitutions:
five historical frozen/source-identity or unchanged-backend checks and two
pixel/style-equality checks are inapplicable to the authorized source/type changes.
Their assertions and historical manifests remain intact. The new exact candidate
manifest, source-body invariants, live handler tests and computed rendering checks
replace them. Exact names/reasons are in `tools/verify_reliability_candidate.py`
and the results JSON. This is not an unqualified run of the original suite.
Historical static projections recognize only exact reviewed candidate hashes;
runtime fixtures execute actual candidate source. Unknown deltas still fail.

Final actual source, changed callers and interruptions were reread. Existing
collision suites plus the new fixtures support the following software-only review:

| State x event x path/background | Result reviewed/tested |
|---|---|
| AUTO/scan/recovery x STOP x HTTP/sensor work | Same-pass stop; old completion does not resume motion |
| Scan x timeout/late/wrong generation/orientation x sensor completion | No new trusted evidence; usable trusted timestamp preserved |
| Scan/recovery x critical/environment hazard x background sensing | Existing hazard ownership and motor fencing retained |
| MANUAL held motion x mode/RELEASE/deadman x pending direction | Old intent retired; stale direction rejected; deadman retained |
| Waiting STOP x failure/timeout/abort x mode click/reconnect | No automatic mode submission; visible inhibited state |
| Sent AUTO x newer STOP/RELEASE/safety x late mode arrival | Old fence revoked; live handler rejects request |
| Mode acknowledgment x older telemetry completion | Obsolete UI mode ignored by generation fence |
| Sonar fault/reacquisition x STOP/hazard x fresh/late sensor result | Original gates/ownership retained |
| STA/HTTP delay x heartbeat/data x background service | Evidence classified separately; physical root cause unresolved |

The 34 trace summaries are byte-equivalent as parsed records, including motor,
sonar and servo counts. The timestamp correction intentionally changes the
previously defective scan cases; the new mode fence intentionally changes stale
or unfenced mode admission. Other normal navigation behavior remains modeled parity.
Host tests are not physical latency, RF, electrical, range or motion validation.

Final 49-file non-secret src/include plus platformio.ini digest:
`ea7c675b6f7d67d20e50c0ce8252f274a72158fbbb121600ab7c591fedaf928e`.
Method: SHA-256 of compact sorted JSON mapping slash-normalized relative paths to
raw-file SHA-256, excluding Secrets.h. Exact six-file candidate hashes are in
`tests/reliability_candidate_review.json`.

Local evidence is in `.pio/reliability-candidate-evidence/`: baseline/candidate
results, logs, original red scan probe, late-mode probe, browser measurements,
trace summaries and final preservation/status records.
Build logs: `.pio/reliability-baseline-build.log`,
`.pio/reliability-candidate-final-build.log`.

## E. Research evidence and supervised owner A/B plan

Useful supported claims: trusted timeout aggregation defect corrected; stale mode
admission fenced; bounded browser transitions fail closed; additional diagnostic
evidence available; larger fonts verified in actual Chromium; unchanged modeled
navigation traces. These do not establish physically better Wi-Fi or navigation.

Use the earlier GitHub firmware as the owner's stronger physical benchmark.
After independent review, supervise A/B evaluation with the same hardware,
battery/load, router placement/distance, phone/browser, speed, servo alignment and
obstacle layout; counterbalance A/B order and record firmware provenance privately.

Collect actual STA loss/reason/count and reconnect attempts alongside uptime/reset,
RSSI, heap and maximum service gap. Separately record ping/data/cmd/STOP outcomes,
RTT distributions/timeouts, request-to-motor response and loaded power observations.
Software RTT does not establish motor-disable latency; use synchronized physical
or electrical measurement where needed.

Exercise held Manual, RELEASE, STOP, both mode directions, uncertain transition,
disconnect/reconnect with old requests, clear-front AUTO, front obstacle, single/
both IR, full scan, two-good/one-timeout, stuck history, boxed recovery, <=7 cm veto,
>=30 cm/OPEN recovery exit, cooldown and automatic retries. Record success,
interventions, hesitation, rejection counts and scan/reacquisition reasons.
Evaluate legibility and STOP use on real mobile/desktop devices at working distance.

Physical acceptance requires preserved safety behavior and no late automatic
restart after STOP/failed transition; benchmark latency/navigation outcomes and
home-router disconnects with comparable data. No numerical improvement threshold
is fabricated. If the candidate remains worse, preserve evidence and pursue the
specific correlated cause. Real DHT/sonar overlap, I2C/HTTP timing, electrical power,
browser/network behavior and motor response remain unmeasured.

Existing figures, research records and original tests/manifests are preserved.
No paper DOCX, Physical Development update, measured-performance figure, camera AI,
TinyML or validated distributed-CPS claim is produced. No flashing or motor operation
occurred. Stage 4 remains PAUSED.

## F. Final Git scope and continuity

Nine pre-existing files changed by this task:

- include/WebDashboard.h
- src/DashboardPublisher.cpp
- src/NavigationManager.cpp
- src/WebDashboard.cpp
- src/WebDashboardAssets.cpp
- tests/native_v46_backend.cpp (fixture declaration only)
- tests/v45_approved_delta.py (exact reviewed candidate projection)
- tests/v46_approved_delta.py (exact reviewed candidate projection)
- tests/v46_browser_race.js (successful STOP mock now supplies fence header)

Ten task-created paths remain untracked:

- include/ModeTransitionFence.h
- tests/mode_transition_fence.cpp
- tests/native_mode_fence.cpp
- tests/native_reliability_candidate.cpp
- tests/reliability_candidate_browser.js
- tests/reliability_candidate_review.json
- tests/reliability_candidate_review.py
- tests/test_reliability_candidate.py
- tools/verify_reliability_candidate.py
- docs/STANDALONE_RELIABILITY_CANDIDATE_20261018.md

Final inventory: nine modified tracked files, the same two owner-deleted tracked
figure paths, and 47 untracked entries (37 original plus ten task additions).
Index remains empty. No branch, commit, push, reset, clean, stash, rebase, history
rewrite or tag change occurred. No original captured file disappeared.
CRLF-aware diff whitespace check passed without configuration mutation.

Research Continuity continuation:
`Research/Continuity/missions/2026/mission-20261018-piku-reliability-authorized-continuation/receipt.md`.
The earlier Phase A paused receipt remains immutable and historical; this document
now reports the authorized final candidate rather than that earlier partial state.

## G. Exact-byte recovery and next step

Independent review is next, followed by supervised physical A/B testing if accepted.
The candidate is not approved for Stage 4 migration or a physical performance claim.

For recovery, first preserve any subsequent owner edits and verify the private
archive hash and embedded manifest. Restore only the nine explicit pre-existing
task-modified paths above from corresponding `working_tree/<path>` entries; compare
each raw SHA-256 and size to its manifest. Do not overlay the whole archive, restore
the two intentionally absent figures, or touch private configuration.

The ten task-added paths were absent at baseline: retain them as evidence outside
the checkout if an exact original inventory is required, then remove only those
explicit reviewed paths after owner authorization. Do not use unrestricted Git
rollback or move the frozen tag. Ignored test/build evidence can remain separate.

