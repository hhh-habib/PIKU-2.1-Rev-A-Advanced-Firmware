# PIKU Rev A final firmware comparison and conservative cleanup

Audit date: 2026-10-18. Local release preparation only; publication remains a separate decision. Executable firmware, configuration, tests, historical assertions, manifests and owner figures were retained. Cleanup removed only 24 backed-up generated Python cache files (180,919 bytes). README and SOFTWARE_EXPLANATION received current guidance above their preserved historical text.

Evidence labels: **SOURCE VERIFIED** means inspected local source or file identity; **TEST VERIFIED** means the stated software check passed; **OWNER PHYSICALLY OBSERVED / OWNER-REPORTED** means the owner's qualitative report; **NOT YET VERIFIED** identifies evidence this audit did not establish. None of these labels implies hardware certification.

## A. Project and Git identity

Editable root, relative to `E:/Development`: `Research/Meo/The PIKU/PIKU 2.1 Rev A/PIKU 2.1 Rev A Software Development`.

| Item | Verified identity |
| --- | --- |
| Branch | `codex/reva-collision-closure-20261005` |
| HEAD / frozen commit | `fa375a7bf739fc306060048b500c2acf413db7a7` |
| Frozen tag | `revA-standalone-v32-restored-physical-validated-20261008` |
| Annotated tag object | `a9063a409ace73c1d2abeb7fa2c09e271f639455` |
| Current non-secret production/configuration | 49 files: 16 CPP, 32 headers, `platformio.ini` |
| Current production fingerprint | `f979ca73108a358b06e3f2f733f97bcb9ecde6d7a69f7323cd5c9ec3251f34b6` |
| Read-only comparison | `C:/Users/User/Downloads/PIKU-2.1-Rev-A-earlier github code`; no Git metadata, so this is a supplied file snapshot, not an independently authenticated GitHub revision |

The fingerprint is SHA-256 of compact sorted JSON mapping every non-secret `src/`, `include/` file and `platformio.ini` to its raw-byte SHA-256. It identifies the newer working tree, not just HEAD. `include/Secrets.h` was excluded from that fingerprint and all archives; its local bytes were checked separately without disclosure.

Initial dirty state: nine tracked modifications, two tracked owner-figure deletions, and 51 untracked files, including 24 generated caches. The nine modifications were `include/WebDashboard.h`, `src/{DashboardPublisher,NavigationManager,WebDashboard,WebDashboardAssets}.cpp`, `tests/native_v46_backend.cpp`, `tests/v45_approved_delta.py`, `tests/v46_approved_delta.py`, and `tests/v46_browser_race.js`. They were preserved byte-for-byte. The index was not changed. Complete status, refs and individual non-secret hashes are in the private backup's `recovery-metadata.json`.

## B. Earlier GitHub snapshot versus current standalone

**SOURCE VERIFIED:** both production/configuration trees were read and compared, with CRLF/LF normalized only in the comparison. Of the 49 current production/configuration paths, 30 are text-identical to the reference, 18 differ, and `ModeTransitionFence.h` is additional. Raw-byte preservation uses raw hashes rather than normalized text. Git attributes/ignore and local editor configuration were also inventoried; `.gitattributes` preserves the approved freeze's exact bytes, and `.gitignore` keeps private configuration local.

The reference already has state-machine AUTO, V3.2 recovery, Manual deadman, priority HTTP, SoftAP/STA provisioning, sensors, OLED, radio and tracing. Those are retained capabilities, not newly invented features. Current source is a selective continuation, not a replacement architecture.

### Substantive differences and responsibilities

All listed paths are relative to the project. Costs below describe source operations and configured waits; no physical latency or isolated feature RAM/flash cost was measured. Removing these changes would either alter behavior or remove existing verification/interfaces, so none was removed.

| Exact file / function | Reachable caller, purpose and removal consequence | Runtime cost and existing verification |
| --- | --- | --- |
| `include/ManualOverridePolicy.h`: `tick()` | `main.cpp::serviceLoop()` retains saved flame/gas permission across session loss while `exemptsFlameGas()` still needs a live session. Earlier code revoked permission on loss. Removing this reinstates redundant permission re-entry. | Constant comparisons/state; Manual policy/native/session tests. |
| `include/MotorController.h`, `src/MotorController.cpp`: `backward()`, `backwardWithSpeedCap()`, `selectedDuty()` | Manual and navigation call `backward()`, which calls the retained cap API with 100. Cap state affects repeated reverse/speed changes. Tests explicitly invoke lower caps. Removing it affects a tested API; current navigation does not call it with 75. | One integer field, min/constrain arithmetic; `native_motor_backoff.cpp`, `test_v331_motor.py`. No change to PWM calibration, direction pins or turn minimum. |
| `include/RobotConfig.h`: `RECOVERY_BACKOFF_MAX_SPEED_PERCENT` | Retained 75 constant is used by motor tests; the restored navigation does not apply it. The seven-turn/30-second/five-pulse constants already match the supplied reference. | Compile-time constant; no independent runtime task. Preserve test dependency and disclose inactive navigation cap. |
| `include/RecoveryIrBackoffPolicy.h`: `cancelPulse()` | Retained compatibility interface, called by `recovery_episode_v33.cpp` and asserted in historical source checks; current navigation uses mission reset on interruption. Removing it loses reproducibility/interface coverage. | Inline event reset when called; no periodic work added. |
| `include/StuckRecoveryV2Policy.h`: `episodeActive()`, `episodeStartedMs()` | `NavigationManager::updateDiagnostics()` exposes current reference-policy state. These accessors do not add the historical V4.5 direction ledger. | Constant state reads; policy/native/navigation diagnostics coverage. |
| `include/V2EscapeDecision.h`: `recoveryPathSafe()`, `obviousBoxedIn()` | `selectRecoveryDirection()`, `finishRecoverySideCheck()`, `selectPathAfterScan()` reject non-finite/non-positive boxed evidence and unsafe pivots. Removing checks changes safety admission. | Bounded finite/range checks; pure policy, restoration, availability and scan regressions. |
| `include/SensorManager.h`, `src/SensorManager.cpp`: `begin()`, `updateControlSensors()`, `updateDht()`, `updateMq2()` | `serviceLoop()->sensors.update()` separates successful temperature evidence from attempts, tracks gas validity and prevents failed reads renewing evidence. Old values are retained without treating them as fresh. | A few fields and comparisons; existing 150 ms gas / 2000 ms DHT cadence, 16-sample gas filter. Producer and availability tests exercise real source bodies. |
| `include/SafetyManager.h`, `src/SafetyManager.cpp`: `update()`, `stateName()`, `alarmReason()`, `updateGas()`, `updateFlame()`, `updateTemperature()` | Service loop and immediate Manual callback classify monitoring availability separately, retain confirmed hazards across invalid/stale reads, and require real fresh clearing evidence. Removing this could silently clear hazards or misrepresent monitoring. | Constant validity/debounce work per serviced pass; producer, availability, environmental and dispatch tests. Unavailability alone remains monitoring-only. |
| `include/NavigationManager.h`, `src/NavigationManager.cpp`: `updateAuto()`, `selectPathAfterScan()`, `finishRecoveryAfterSettle()`, `completeEnvironmentalRecovery()` | AUTO environmental work holds on IR, invalidates the interrupted scan, rescans after clear, and requires new front proof after its turn; fresh boxed geometry can transfer to recovery. Earlier paths could discard environmental context or complete at the wrong checkpoint. | Existing state machine plus one flag and bounded checks; collision/native/restoration/IR tests. These are active safety paths. |
| `src/NavigationManager.cpp`: `notifyOperatorStop()`, `cancelForModeChange()`, `handleCriticalSafety()` | Operator/mode/safety callbacks retire gyro target and old turn state. Removing retirement risks stale actuator context. | Field resets; mode, STOP, collision and navigation tests. |
| `src/NavigationManager.cpp`: `requestedSample()`, `updateFrontValidation()`, `updateScan()`, `updateRecoveryFrontCheck()`, `updateForward()` | Scan/front callers validate requested sequence/time, age and facing; Forward additionally rejects untrusted current evidence. Signed modular request comparisons handle rollover. | Constant comparisons and counters; producer rollover/orientation/request-time, native scan and untrusted-forward tests. |
| `src/NavigationManager.cpp`: `recordScanReading()`, `scanSampleTrusted()` | Scan aggregation timestamps only trusted finite-positive or OPEN evidence; a synthetic timeout cannot overwrite the trusted timestamp. Real trusted zero/wrap timestamps are handled within aggregation. Quorum/fault/age gates remain. Removing this restores scan availability failures. | No new buffer or scan transaction; 19 candidate scan scenarios, including timeout placement, quorum, stale, wrap, fault and interruption. |
| `src/NavigationManager.cpp`: `beginSonarReacquire()`, `updateSonarReacquire()` | AUTO fault retires old scan/turn/backoff data; after two new trusted front samples, an incomplete environmental job restarts its full scan. The restored policy aborts its old V2 mission rather than preserving V4.5 ledgers. | Existing front-only cadence; counters and resets. Sonar, environmental-context and restoration tests. |
| `src/NavigationManager.cpp`: `observeIrEpisodes()` | `serviceLoop()` calls this intentionally empty retained interface; IR arbitration happens in `updateAuto()`. It is reachable, not unreachable code. | Empty body; keep rather than refactor the established service seam. |
| `include/WebDashboard.h`, `include/ModeTransitionFence.h`, `src/WebDashboard.cpp`: `acceptModeFence()`, `handleMode()`, `handleModeAuto/Manual()`, `applyCommand()`, `sendOk()`, `markCommandApplied()`, `invalidateControlSession()` | HTTP route callbacks enforce a boot-specific, one-use STOP-issued 2000 ms ticket on mode admission. New STOP/RELEASE, safety or session revocation invalidates old admission. This prevents delayed AUTO after a newer STOP. Removing it changes the owner-authorized stale-mode safety contract. | Fixed integer state, bounded decimal parser, headers on existing responses; 19 actual-handler scenarios plus pure fence and browser races. No new endpoint; API clients must supply the ticket. |
| `src/WebDashboard.cpp`: `handleSession()`, `sendOk()` | Session replacement applies RELEASE first and reports the advanced server sequence; existing OK acknowledgments report sequence and per-request Manual session state. Browser resynchronizes instead of issuing a stale first direction or assuming heartbeat proves authority. | Small JSON/header additions to existing transactions; live backend sequence, authority, replacement and multiple-client cases. |
| `src/WebDashboard.cpp`: `handleCommand()` | A declared Manual lifecycle retirement STOP arriving after leaving Manual is retired. Genuine untagged STOP remains fail-safe, including old sequence numbers. | One conditional on the command path; backend/browser retirement and explicit-STOP tests. |
| `src/WebDashboardAssets.cpp`: `setMode()`, `boundedRequest()`, `issueCmd()`, `flushMoveCommand()`, `sendCmd()`, `inhibitControl()`, `restoreManualReadiness()`, `ensureControlSession()`, `explicitStop()`, `lifecycleStop()`, telemetry rendering | Button/keyboard/pointer events enter bounded HTTP control. Three response slots permit held refresh without waiting for each response; only newest unsent intent is kept. STOP/release cancel old intent. Current acknowledged STOP/RELEASE plus matching authority clears uncertainty for a fresh press. Session RELEASE finishes before the first queued direction. Mode-generation guards retire late responses; heartbeat alone never unlocks uncertain control. Removing these corrections can reproduce sticky inhibition or delayed-mode races. | Browser-side promises/timers, at most three direction responses, one pending direction. Refresh 250 ms; response deadline 1800 ms; no additional healthy-path transaction. 55 live-asset Chromium scenarios cover 20/650/900/1200 ms response delays, cancellations, reordering, loss, STOP and mode/client races. |
| `src/WebDashboardAssets.cpp`: HTML/CSS, `drawRadar()`, request diagnostics, override labels | Existing panels/listeners retain larger responsive text, sharper radar labels, explicit request/heartbeat evidence and permission state. | Larger flash assets, local DOM/canvas rendering; 75 rendered layout cases, preserved HTML/CSS hashes. No control polling cadence change from the reference. |
| `src/DashboardPublisher.cpp`: `dashboardChanged()`; navigation `updateDiagnostics()`; dashboard `sendJson()` | Existing publish path now observes/adds scan rejection, timeout, untrusted Forward and reacquisition counters and recovery state. Removing it loses diagnostics, not a second motor controller. | Constant comparisons, a few counters/strings and additional telemetry bytes; source manifest, scan tests and browser rendering. |
| `src/main.cpp`: `onDashboardCommandAccepted()`, `onDashboardModeAccepted()`, `serviceLoop()`, `handleManualControl()`, `loop()`, `currentEnvironmentalAlarmMask()`, `getGasStatus()` | Immediate STOP sets a same-pass fence so later dispatch cannot move; ordinary STOP/release preserve eligible permission. DHT/MQ-2 and IMU acquisition no longer starve under control load; confirmed safety classifications drive alarms. Critical temperature retains explicit AUTO rearm. Removing these changes affects priority, monitoring or authority. | One pass flag; cadence-limited sensor/I2C work, existing immediate callback and telemetry. Dispatch/collision/availability tests and four extracted actual Manual/deadman timing cases. |

### Unchanged source responsibilities

The following complete files are text-identical to the supplied reference (raw line endings can differ). All remain useful or configuration/test dependencies; none was removed.

| Exact files | Reachable responsibility / cost / verification |
| --- | --- |
| `include/BuzzerManager.h`, `src/BuzzerManager.cpp` | `setup()/serviceLoop()` preserve alarm configuration and optional GPIO patterns. Installed profile disables the physical buzzer; browser audio is separate. Optional capability is not dead-code proof. |
| `include/DisplayManager.h`, `src/DisplayManager.cpp` | Publisher drives SH1106 when allowed, 250 ms refresh / 2500 ms pages; SPI transfer is cooperative synchronous work. Build/source preservation, not measured physical display latency. |
| `include/DriveCommandHandler.h`, `src/DriveCommandHandler.cpp` | `handleManualControl()` dispatches motors and recognizes held refresh. Native dispatch/browser sequence coverage. |
| `include/ImuManager.h`, `src/ImuManager.cpp` | `setup()/serviceLoop()` read MPU6050 at 25 ms cadence, finite checks and 250 ms freshness; relative gyro turns use it. I2C/library call time remains unmeasured. |
| `include/ServoScanner.h`, `src/ServoScanner.cpp` | Manual head and navigation own servo attach/angles/detach; existing timer/settle state and host traces retained. |
| `include/RadioManager.h`, `include/RadioTypes.h`, `src/RadioManager.cpp` | `setup()/serviceLoop()` retain 12-byte experimental radio heartbeat/status, up to four received packets per poll, PA LOW, 750/3000 ms heartbeat backoff. `RF24::write()` is synchronous. No standalone radio motor-command dispatch was found; packet enum names do not establish implemented remote driving. Build/source verified; physical RF reliability unmeasured. |
| `include/PriorityWebServer.h`, `src/PriorityWebServer.cpp`, `include/WebServerLimits.h` | `dashboard.loop()` serves one current HTTP client; parser limit 250 ms and telemetry write budget 125 ms. Non-telemetry writes delegate to WebServer; this is not an independent emergency-stop channel or hard response bound. Framework/library build preserved. |
| `include/ControlOriginPolicy.h` | All control endpoints check accepted local/coordinator origins; empty Origin remains permitted for non-browser clients. Policy tests pass. This is not HTTP authentication. |
| `include/ManualMotionGate.h` | Manual direction/safety/deadman/authority decisions and 20/25 cm front hysteresis; known IR/near turn gates remain. Pure policy/native dispatch coverage. |
| `include/SonarReacquirePolicy.h` | Two consecutive new trusted front samples; the 1000 ms window is diagnostic only. Pure policy/native tests; retained ordinary unsigned sequence comparison has no formal all-rollover proof. |
| `include/NavigationTrace.h`, `src/NavigationTrace.cpp`, `include/TraceRing.h`, `include/TraceChecksum.h` | `serviceLoop()->navTrace.service()` retains serial-only optional capture, fixed 16-record RAM ring, bounded serial chunks, checksum, no continuous flash logging. Trace/parser/native checks pass. The RAM buffer exists even while capture is off. |
| `include/StaReconnectPolicy.h`, `include/WiFiConfig.h` | Background STA backoff 1500/3000/6000/8000 ms, SoftAP retained, stored provisioning and special lab profile unchanged. Values remain private. No new reconnect policy or RF engineering. |
| `include/DashboardPublisher.h`, `include/WebDashboardAssets.h` | Existing publisher contract and flash asset declarations, build/render/native coverage. |
| `include/PinConfig.h`, `include/Secrets.example.h`, `platformio.ini` | Same pin map, private-configuration template and ESP32-S3 N16R8/Arduino/dependency settings. Build verified; pin/voltage/radio wiring requires physical evidence. Local `Secrets.h` excluded from comparison and unchanged. |

## C. Current capabilities retained

**SOURCE VERIFIED / TEST VERIFIED:** Manual once -> valid held refresh -> release -> next fresh direction remains the current browser/backend contract. STOP/deadman stop motion without selecting another Manual mode. Genuine uncertainty retires the old direction; acknowledgment and current authority are required for readiness. The browser never resumes the retired hold. Multiple-client cases verify guarded session recovery; ordinary hazard-free legacy API motion is not globally session-exclusive.

Ordinary AUTO first validates front evidence. Obstacles/IR use the restored bounded reverse, full L/F/R scan, eligible side and approximately 35-degree turn, then fresh front decision. Side eligibility in ordinary scans is above 20 cm; boxed recovery can use a committed side above the separate 7 cm hard veto. Trusted finite-positive complete L/F/R below 30 cm can admit recovery. Recovery allows seven started approximately 25-degree turns, five timed 200 ms IR pulses, and a 30-second elapsed mission bound; exhaustion stops and a 2500 ms cooldown permits a new scan/round. A fresh front at least 30 cm or trusted OPEN exits. There is no claim of a lifetime limit across retries.

Sonar fault/stale stops AUTO and discards old turn/backoff. Two new trusted front samples permit a fresh decision; unfinished environmental recovery requires a new full scan. Flame/gas interruption aborts the old V2 mission and, after continuous clear wait, performs environmental recovery. DHT/MQ-2 unavailability alone remains diagnostic, while failed reads cannot clear confirmed hazards. Temperature, IR, known obstacles, STOP and applicable live-session gates remain independent.

PWM, calibration, sensor thresholds, Wi-Fi settings, OLED, nRF implementation, radar, typography, navigation trace format, diagnostics and endpoint set were not changed in this audit.

## D. Dead/redundant production-code findings

No production removal was justified across every configuration, indirect dispatch and relevant test path. Kept candidates include:

- `observeIrEpisodes()` is empty but still called by the service loop. Removing the seam would be refactoring, with negligible unmeasured benefit.
- `cancelPulse()` and the lower reverse-speed cap interface have direct test/interface users. `backward()` itself calls the cap implementation. The 75 constant is not evidence that current navigation applies that cap.
- `autoDashboardPollAlive()`, `SensorManager::forceDistanceSample()/readActiveDigital()`, global motor wrappers, unused-looking diagnostics/state members and disabled buzzer support are candidates for a separately reviewed API/configuration cleanup. Text-search absence and linker elimination are not proof of safety across every supported context. They remain.
- Callback routes, ultrasonic ISR, policy templates, serial trace commands, optional capability flags and radio packet types were checked as indirect/conditional entry points. No feature-configuration matrix was exhaustively built in this task; that limitation prevents speculative source deletion.

## E. Documentation, tests and figures inventory

The [pre-cleanup file-level manifest](FINAL_RELEASE_CLEANUP_MANIFEST_20261018.csv) contains 212 existing-file/absence/private-file records with category, original Git status, dependencies, reason, risk, proposed action and exact backup entry: **171 RED, 17 YELLOW, 24 GREEN**. It was written before the first cache removal. Newly created report/manifest are task deliverables, not pre-existing cleanup candidates. Ignored `.pio` build/tool outputs were retained and excluded from preservation input; their selected new verification evidence is copied separately. Ignored historical logs and local `.vscode` configuration are included in the complete private archive.

All source tests, test fixtures, review manifests and source projections remain. Python caches have surviving matching `.py` sources; the unchanged runner explicitly disables bytecode writes. No old assertion was deleted, edited or newly skipped.

| Material | Content/dependency finding and action |
| --- | --- |
| `docs/COLLISION_CLOSURE_STATE_EVENT_MATRIX.csv` | 6,422 rows / 16 columns, including 6,316 rows marked SOURCE. Cross-repository historical state/event evidence with guide/test references. Size does not make it redundant; KEEP RED. Historical source pointers and dispositions are not a current hardware pass. |
| `docs/COLLISION_CLOSURE_*`, `SYSTEM_STATE_COLLISION_VERIFICATION.md` | Defect reproduction, protection inventory, ledger and collision provenance used by historical coverage; KEEP. |
| `docs/V32_RESTORATION_*` | Source audit, frozen manifests, trace summary, preflight, raw diff/status/continuity evidence and eight ignored logs form a reproducible restoration chain; KEEP. Failed early/parity logs are unique development evidence, not errors to erase. UTF-16 logs were decoded for review without rewriting them. |
| `docs/STANDALONE_FREEZE_20261008*` | Exact historical commit/tag/manifest records; KEEP immutable. |
| V3/V3.1/V3.2/V3.3/V3.3.1/V4.5/V4.6 reports, approved execution and communication audit | Different dated algorithms, scope and evidence. YELLOW where historical/current wording is ambiguous; KEEP and qualify through the new current sections. No byte-identical report was established as safely expendable. |
| Physical validation / TESTING / Stage 1 and 2 notes | Physical observations, pending checklists, electrical/methodology context and dated experiments have distinct manuscript value. Preserve original qualifications. |
| Three 2026-10-18 control/reliability reports | Root-cause evolution and software evidence; KEEP. Earlier intermediate test counts are dated snapshots, not today's suite result. |
| Historical/current Python, C++, JS and JSON test assets | Native harnesses, static projections and browser cases depend on one another and the manifests; retain all. A historical filename does not mean its cases are unused. |
| Four `architecture_source/*.mmd` files | Preserve editable research originals. The recovery diagram still describes six turns and preserved episodes; it needs a later owner-reviewed current figure revision before manuscript use. |

All six root PNGs were visually inspected and hashed. Dimensions and roles:

| Exact owner asset | Dimensions | Finding/action |
| --- | --- | --- |
| `figures/Circuit_Diagram.png` | 1055 x 1491 | Power/signal overview; preserve, physical wiring accuracy not certified here. |
| `figures/PIKU_2.1_Rev_A_Software_Architecture.png` | 1122 x 1402 | Byte-identical to deleted tracked `figures/architecture/PIKU_2.1_Rev_A_Software_Architecture.png`: confirmed relocation by bytes. Preserve current path and historical deletion. Some radio-command/servo ownership labels need source reconciliation before paper use. |
| `figures/PIKU_2.1_Rev_A_System_Architecture.png` | 1448 x 1086 | Distinct system/sensing/communications overview; preserve. |
| `figures/PIKU_2.1_Rev_A_Technical_Blueprint_Black.png` | 1448 x 1086 | Different bytes from same-sized deleted tracked architecture-path original; treat as revision, not redundant duplicate. Preserve both provenance and current deletion. |
| `figures/PIKU_2.1_Rev_A_Technical_Blueprint_White.png` | 1448 x 1086 | Distinct owner print-style figure; preserve. |
| `figures/software_diagram.png` | 9427 x 10308 | High-resolution source/module relationship figure; preserve original. |

No figure was regenerated, edited, moved, restored or deleted. No explicit manuscript dependency was assumed absent merely because an image has no text references.

### Seven superseded historical checks: independent replacement review

The unchanged runner exposes seven historical skips rather than modifying their assertion files. Four additional pre-existing CPS-only skips reflect the standalone scope. Replacement review found:

| Historical assertion | Current replacement and practical limit |
| --- | --- |
| `ClosureInventory.test_exact_current_production_delta_and_baseline` | `CandidateReview.test_exact_reviewed_candidate_and_untouched_frozen_source` checks six changed production hashes and every remaining frozen non-secret production hash. This audit independently enumerated all 49 current paths and compared pre/post hashes; a manifest alone is not a formal semantic proof. |
| `V45Inventory.test_reviewed_current_source_delta` | Same candidate/frozen identity coverage plus existing live producer/availability/navigation tests. The historical V4.5 algorithm is intentionally not asserted to be active. |
| `V46Inventory.test_exact_current_scope_and_hashes` | Current candidate hashes and frozen remainder, with historical starting-source JSON retained byte-for-byte. |
| `V46Inventory.test_restoration_source_and_preserved_infrastructure` | Frozen remainder hash checks; `test_navigation_and_command_authority_delta_is_narrow` compares all original navigation function bodies except six explicitly admitted scan/counter functions. Native restoration traces verify reachable navigation. |
| `V46Inventory.test_explicit_stop_and_atomic_mode_backend_are_unchanged` | Exact `handleCommand()/applyMode()` bodies and `applyCommand()` equality after the one allowed fence block, plus 19 extracted actual backend scenarios. Original exact identity would wrongly reject the authorized fence. |
| Rendered `test_shared_computed_styles_and_state_match_actual_frozen_assets` | Current asset rendering across 75 layout cases plus exact preserved current HTML/CSS hashes. Enlarged typography intentionally differs from historical pixels. |
| Rendered `test_other_pages_and_bottom_navigation_have_no_new_visual_regression` | All five panels, narrow/wide widths, zoom/enlarged text, controls and bottom navigation checked by the current Chromium harness. This is defined rendered coverage, not exhaustive device testing. |

Historical static projections return frozen text only when a known candidate hash matches; they cannot substitute for live runtime tests. Current backend bodies, navigation fixtures and browser assets are exercised separately. Projections/manifests/runner remained unchanged. Replacement checks were run before and after cleanup; there was no test-count reduction.

## F. Exact actions performed

1. Captured full status/refs/non-secret file hashes, local private-file hash, production fingerprint, reference and four other protected project baselines.
2. Created and verified private archives before editing; the complete archive additionally preserves ignored historical logs/editor files discovered during inventory.
3. Ran the unchanged complete applicable suite and ESP32-S3 build before cleanup; retained their generated results.
4. Wrote the 212-row pre-cleanup manifest. Rechecked each of its 24 GREEN cache files against the exact archived bytes, validated the resolved target stays under this repository and its `.py` source exists, then removed that exact file only. Total: 180,919 bytes; no directory-wide delete.
5. Prepended current sections to `README.md` and `SOFTWARE_EXPLANATION.md`; their original raw bytes remain intact as suffixes. Added this one report and the requested CSV manifest. All historical records and dates remain unchanged.
6. Ran the same full suite/build after cleanup and verified protected/current hashes and refs. No production or test source was edited.

The exact removed-file allowlist and hashes are in the GREEN manifest rows and private `cleanup-actions.json`; each is under `tests/__pycache__/` (23 files) or `tools/__pycache__/` (one file). All 24 were initially untracked. No historical document/test was archived out of the working tree because unique evidence/dependencies remained.

## G. Verified backup and recovery

Complete private archive, relative to `E:/Development`:

`Research/Continuity/preservation/PIKU_RevA_FinalRelease_Complete_20261018T154601.zip`

SHA-256: `8eeaa03426cb7d0938badf29264fd9ed1aa642d3ad02b9dcb52e549ed390fe9f`.

It contains 209 raw working-tree files under `working_tree/` and `recovery-metadata.json`, including every removed cache, owner figure, dirty source/test file, historical evidence and ignored local editor file. All entries were reread and SHA-256 checked against the captured input, and ZIP CRC verification passed. ACL access was restricted to the current Windows user and SYSTEM. It is outside all Git repositories, uniquely created, and no prior backup was overwritten. The earlier narrower `PIKU_RevA_FinalRelease_20261018T154152.zip` remains preserved; use the complete archive above.

`Secrets.h`, `.git`, and `.pio` were intentionally excluded. This is a working-tree preservation archive, not a self-contained Git clone or public release package. Existing frozen Git blobs preserve the deleted tracked figures; their absent working-tree paths are recorded. Keep the owner's private configuration separately.

Recovery is selective and does not require reset/clean: verify the archive hash using `Get-FileHash -Algorithm SHA256 -LiteralPath <archive>`, extract to a new private inspection directory, compare the selected `working_tree/<path>` with the current file, then copy only the intended reviewed file. For generated caches, restoration is unnecessary for execution, but the exact 24 archived entries are available. For documentation rollback, restore the two original Markdown files only. Do not overwrite newer owner work, restore historical figure deletions by accident, or publish the private archive. Metadata retains original status, missing paths, hashes and refs for comparison.

## H. Pre/post software verification

Commands, unchanged in both runs:

```powershell
python -B tools/verify_reliability_candidate.py
pio run -e esp32-s3-n16r8
```

| Gate | Before cleanup | After cleanup |
| --- | --- | --- |
| Native policy executables | 13 passed | 13 passed |
| Python discovered suite | 218 run: 207 passed, 11 existing skips; 0 failures/errors | Identical counts, test/skip inventory and outcomes |
| Actual Chromium behavior/layout | 55 behavior scenarios, 75 layout cases | Same 55 scenarios; 75 cases without layout/console errors |
| Actual mode/STOP/session backend | 19 scenarios pass | Same 19 pass |
| Extracted actual Manual timing/deadman | Four scenarios pass | Same four pass |
| Navigation trace parity | 34 matching scenarios, 3161 modeled control passes | All summaries and motor/servo/sensor/state trace hashes identical |
| ESP32-S3 build | PASS | PASS; firmware binary identical |
| RAM | 58,996 bytes | 58,996 bytes; delta 0 |
| Flash | 1,059,977 bytes | 1,059,977 bytes; delta 0 |
| Production/configuration SHA fingerprint | `f979ca73108a358b06e3f2f733f97bcb9ecde6d7a69f7323cd5c9ec3251f34b6` | Identical across all 49 paths |

Pre/post `firmware.bin` SHA-256: `a2fbf661719c39734656a2a246457b9c5830a08668840c69d182179aa3c16aaf`. This identifies the local software build, not a verified flashed robot image. Whitespace review passed with command-local CRLF-aware Git settings. All pre-existing non-secret files except the two authorized Markdown additions and 24 exact cache removals retained their captured raw hashes; original Markdown suffixes, all test assertions, freeze records and figures were checked separately.

The mandatory repeated Forward-10-seconds/release/Left/release/Forward/release pattern runs without redundant Manual selection in the browser harness at 20, 650, 900 and 1200 ms response delays. Other cases cover timeout with explicit STOP/authority recovery, intentional abort, independent 750 ms deadman, genuine loss, late/reordered responses, session replacement/expiry, no old held-intent replay, AUTO/Manual switching, and multiple clients. Native Manual timing executes extracted actual admission/STOP/deadman source; browser transport and motors use doubles. These are software checks, not measurements on the robot.

Baseline and post logs/results, navigation traces, browser evidence, the full source comparison, per-file content review and preservation checks are retained under ignored `.pio/final-release-audit/`. The durable private copy is `Research/Continuity/preservation/PIKU_RevA_FinalRelease_Evidence_20261018T155628/`, relative to `E:/Development`; the mission is `piku-reva-final-comparison-cleanup-20261018`. It contains `final-verification.json`, both complete evidence sets, the pre-cleanup manifest, exact removal/restore entries and the non-disclosing security findings. The runner's historical trace-summary write is redirected to generated evidence without changing the historical file. No assertions, runner or verification manifests were modified. Inherited skips remain explicitly disclosed.

## I. Source/documentation inconsistencies

The new CURRENT sections take precedence; old text remains as dated provenance.

- README's V4.5 six-turn/45-second ledger, phase-resumption and no-five-pulse-ceiling passages do not describe the restored active implementation. Current source uses seven starts, five pulses and `now - escapeStartedMs_ >= 30000`; it has no historical active-time ledger.
- Sonar and flame/gas interruption abort/reset old V2 work in the restored implementation. Claims that its old mission/committed-side budget always survives those interruptions are historical. Unfinished environmental context does survive sonar interruption and requires a fresh scan.
- Historical V3.3.1 reports and retained motor-cap tests verify the cap API, but navigation's `beginRecoveryIrBackoff()/updateRecoveryIrBackoff()` call ordinary `backward()`, which applies cap 100. No 75% navigation cap or ordinary reverse cap is claimed for this working tree. This audit deliberately did not alter output to match old prose.
- Legacy firmware/stage strings still identify Stage 2; they do not uniquely identify the uncommitted working revision. Use the source fingerprint and generated binary hash for repeatable identification.
- Old physical-pending labels are historical snapshots. The owner's latest good-operation report is qualitative and does not identify a measured latency or independently hash-verified flashed image.
- Recovery Mermaid and some supplied architecture labels predate the restored behavior. Owner images and diagram sources remain untouched; paper illustrations need reconciliation against current source before publication.
- The historical collision matrix includes CPS/radio paths outside standalone. Its row count is not a count of executed current tests or physical experiments.

## J. Future research-paper evidence inventory

| Claim/evidence area | Exact retained source and supporting records | Evidence boundary |
| --- | --- | --- |
| Firmware architecture/control authority | `src/main.cpp`, `DriveCommandHandler.cpp`, `MotorController.cpp`, four `architecture_source/*.mmd`, six owner PNGs | Source relationships; reconcile historical labels before manuscript use. |
| AUTO and boxed recovery | `NavigationManager.cpp`, `RobotConfig.h`, `StuckRecoveryV2Policy.h`, `RecoveryIrBackoffPolicy.h`, `V2EscapeDecision.h`; native restoration/navigation fixtures and `docs/V32_RESTORATION_TRACE_SUMMARY.json` | Reachable policies and modeled traces; no angle/displacement/rate distribution measured. |
| Sonar evidence and reacquisition | `SensorManager.cpp`, `SonarReacquirePolicy.h`, candidate scan fixture and producer tests | Validity, quorum, request/facing/time and timeout behavior in software; no-return ambiguity remains. |
| Environment and arbitration | `SafetyManager.cpp`, `ManualMotionGate.h`, `ManualOverridePolicy.h`, collision/availability/dispatch tests | Logic and retained confirmed-hazard evidence, not calibrated gas concentration or safety certification. |
| Manual/HTTP reliability | `WebDashboard.cpp`, `WebDashboardAssets.cpp`, `ModeTransitionFence.h`, `tests/native_mode_fence.cpp`, `tests/native_manual_timing.cpp`, `tests/reliability_candidate_browser.js` | Source-extracted handlers and actual browser assets with controlled response schedules; no real-network latency distribution. |
| Physical methodology/history | `docs/PIKU_2.1_REV_A_TESTING.md`, both `docs/PIKU_2.1_REVA_V3*_PHYSICAL_VALIDATION.md`, `docs/TESTING.md`, Stage 1/2 notes, owner figures | Preserve date, reported/observed/pending distinctions. The current owner statement reports good operation, without new instrumented measurements. |
| Baseline comparison/provenance | Supplied earlier tree, frozen commit/tag, exact manifests, raw restoration diffs/logs and this report | Supplied reference bytes are identified; remote GitHub provenance is not independently authenticated. Preserve authorship/history privately. |
| Resources and reproducibility | Exact source fingerprint, pre/post build logs, trace summaries and browser/native results in private continuity evidence | Tool-reported RAM/flash and modeled passes only. Heap/loop-gap/RSSI fields are observability facilities, not recorded physical performance results. |

No manuscript DOCX, photographs, invented measurements, novelty claim, AI capability or new algorithm was created.

## K. GitHub publication security blockers

**SOURCE VERIFIED, privately:** all 425 reachable Git blob objects were inspected with local-value/pattern checks; `include/Secrets.h` has two path-history commits, including its removal. Reachable history contains the current configured SoftAP password in historical `include/Secrets.h` and `include/WiFiConfig.h`. Current non-private inventoried files did not contain that current password, but contain network/profile identifiers and private workstation/provenance metadata. The generated local `firmware.bin` contains the configured password. Values are deliberately omitted from all audit output and this report.

A current ignore rule does not remove historical blobs. The private archive excludes `Secrets.h` but preserves ordinary historical documents, network identifiers and local paths; neither it nor local binaries is approved for public distribution. Firmware provisioning also stores network configuration in device Preferences; that device state was not read.

Before any publication, the owner must decide and authorize: rotate credentials exposed by any public history/artifact; sanitize existing history and other retained refs/copies, or create a clean public repository with reviewed source, attribution and a non-sensitive provenance record; and construct/scan a sanitized release allowlist. Exclude local secrets, editor/build caches, private logs/archives and configured firmware binaries; public binaries need a deliberately non-secret build configuration. Choose these actions separately; this task performed no rotation, rewrite, new public repository, commit or push.

Pattern scanning and exact current-value matching cannot prove absence of every unknown historical token or identifier. Literal matches can be placeholders or labels. This was a non-disclosing local preflight, not a claim that the repository is safe to publish. Plain HTTP, permissive empty Origin and legacy hazard-free direction aliases remain existing trusted-network assumptions; mode tickets are ordering protection, not authentication.

## L. Remaining risks and acceptance boundary

**OWNER PHYSICALLY OBSERVED / OWNER-REPORTED:** the owner reports the latest firmware is performing well. **NOT YET VERIFIED:** the auditor did not operate/flash the robot, identify its flashed binary, measure latency, RF range/reliability, angle error, reverse distance, wiring or electrical robustness. No new physical pass is inferred from software tests.

Cooperative HTTP parsing/writes, the 5 ms direction-change settle, DHT/library reads, I2C, radio writes, OLED and serial work can delay service. The 750 ms deadman is enforced when the service loop runs; it is not an independent hardware timer. Three browser response slots tolerate the tested schedules but do not establish a worst-case real-network bound. Sequence rejection prevents older commands after newer accepted commands; request age starts at firmware acceptance, with no claimed universal wall-clock send-to-receive expiry. Session-recovery protection is not exclusive multi-user authentication of every legacy direction endpoint.

The robot has no rear clearance sensor or wheel-encoder distance confirmation. OPEN sonar is operational evidence, not proof of empty space; stuck-LOW echo can resemble no-return. MQ-2 thresholds are relative, IMU turns are short-term relative estimates, and no hardware failure certification is claimed. Long-run rollover, every optional feature profile and exhaustive browser/network combinations remain outside the finite suite.

For a later supervised physical acceptance run, record the source fingerprint/binary identity, select MANUAL once, hold Forward ten seconds, release, press Left, release, press Forward and repeat; verify wheel stop and readiness after each release. Repeat under controlled response delay and genuine disconnection with wheels initially raised; verify deadman/STOP, no stale movement after reconnect, one-click AUTO/Manual, and two-browser session interference. Check ordinary obstacles, boxed retry, environmental stop/clear and sonar reacquisition using the existing safe methodology. Do not report latency/turn accuracy without instrumented measurements.

**SOURCE VERIFIED at closeout:** the protected reference, PIKU Stage 4, Coordinator, Node standalone and Physical Development retained their captured non-secret file hashes, original missing paths, and Git status/branch/HEAD/refs where applicable. Frozen commit/tag and local private configuration are unchanged. Final dirty inventory has 11 tracked modifications (nine original plus the two Markdown guides), two original owner-figure deletions and 29 untracked files (51 original minus 24 caches plus report/manifest). No reset, clean, stash, branch switch, history rewrite, auto-commit, push or flashing occurred.

FINAL COMPARISON AND SAFE CLEANUP COMPLETE
