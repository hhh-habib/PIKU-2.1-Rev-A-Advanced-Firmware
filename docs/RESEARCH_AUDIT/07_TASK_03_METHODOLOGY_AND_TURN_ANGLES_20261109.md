# PIKU 2.1 Rev A Task 3: methodology, physical turn angles and firmware identity

Review/owner-clarification date: 9 November 2026, Asia/Dhaka. Confirmation and consolidation dates are not individual experiment timestamps. This documentation/data checkpoint performed no physical test, firmware build, upload, flash readback, hardware change or IEEE manuscript edit. Tasks 4–6 await separate assignment.

## Evidence levels

- **OWNER-CONFIRMED:** owner's project identification, physical procedure and observations. This review did not independently operate the robot.
- **SOURCE-CODE VERIFIED:** behavior checked in actual local source at the recorded revision.
- **ARTIFACT/LOG VERIFIED:** inspected file existence, bytes, hashes, formulas and saved run records. Arithmetic validation does not independently replicate experiments.
- **NOT INDEPENDENTLY VERIFIED:** a link without independent identifying records, notably the exact experiment-specific binary/commit.
- **NOT RECORDED:** unavailable individual metadata. Blank is not zero or No.

## Firmware identity: four distinct layers

| Layer | Finding | Evidence level |
|---|---|---|
| A. Firmware project | **PIKU 2.1 Rev A Software Development was uploaded to the physical robot**, as confirmed by the owner | OWNER-CONFIRMED |
| B. Current reviewed source | master `004cf73521de2e67470bad40648e09770befda53`, the Task 1–2 checkpoint; PlatformIO **esp32-s3-n16r8** | SOURCE-CODE VERIFIED |
| C. Existing build/image evidence | Two actual ESP32-S3 firmware images and paired ELF files exist locally, were hashed and passed image validation. Saved master-final regression JSON and its build receipt exist | ARTIFACT/LOG VERIFIED |
| D. Exact image/revision deployed for T01–T22 and angle tests | No experiment-linked upload record, flash readback or synchronized unique build identifier was located in the inspected records | NOT INDEPENDENTLY VERIFIED |

Use: **Firmware project: PIKU 2.1 Rev A Software Development (owner-confirmed). Exact tested binary/commit: not independently verified.** Do not call the project unknown or assign the newest Git SHA to past experiments.

Revision B is the source reviewed before this documentation/data commit. Production source/configuration is unchanged since `0b680ff05d1c7f1e5a0c9da580610f271f042ab8`. At the reviewed revision, the `src` tree is `65e190e66c614dfc30bba7f1963bd697c93b9b39`, `include` tree `81c1b431f2c2effe997b3924dff2e881d911dc38`, and `platformio.ini` blob `1a751d219fa9f5a6841bcad5ff4d03ca070a271a`. The nonprivate 49-file fingerprint remains `331a3f4c47580255a0d7a9635d67d6ec406d4a84d7d7e43a63c17d6f1e888164`. Recipe: sort slash-relative src/include paths plus platformio.ini, excluding Secrets.h; SHA256 UTF-8 records `relative_path + NUL + lowercase_file_SHA256 + LF`. Private configuration is not copied or published.

`platformio.ini` selects Arduino on esp32-s3-devkitc-1, 16 MB flash, default_16MB.csv and qio_opi memory configuration. Libraries have specified versions; the espressif32 platform itself is not version-pinned there. This describes current configuration, not an independently identified historical toolchain or deployed image.

### Version reporting and available records

The static firmware label is **2.1.0-revA-stage2**: `include/RobotConfig.h:9`, `include/WebDashboard.h:12`. `src/main.cpp:927–930` prints the boot version, `src/WebDashboard.cpp:1223` returns the API firmware field, and `src/NavigationTrace.cpp:169` uses the same label. No unique Git SHA/build-ID injection was found in inspected production/configuration paths. Implemented boot printing is not a captured boot log. This reused label cannot distinguish the two actual images below.

Paths in this table are relative to Software Development. Images/ELFs remain ignored local artifacts and are not copied into Git.

| Existing artifact | Bytes | SHA256 |
|---|---:|---|
| `.pio/master-final-20261109-56028aa2/build/esp32-s3-n16r8/firmware.bin` | 1,061,888 | `ab1a173137e21720e36d6fa5aee9316da0df248f98637d9e9d0126ae5644566f` |
| `.pio/master-final-20261109-56028aa2/build/esp32-s3-n16r8/firmware.elf` | 17,456,044 | `eafc8db1aa3274b47d3375d80fff0832c541f0ad41505c44f1f8740a4ff29957` |
| `.pio/git-cleanup-20261109-91fd0357/build/esp32-s3-n16r8/firmware.bin` | 1,061,888 | `d6c9bee316eb18314ea35f368b401d03d4fe4c2482ebe7b0b27a5fca09f40959` |
| `.pio/git-cleanup-20261109-91fd0357/build/esp32-s3-n16r8/firmware.elf` | 17,456,008 | `739b08e3f8ce6fa0da25719972ff9e0f3feb14b550f3009d91f95c3cfbef230d` |
| `.pio/master-final-20261109-56028aa2/regression/candidate-results.json` | 2,155 | `34bbc7c82cf1ccda72c9838798b40bbe4d6e71e19c99de5f06ca8189b3f468cc` |

Installed esptool 4.11.0 checked both images read-only with `--chip esp32s3 image_info --version 2`: five segments, ESP32-S3, 16 MB image header, valid checksum and validation hash. These are image-format checks, not upload verification. The master-final embedded ELF SHA256 matches its paired ELF. Its application descriptor contains `arduino-lib-builder`, `esp-idf: v4.4.7 38eeba213a`, compile time `Mar 5 2024 12:12:53`, and ESP-IDF `v4.4.7-dirty`. These generic prebuilt-framework fields do not identify the PIKU Git revision or the date of robot experiments. The image validation hash is distinct from the complete-file SHA256 above.

Development-relative receipt `Research/Continuity/missions/2026/mission-20261109-063511-56028aa2/receipt.json` records the November 9 master-final build: 61,188 bytes RAM, 1,061,529 bytes flash, 278 discovered regressions with 267 passed, 11 skipped and no failures/errors, plus 13 native policies. The saved regression JSON agrees. Flash resource reporting and complete image file size are different quantities. These are prior software-run records, not newly rerun tests or evidence that this image was deployed for physical trials. Ten scoped log candidates were checked: dated docs logs, cv_debug and master-final tests. No captured PIKU boot prefix or upload-success marker was located there. No experiment-linked upload/readback was supplied with physical evidence. This bounded search does not establish that no other records exist elsewhere.

## Primary T01–T20 physical pilot

The owner confirms indoor physical obstacle experiments using the PIKU robot. Four original conditions have five repetitions each: white box, thin rod, 90° corner and `Pocket / boxed-in`. The last label is retained from the workbook. Its available-exit constrained course is not established as geometrically equivalent to the older B0–B4 pocket. Measured course dimensions and floor-friction characterization were not supplied.

| Method item | Owner-confirmed procedure | Scope |
|---|---|---|
| Operation | AUTO, motor-speed setting **80%** | T01–T20; controller percentage is not measured linear speed |
| Observation limit | **Stopwatch-controlled 60 seconds** for each trial | Observation limit, not a recorded completion time or measured recovery latency |
| Start T01–T10 | Approximately **40 cm before the obstacle** | White-box/thin-rod subgroup |
| Start T11–T20 | **Inside the constrained obstacle area** | Corner/constrained-course subgroup; generic 40 cm field does not describe these starts |
| Success | After passing the obstacle, autonomous travel **more than 30 cm beyond it**, within the observation limit | Contacts counted separately |
| Human intervention | Zero reported interventions required for successful completion | Owner report, not independent video verification of all 20 tests |

Results remain **20/20 owner-reported successful goal attainments, 23 physical contact events, 6 recorded recovery-activation entries and zero reported human interventions**. Original rows, revisions and discrepancies remain in [Task 1–2](06_TASKS_01_02_EVIDENCE_PRESERVATION_20261109.md). T03's Operator STOP has an owner clarification that STOP followed goal crossing; no exact timestamp or STOP needed to achieve the goal is inferred. No individual elapsed completion times are invented.

### Definitions and historical field reconciliation

A contact event is a discrete confirmed physical touch episode. The archived definition counts a continuous scrape as one episode; repeated distinct touches count separately. Goal attainment can coexist with contacts. Event totals are not counts of unique collision-positive trials or collision probabilities.

A recovery-activation entry is a recorded Yes/state/status observation. Six pilot entries do not establish six independently traced V2 episodes, six completed escapes or a recovery success probability. Physical recovery means leaving the obstructed situation and continuing autonomously, with the experiment's specified endpoint. Firmware mission limits are not measured recovery duration. A generic Recovering UI indicator can cover several navigation/hazard states; batch observations do not uniquely identify each internal phase.

Archived `START_HERE!A9` is **Flashed firmware build/tag/SHA (if known)**, with E9 Unknown. A10 is **Firmware identification method**, with E10 Not verified. These are exact-build/method fields; they do not mean that the firmware project is unknown. Original cells and blank exact-SHA/tag fields remain unchanged. The current owner-confirmed project distinction above governs interpretation.

The archived speed input E11 is numeric 0.8 formatted as 0%, displaying 80%; the Task 1–2 native Excel read-only check verified this. The owner now additionally confirms actual 80% use for the primary pilot. E15:E17 store generic 40/30/60 values, but E15 does not encode inside-start trials. The old success definition says crossing a 30 cm goal line; current owner clarification is more than 30 cm beyond the obstacle after passing it. Current methodology supersedes these incomplete/generic readings without changing archived cells. Stopwatch use is owner-confirmed; independent per-trial timed recordings and exact completion times remain unavailable.

Preserve the first report's 19/20 goals, 32 contacts, 12 recovery entries and 1 intervention as revision history. Do not turn versions/corrections into extra independent trials. The historical B0–B4 500-encounter aggregates remain separate; their original notebook and exact comparator-build mapping are unavailable. Do not transfer this pilot's stopwatch, speed, starts or success rule to the historical comparison.

## Separate supplementary corridor tests T21–T22

The robot started before the entrance to an L-shaped constrained cardboard corridor. The owner activated AUTO. In both experiments it autonomously entered and traversed the L-shaped path, exhibited recovery behavior at the blocked end, then exited through the original entrance.

| Test | Owner-reported outcome | Contact events | Human intervention |
|---|---|---:|---|
| T21 | Successful autonomous entrance-to-exit escape | 5 | None reported |
| T22 | Successful autonomous entrance-to-exit escape | 7 | None reported |
| Separate supplement | 2/2 successful escapes | 12 | None in either test, owner-reported |

The owner proposes that chassis regions outside IR coverage contributed to contacts. This is an **owner-proposed explanation, not independently proven causality**. No controlled coverage/causality experiment is supplied. Visible recovery behavior does not supply an additional V2 activation count.

Do not automatically apply the pilot's 60 s limit, more-than-30 cm criterion, 80% speed, start spacing or other unrecorded parameters to T21–T22. Exact corridor timestamps, durations, internal telemetry and per-turn angles are NOT RECORDED. The combined **22 reported successes and 35 contact events** are descriptive arithmetic across two distinct protocols, not one uniform 22-test success-rate study.

## Separate physical turn-angle characterization

The owner measured heading change using manual marks and a protractor. Four groups contain ten physical turns each: Recovery V2 nominal 25° at 75% and 80%, and normal avoidance nominal 35° at 75% and 80%. A01–A20 and O01–O20 identify angle observations, not T-series navigation tests. **None of these 40 readings is attributed to T21–T22.**

The recovery batches have owner-confirmed Recovering dashboard status at batch level. Recovery 80% has no reported manual safety STOP; Recovery 75% safety-STOP status is NOT RECORDED. Both normal-avoidance batches were described as freely turning with no manual safety STOP. Per-turn left/right directions, episode IDs, exact timestamps, trigger details, gyro terminal angles and completion reasons are NOT RECORDED. A qualitative similar-left/right statement does not mean each row was measured twice. Manual-mark/protractor uncertainty and instrument calibration were not quantified.

| Turn family | Setting | Nominal target | n | Mean physical angle | Mean signed error | MAE | Observed range |
|---|---|---|---:|---:|---:|---:|---|
| Recovery V2 | 75% | 25° | 10 | 24.0° | -1.0° | 1.4° | 22–26° |
| Recovery V2 | 80% | 25° | 10 | 25.1° | +0.1° | 1.1° | 23–28° |
| Normal avoidance | 75% | 35° | 10 | 31.8° | -3.2° | 3.2° | 31–33° |
| Normal avoidance | 80% | 35° | 10 | 35.1° | +0.1° | 1.5° | 33–37° |

Signed error = measured physical angle minus nominal target. Absolute error = ABS(signed error). MAE averages individual absolute errors, not the absolute value of mean error. These calculations reference nominal **25°/35°**, not internal **26°/36°** completion thresholds. No pooling across turn types, statistical superiority of 80%, calibrated accuracy guarantee or causal speed-effect claim follows from these n = 10 descriptive samples.

### Canonical workbook integrity and reproducibility

The one Git-tracked workbook is [the consolidated angle evidence](data/PIKU_RevA_Task3_Consolidated_Physical_Turn_Angles_20261109.xlsx). It is a byte-for-byte copy of the owner-resaved original, **22,643 bytes**, SHA256 **`3ad68cfac037e8d38c79627de3aefec895f0262c77beadf6506912590827661a`**.

Original Development-relative location:

`Research/Meo/The PIKU/PIKU 2.1 Rev A/PIKU 2.1 Rev A Physical Development/Task_03_Turn_Angle_Evidence_20261109/PIKU_RevA_Task3_Consolidated_Physical_Turn_Angles_20261109.xlsx`

Five sheets remain intact: START_HERE, RECOVERY_25_RAW, AVOIDANCE_35_RAW, ANGLE_COMPARISON and EVIDENCE_PROVENANCE. Raw rows are `RECOVERY_25_RAW!A8:K27` and `AVOIDANCE_35_RAW!A8:K27`; speed groups use rows 8–17 and 18–27. Measured angles are E, targets C, signed errors F and absolute errors G. Summary is `ANGLE_COMPARISON!A7:L10`; descriptive speed differences are B14:C15. Original metadata and provenance text are retained.

Read-only ZIP/XML inspection verified archive CRC, all **40 raw readings**, observation IDs/group separation and blank direction/episode fields. All **80 row-error formula cells** and **28 summary/difference formula cells** were independently checked against raw inputs. Excel shared formulas were resolved from master references, not mistaken for missing formulas. All **108 formula cells** and cached results match independent arithmetic within floating-point tolerance. No stored spreadsheet error cells or external workbook links were found. All 20 consolidated recovery observations match prior recorded IDs, targets, speed settings and measured values. The 20 normal-avoidance readings come from the consolidated owner record; no prior independent trace is claimed.

The package identifies Microsoft Excel as the saving application, consistent with the owner report. No pre-resave checksum exists for this consolidated file; this review cannot independently authenticate every pre-resave byte/edit. It verifies the current complete structure, raw values, calculations and provenance fields, cross-checks prior recovery readings, then preserves the resaved file exactly. No workbook was recalculated, repaired, reformatted or resaved by this review. Unchanged workbook views were not rendered/redesigned.

Earlier originals remain unchanged at the same physical evidence folder:

| Task 3 original file | SHA256 |
|---|---|
| PIKU_RevA_Recovery_Turn_Angles_75pct_80pct_Recorded_20261109.xlsx | `97c7b50ffe97cf14dfae1bab9b9dc17728b74a7562c4f0f84a76ea91e31fb490` |
| PIKU_RevA_Task3_Turn_Angle_Evidence_Added_20261109.xlsx | `1d28039aeb04dc35c908c63a4ea3a23392279ed07f924c8f3b7856f4ec87862c` |
| PIKU_RevA_Task3_Consolidated_Physical_Turn_Angles_20261109.xlsx | `3ad68cfac037e8d38c79627de3aefec895f0262c77beadf6506912590827661a` |

Only the consolidated workbook is added to Git. The older template's zero protocol-eligible count reflects missing required metadata, not zero measured angles. Its old 75%-summary statement that 80% is pending is an intermediate note superseded by recorded 80% inputs and the consolidated file. Earlier TURN_ANGLE_EVIDENCE ranges and qualitative >90% observations are preserved historical observations, not extra rows in the 40-reading dataset.

## Source-checked turn execution

All findings refer to reviewed source `004cf73521de2e67470bad40648e09770befda53`. They do not independently identify the image flashed during the angle tests.

| Source location | SOURCE-CODE VERIFIED behavior |
|---|---|
| `include/RobotConfig.h:110–128` | Normal nominal target 35°, recovery 25°, shared added threshold 1°, gyro deadband 2°/s, individual deadline 2500 ms, ordinary timed fallback 1500 ms |
| `src/NavigationManager.cpp:982–1013` | Ordinary turn resets relative angle, records initial gyro-Z baseline and commands wheel spin |
| `src/NavigationManager.cpp:1031–1060` | Distinct fresh gyro samples accumulate magnitude of baseline-subtracted gyro-Z rate × sample interval. GYRO_TARGET check is angle >= target + 1°, therefore **36° normal / 26° recovery** in this path |
| `src/NavigationManager.cpp:1025–1048`, `:1066–1077` | V2 invalid/stale/gapped gyro enters hold; no timed completion. Ordinary unavailable-gyro turn can complete after 1500 ms elapsed from turn start. Hard deadline stops without proving target attainment |
| `src/NavigationManager.cpp:1081–1108`, `:1408–1454`, `:1521–1560` | Completion stops/settles then checks fresh front evidence. V2 requires ready IMU, committed direction and resets each increment |
| `src/ImuManager.cpp:85–111`, `:181–190`, `:223–225` | Checked data-ready sensor frames, gyro-Z conversion and successful-sample timestamps |
| `src/MotorController.cpp:227–240` | Spin commands drive opposite wheel sides in opposite directions using selected speed |

Configured timing is not experimentally measured timing. Nominal target, integrated gyro completion threshold and protractor-measured heading change are distinct quantities. Adding 1° is not ±1° physical accuracy. This path accumulates rotation magnitude and has no post-stop precision-heading correction. Without recorded terminal samples/reasons, no physical reading is assigned GYRO_TARGET, TIMED_FALLBACK or an interruption. No observed offset is causally assigned to gyro error, wheel slip, stopping delay or another mechanism.

## Reconciliation and preserved boundaries

- Owner-confirmed project identity supersedes generic unknown-project wording. Exact deployed image-to-experiment mapping remains NOT INDEPENDENTLY VERIFIED. Current hashes, static labels and saved build success cannot supply that link.
- Original build/SHA placeholders and blank identification fields remain unchanged in archived workbooks. Current methodology incorporates the owner's explicit stopwatch, 80%, start-pose and success-rule clarification.
- T21–T22, the primary pilot and four angle groups keep separate protocols. No unrecorded corridor timing/speed/angle values are filled in.
- The original demonstration supports qualitative constrained-course recovery. Separate screenshots show AUTO acknowledgment and Auto Navigation: Recovering. Their positive evidence and synchrony limits remain as documented in Task 1–2; they are not reassigned to particular corridor trials or angle rows.
- All historical B0–B4 results, earlier workbook versions, original MP4/screenshots/ZIPs and IEEE papers remain unchanged. Original Task 2 hashes remain in the unchanged [18-entry manifest](06_TASKS_01_02_SHA256_20261109.csv). The large MP4 and local firmware binaries stay outside Git history.
- Remaining gaps are exact tested-image mapping; individual turn directions/episode IDs/timestamps/internal terminal states; quantified manual angle-measurement uncertainty; Recovery 75% safety-STOP status; and unrecorded T21–T22 timing/speed. The historical notebook/comparator mapping remains unavailable. These limits do not erase the 40 supplied measurements or owner-confirmed methodology.

No firmware, hardware, manuscript, original evidence, AI functionality, merge or push is changed/performed by this checkpoint.
