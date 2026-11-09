# PIKU 2.1 Rev A Tasks 1 and 2 evidence preservation

Initial preservation review and owner-authorized documentation checkpoint: 9 November 2026, Asia/Dhaka. Scope: Task 1–2 research records only. Hardware, resistor values, firmware, manuscript and all original evidence remain unchanged. Task 3 is in progress separately; its workbooks are outside this checkpoint. Tasks 4–6 are outside scope. This is an additive current record; earlier audit documents and their original conclusions remain preserved as dated history.

## Preservation plan

Keep the five supplied files and all archive members at their existing source locations. Add this combined record and its [checksum manifest](06_TASKS_01_02_SHA256_20261109.csv) to the existing audit structure. Separate measured, calculated and unverified electrical quantities; separate the current pilot, its revision history, historical B0–B4 aggregates, and supplemental video/screenshots. Retain discrepancies rather than resolving them by editing original evidence. Keep the large MP4 outside ordinary Git history and reference its path and checksum. The initial review left these two documents uncommitted. The owner now authorizes a local master-branch checkpoint of only this record and its unchanged checksum manifest, after accuracy, sensitive-information and preservation checks. No original evidence, Task 3 workbook, firmware, hardware configuration or manuscript belongs in that commit; no merge or push is authorized.

## Task 1 voltage measurement record

Source: owner's measurement report supplied with this task. These are **owner-reported physical multimeter measurements**, not readings independently repeated by this review. The actual measurement time, meter model/range/accuracy, probe loading and simultaneous acquisition were not supplied.

| Measurement point | Reported multimeter reading | Evidence class |
|---|---:|---|
| ESP32-S3 3V3 | 3.29 V | Measured, owner-reported |
| ESP32-S3 VIN | 5.13 V | Measured, owner-reported |
| HC-SR04 VCC | 5.10 V | Measured, owner-reported |
| MQ-2 VCC | 5.10 V | Measured, owner-reported |
| MQ-2 AO | 0.74 V | Measured, owner-reported |
| ESP32-S3 GPIO4 | 0.64 V | Measured, owner-reported |
| HC-SR04 ECHO | 0.02–0.10 V | Multimeter reading on a pulsed signal; HIGH amplitude unverified |
| ESP32-S3 GPIO10 | 0.03–0.06 V | Multimeter reading on a pulsed signal; HIGH amplitude unverified |

Both existing dividers remain wired as reported: **signal → 47 kΩ → GPIO → 100 kΩ → GND**. This agrees with Physical Development `Docs/Pin_Map_and_Power_Architecture.md`, implemented signal-conditioning section. These are the documented resistor specifications, not new measured resistance values.

For an unloaded nominal divider:

`V_GPIO = V_signal × 100 / (47 + 100)`

- A hypothetical **5.10 V HIGH calculates to 3.469388 V, approximately 3.47 V**, at the GPIO. This is calculated, not a measured ECHO HIGH or GPIO peak.
- For a hypothetical unchanged 0.74 V AO input, the same nominal ratio calculates to **0.503401 V, approximately 0.50 V**. The separately reported GPIO4 reading is **0.64 V**. Preserve both measurements and this numerical discrepancy. Simultaneity, signal variation, actual resistances, loading and meter conditions are unknown; this does not establish a wiring error or authorize changing either value.
- A low multimeter reading on ECHO/GPIO10 does not establish the amplitude of a short HIGH pulse. No oscilloscope waveform or peak measurement was supplied.

**GPIO electrical safety margin is NOT conclusively verified.** The owner reports that the sensors have been functionally operational during extensive testing. Functional operation is preserved as owner-reported evidence; it does not establish electrical input margin over all signal levels or transients. This record makes no electrical-safety certification or measured worst-case-margin claim. No hardware redesign, resistor substitution or firmware change is made or proposed as an action in this task.

Older nominal 5.2 V design values and conditional divider calculations remain historical design records. The newly reported 5.13 V/5.10 V measurements do not silently replace those specifications.

## Task 2 original files and integrity

Original source directory, relative to Development:

`Research/Meo/The PIKU/PIKU 2.1 Rev A/PIKU 2.1 Rev A Physical Development/Task_02_Physical_Evidence_20261109/`

| Supplied file | Role |
|---|---|
| `PIKU_RevA_Task2_Latest_Audited_20261109.xlsx` | Derived reviewed workbook; latest 20-row table plus original-record and audit sheets |
| `PIKU_RevA_Task2_Latest_Codex_Evidence_20261109.zip` | Preserved first report, previous/latest owner revisions, reviewed workbook, setup photo and handoff |
| `PIKU_RevA_Video_Evidence_v2_Codex_Addendum_20261109.zip` | Original video audit, later corrective addendum and three separate dashboard screenshots |
| `PXL_20261109_064836104.mp4` | Owner-supplied original supplementary demonstration; retained outside a Git worktree |
| `PIKU_RevA_IEEE_Independent_Reviewer_Audit_20261109.md` | Supplied simulated reviewer assessment, scoped to the manuscript and November 8 source archive it inspected |

All five files were hashed. All 13 ZIP members were read/decompressed and hashed; the 12 declared checksum references across the two packages, including the external MP4, match. The standalone latest workbook is byte-identical to ZIP member `latest_independent_audit.xlsx` (SHA256 `9a527bc90e344dadbe60d5279785ec477e8676cfdeb1fc5297d8383d9f5705e5`). The 14-sheet workbook and the three preserved owner versions were parsed read-only. Original XLSX/ZIP/MP4/audit files were not rewritten or exported.

The MP4 remains at the original directory above, filename **PXL_20261109_064836104.mp4**:

- Size: **248,417,709 bytes**.
- SHA256: **4a38b7fb42acd2464fa3bce58522c67537b1b26b301d41dc42a71a4b5b6d4eb9**.
- Container duration: **99.946256 s** (approximately 1 minute 40 seconds); video HEVC, 1920 × 1080, 2,978 video frames; AAC audio stream present.
- The full video stream decoded without reported error. Visual review used 100 one-second-spaced samples across the clip, plus full-resolution checks around mode displays and human/robot interaction. These are video-review samples, not exact physical turn or response-time measurements. Audio was not used to infer trial outcomes.
- A checksum establishes byte identity for preservation; it does not independently authenticate camera capture, absence of prior editing or experimental conditions.

## Current owner-reported 20-trial pilot

The following totals were independently recomputed from `20_TRIAL_FORM!A6:L25` and agree with the stored `AUTO_SUMMARY` and `PAPER_RESULT_TABLE` summaries. Arithmetic reconciliation verifies the entered data, not independent execution of the physical trials.

| Current sheet condition | Trials | Goal attainment | Contact events | Contact-positive trials | Recovery-Yes entries |
|---|---:|---:|---:|---:|---:|
| White box | 5 | 5/5 | 0 | 0 | 0 |
| Thin rod | 5 | 5/5 | 2 | 2 | 0 |
| 90° corner | 5 | 5/5 | 5 | 2 | 1 |
| `Pocket / boxed-in` | 5 | 5/5 | 16 | 4 | 5 |
| **Total** | **20** | **20/20** | **23** | **8** | **6** |

Current entries also report zero operator interventions. All 20 `Video saved?` cells are No; individual-trial evidence/filename cells are empty. Planned names T01.mp4–T20.mp4 are not supplied recordings. Preserve **20/20 goal attainment, 23 contact events and 6 recovery-activation entries as the current owner-reported pilot**. The recovery column says `Yes: log/status` for T13 and T16–T20; corresponding contemporaneous per-trial logs were not supplied. Do not relabel these as six independently verified bounded-V2 activations or completions.

Success is distinct from contact-free travel and from a recovery activation. Contacts are distinct physical touch episodes under the workbook definition; repeated contact events do not become collision probability. No binomial contact-event interval, significance or universal reliability claim is made.

The fourth condition is described by the owner/photo as a constrained L-/U-like course with an available exit. Keep its original `Pocket / boxed-in` sheet label, but do not treat its geometry or success result as equivalent to the historical pocket condition. The photo supports a multi-obstacle arrangement; no measured course dimensions or difficulty comparison are established.

The worksheet defines a 30 cm goal line, 60 s observation window and approximately 40 cm start spacing. These are recorded protocol/settings; no per-trial stopwatch, dimensioned setup image or video independently verifies adherence for all 20 rows. `START_HERE!E15:E17` records 40, 30 and 60. `E18` (same settings) is blank. `E9` says Unknown and `E10` Not verified for the flashed firmware. The date entry `E6` is blank, although `A6` contains a date in its label. Battery values E12/E13 are owner-entered 11.6 V/11.2 V, with measurement conditions unverified.

### Revisions and discrepancies remain preserved

| Record within the first ZIP | Goals | Contact events | Recovery-Yes entries | Interventions | Evidentiary status |
|---|---:|---:|---:|---:|---|
| `original_first_report.xlsx` | 19/20 | 32 | 12 | 1 | First owner-recorded report; original retained |
| `previous_user_revised_v1.xlsx` | 20/20 | 23 | 6 | 0 | Previous owner revision; retained |
| `latest_user_uploaded_v2.xlsx` | 20/20 | 23 | 6 | 0 | Latest owner revision; retained |
| `latest_independent_audit.xlsx` and identical standalone XLSX | 20/20 | 23 | 6 | 0 | Derived reviewed record; not a replacement for raw owner versions |

First report versus latest owner revision contains **19 changed outcome/recovery/contact/intervention/final-state fields over 10 trial IDs**, independently checked against `ROW_CHANGE_LOG!A4:D22`. Previous revision versus latest owner revision differs in one such field: **T08 final state (`20_TRIAL_FORM!H13`) changed from `Stopped / blocked` to `Goal reached / moving`**. T08's current Success and final state now agree. This does not transform the first reported failed/intervened attempt into a success. The owner previously reported retesting, but specific original-versus-retest run identities remain unresolved. Do not add these versions as extra independent trials or overwrite the first report.

Other preserved issues:

- T03 final state `Operator STOP` is accompanied by an owner clarification that STOP occurred after goal crossing; retain that explanation without inventing its timestamp.
- Latest T08 and T09 each record one thin-rod contact; an earlier verbal description of one incident remains a reconciliation issue, not an instruction to subtract a contact.
- T13 changed from no recovery/zero contacts to recovery Yes/three contacts; T18 contacts changed from eight to three. These remain part of the explicit revision history.
- Old BATCH_01_REVIEW–BATCH_04_REVIEW sheets and some note/title text reflect prior states. `PAPER_RESULT_TABLE!A1` still says T08 pending; `ROW_CHANGE_LOG!E12:E15` retains an older blocked-state interpretation, while the later H13 and change-log row 22 record the correction. Preserve the originals and use this dated reconciliation when interpreting current data.

### Speed-format issue resolved at the workbook-display level

Earlier handoff/audit notes call numeric `START_HERE!E11 = 0.8` ambiguous. Inspection found cell style 42 with built-in `numFmtId=9` (`0%`). Opening a temporary byte-identical copy read-only in native Excel independently confirmed **Value2 = 0.8, NumberFormat = 0%, displayed Text = 80%**. Thus the current workbook displays an 80% setting. This resolves the spreadsheet-format question; it does not independently prove the actual speed used in every physical trial. The source workbook and old audit statements were not edited.

## Supplementary owner-reported corridor experiments (T21–T22)

Source: the owner's supplementary physical-results report supplied for this checkpoint on 9 November 2026. T21 and T22 are additional entrance-start experiments in an **L-shaped constrained cardboard obstacle corridor**, separate from the original T01–T20 pilot. The owner reports autonomous entry, navigation through the L-shaped path, recovery behavior at the blocked end, and autonomous exit through the original entrance in both experiments. These are owner-reported physical observations, not independently replayed tests or internally traced recovery episodes.

| Supplementary test | Owner-reported outcome | Chassis-contact events | Human intervention |
|---|---|---:|---|
| T21 | SUCCESS: autonomous exit through original entrance | 5 | None reported |
| T22 | SUCCESS: autonomous exit through original entrance | 7 | None reported |
| **Supplementary total** | **2/2 successful corridor escapes** | **12** | **None in either test, owner-reported** |

The owner observed contact involving chassis portions outside the IR sensors' coverage and proposes incomplete coverage as an explanation for those contacts. Preserve this as an **owner-proposed explanation, not a proven causal finding**. No coverage measurement, controlled causal comparison or synchronized sensor trace was supplied.

The original pilot remains **20/20 owner-reported goal attainment, 23 contact events and 6 recovery-activation entries**. T21–T22 add a separate **2/2 owner-reported successful corridor escapes and 12 contact events**, without changing any T01–T20 row, workbook, revision history or recovery-activation total. Observed recovery behavior in T21–T22 does not establish a count of additional V2 activations or completions.

A combined **descriptive count of 22 reported successes and 35 contact events** is arithmetic across the two groups (20 + 2 successes; 23 + 12 contacts). It is **not one uniform 22-test experimental protocol**, a pooled success-rate estimate, or evidence of contact-free travel. Original pilot goal attainment and supplementary entrance-to-exit corridor escape retain their distinct endpoints and conditions. Keep both groups separate from the historical B0–B4 500-encounter aggregates.

No exact run timestamps, test durations, motor-speed settings, internal telemetry, synchronized state traces or per-turn physical angle measurements were supplied for T21–T22. Do not assign the existing MP4, dashboard captures or separate Task 3 angle-characterization measurements to these corridor trials without specific linking evidence. This supplement is documented here from the owner's report; it does not add or overwrite spreadsheet rows or create fabricated instrument records. The checksum manifest continues to inventory only the five original Task 2 files and their 13 archive members; no new media is implied by this supplement.

## Supplemental video and dashboard evidence

Observed evidence must not be described as absent:

- The original MP4 shows the physical prototype changing position/heading and maneuvering through the constrained multi-obstacle arrangement into more open floor space. This supports qualitative constrained-course navigation/recovery behavior; the exact responsible internal algorithm is not uniquely identified by visible motion.
- `screenshots/AUTO_ACKNOWLEDGED.png` visibly shows `Mode: AUTO`, `AUTO ACTIVE` and `Mode change acknowledged: AUTO.`
- `screenshots/AUTO_NAVIGATION_NORMAL.png` visibly shows `Auto Navigation: Normal`.
- `screenshots/AUTO_NAVIGATION_RECOVERING.png` visibly shows **`Auto Navigation: Recovering`**. `Manual mode required` in that capture belongs to manual-only head controls and does not establish that robot mode was Manual.
- The three original screenshots remain inside the addendum ZIP; no second copy is added to the repository.

The old handoff, workbook video-status notes and original video audit are retained as dated records. Their blanket absence statements about a supplied MP4 or readable recovery-related UI are superseded by the supplied MP4 and screenshot addendum. The per-trial video count remains zero because this is a separate demonstration, not proof of T01–T20 recordings.

### Current source qualification

Current local source was inspected read-only at clean software `master` **0b680ff05d1c7f1e5a0c9da580610f271f042ab8**:

- `include/RobotConfig.h:112–125` enables bounded recovery and configures a 25° step target, seven turn starts, five IR back-off pulses, 30 s mission budget and 2.5 s cooldown. These are implementation settings, not per-turn physical measurements or a guarantee of escape.
- `src/WebDashboardAssets.cpp:2361–2367` displays the generic Recovering summary for a non-NORMAL/OFF V2 state, a non-IDLE hazard state, or an active avoidance source (with assistance-required handled separately). Thus **Recovering is a composite UI indicator**, not a unique V2 activation/completion proof.
- `src/WebDashboard.cpp:1299–1302` exposes more specific episode fields including active, committed side, turns used and IR pulses. No synchronized capture of those fields tied to the decisive video movement or all 20 trials was supplied.

The video audit addendum inspected a November 8 source archive. This review confirms the relevant composite-display logic in the current local checkout; neither fact identifies the firmware flashed during the pilot or demonstration.

The sampled MP4 close-ups around 27 s and 57 s show a Manual-mode screen and 80% setting near AUTO-button taps. The separate screenshot confirms an acknowledged AUTO session at 74%. No synchronized timestamps/log establish that those screenshots describe the same movement segment. Do not use the sampled Manual views to deny the separate AUTO acknowledgement, or use the screenshot to assert AUTO throughout the MP4.

Full-resolution frames around 50–52 s also show a human foot adjacent to/apparently touching the robot. Intent and whether it was an obstacle challenge, restraint or repositioning are not established. Preserve this observation without assigning a numerical intervention/contact count or attaching it to a pilot trial. The clip is not evidence of an entirely untouched 20-trial procedure.

No exact physical turn angles, per-turn timing, synchronized internal-state trace, exact video contact total, flame response latency or verification of all 20 trials is inferred from this clip. No media forensics independently authenticates the owner's original/unedited label; original supplied bytes are preserved by hash.

## Historical B0–B4 and reviewer-audit boundaries

Keep the historical five-configuration, 500-encounter owner aggregates in their existing records, including `01_CONTROLLER_PROVENANCE.md`, `02_500_TRIAL_CLAIM_REGISTER.csv` and `OWNER_CLARIFICATIONS.json`. Their original notebook/encounter-level data and exact tested build mapping remain unavailable. Do not reconstruct trial rows, replace their zero-escape pocket result with the new course successes, pool them with the current 20 trials or infer that the local Git SHA identifies all historical binaries.

The supplied independent reviewer audit is a simulated technical assessment of its stated manuscript/source archive. It supports the need to separate source verification, reported physical aggregates and calculations. It did not independently execute the current 20 trials, measure voltages, reproduce the November 9 build, or verify the post-merge checkout. Its hardware/paper/release recommendations are preserved for later authorized work, not executed here. Tasks 3–6 and any future manuscript disposition of historical versus pilot results remain outside this mission.

## Task 3 boundary: evidence awareness only

Task 3 remains in progress separately. Its two workbooks are located at:

`Research/Meo/The PIKU/PIKU 2.1 Rev A/PIKU 2.1 Rev A Physical Development/Task_03_Turn_Angle_Evidence_20261109/`

- `PIKU_RevA_Recovery_Turn_Angles_75pct_80pct_Recorded_20261109.xlsx`
- `PIKU_RevA_Task3_Turn_Angle_Evidence_Added_20261109.xlsx`

The owner reports earlier angle observations and 20 NEW physical Recovery V2 angle-characterization measurements: ten at 75% motor speed and ten at 80%, each with a nominal 25-degree target. These were separate characterization tests, **not measurements during T21–T22**. Normal 35-degree obstacle-avoidance measurements and a consolidated Task 3 workbook are to be supplied later. This checkpoint acknowledges location and owner-reported scope only; it does not inspect or reinterpret workbook contents, calculate angle results, duplicate the workbooks, edit them or include them in Git. No Task 3 angle result is claimed as a Task 1–2 result.

## Verification and unchanged originals

Review performed: five-file SHA256 inventory; all ZIP-member decompression/hashes; 12 declared checksum-reference matches; five workbook instances parsed including all raw/derived versions; 20-row external arithmetic and 19-field change-log reconciliation; native Excel read-only speed display check; full MP4 video-stream decode plus sampled visual review; three supplemental screenshots and setup photo visually inspected; current source recovery/display logic checked.

No stored spreadsheet error cells were found. Counts were independently calculated from row inputs rather than treated as valid solely because a cached summary matched. No original workbook was authored, saved or repaired. Temporary workbook copies, extracted review images and analysis outputs remain outside the repository.

Only this additive record and the CSV manifest are included in the owner-authorized local documentation checkpoint. The nine pre-existing audit records, source files, original five evidence files, all manuscript DOCX files and original physical documents remain unchanged. The T21–T22 supplement changes this record only; the original evidence checksum manifest remains byte-identical. Task 3 workbook byte integrity is checked without opening their worksheets, and both workbooks remain outside Git. The initial preservation pass performed no commit; this later checkpoint explicitly authorizes only the two-file local commit after verification. No merge, push, publication, hardware change, firmware edit, manuscript edit or rewrite of original experimental records is authorized/performed. The large MP4 stays outside ordinary Git history.