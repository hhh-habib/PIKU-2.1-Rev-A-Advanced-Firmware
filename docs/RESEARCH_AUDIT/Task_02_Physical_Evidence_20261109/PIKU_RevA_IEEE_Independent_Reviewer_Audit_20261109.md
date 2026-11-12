# PIKU 2.1 Rev A — independent IEEE robotics-paper technical audit
**Audit date:** 9 November 2026  
**Decision (simulated peer review):** Major revision before submission. This is an assessment, not an acceptance forecast.

## 1. Audit scope, evidence levels, and exclusions

Read and independently rendered the supplied **PIKU_RevA_IEEE_Revised(2).docx**: six A4 pages in the renderer. Compared its text, table structures and figures against **PIKU_RevA_Team_Based_Revision_20261109.docx** and the earlier September manuscript, and checked relevant source files and preserved verification records in **PIKU 2.1 Rev A Software Development(20261108-232415).zip**.

Important version boundary: the ZIP is a **2026-11-08 archive with uncommitted changes**; the user subsequently reported that Codex merged later Git commits into `master`, ending at `0b680ff05d1c7f1e5a0c9da580610f271f042ab8`, and reported 267/278 passed tests, 11 existing skips, and an ESP32-S3 build pass. Those 9 November verification outcomes and commit identity are **Codex-reported**, not independently reproduced against the user's post-merge Windows checkout. The nine newly committed `docs/RESEARCH_AUDIT/` files in the user's Windows repository were not separately supplied for inspection. The archive contains older related records, including the October V3.2 physical-validation note and November 7 software-verification note.

Verification vocabulary used below:
- **SRC:** verified from inspected 8 November C++ source/configuration.
- **SW-REC:** documented/reported software tests or build; no physical robot execution established.
- **OWNER-PHYS:** reported physical observations/aggregates in manuscript, not individually reproducible from the supplied files.
- **CALC:** calculated under stated assumptions, not measured.
- **UNRESOLVED:** cannot establish from supplied evidence.

No source code, test evidence, manuscript or hardware was changed in this audit.

## 2. Overall referee verdict

### Strengths
1. A completed, photographed four-wheel ESP32-S3 inspection robot with realistic budget, actual pin assignments and separately described motor/logic power rails.
2. Nontrivial source-level motion policy: sensor *validity/direction/age* checks, three-request scan aggregation with a trusted-readings quorum, fault/stale reacquisition, and no replay of interrupted actuation.
3. A bounded, committed-side recovery state machine with explicit exit checks, turn/back-off limits and a cooldown; it does not pretend to guarantee escape.
4. A surprisingly detailed supervisory-control implementation: STOP, command sequencing, explicit/manual permission, short command expiry, mode-transition fencing, session checks, manual obstacle veto and one `MotorController` output owner.
5. Environmental hazards linked to motion authority and fresh scans, not merely decorative sensing.
6. Proper statistical-label correction: contact **events** are not binary contact-positive encounters; no unjustified Wilson intervals.
7. Honesty about unreliable/unknown timing, uncalibrated MQ-2, no encoder/map and inability to attribute historical comparison firmware exactly.
8. A six-page document with actual robot photos, design block diagram, module diagram, controller comparison and relevant hardware datasheets.

### Main weaknesses
1. **Historical comparator traceability**: no encounter-level data or B0–B4 tested image/build IDs in supplied archive; paper states 500 encounters, but available evidence is author aggregates.
2. **Novelty relative to closest prior art**: paper references mobile environmental sensing and classical fusion/reactive-control/HRI sources, but does not rigorously position the exact combination against the most comparable recent low-cost reactive supervisory controllers.
3. **Electrical risk**: documented divider orientation of Rt=47 kΩ, Rb=100 kΩ gives Vout≈3.54 V from a 5.2 V input; with assumed ±5% tolerance worst-case≈3.65 V. This is above nominal ESP32-S3 3.3 V I/O rail; absolute-max margin and actual wiring/voltages have not been verified.
4. **Qualitative metric ambiguity**: Table V combines manually measured angles/gaps with "link-loss stop policy" labels without fully describing experiment timing, where distance was read or how hazard events were defined.
5. **Figure economy**: three tall dashboard screenshots consume space needed for methods/ablation/nearest-prior-art discussion; printed small dashboard text is not independently legible.
6. **Hardware novelty language** needs to be integration novelty, not new sensor or new algorithm invention.
7. **Reproducibility**: current Git SHA identifies a build/test source snapshot; it does not identify the firmware used for each historical physical comparison.

## 3. Technical claim-to-implementation matrix

| Claimed contribution / behavior | Source inspected | Status | Reviewer note |
|---|---|---|---|
| Freshness-, orientation-, validity-aware sonar decisions | `src/NavigationManager.cpp` `recordScanReading`, `finalizeCurrentScan`, `updateAuto`; `RobotConfig.h` | **SRC strong** | Requests 3 readings; final finite estimate requires ≥2 accepted distances, median if 3. Do not claim three valid samples are always obtained. |
| Sonar valid/open/fault/stale states; reacquire on fault/stale | `src/NavigationManager.cpp` and `include/SonarReacquirePolicy.h` | **SRC strong** | Operationally open no-echo is a design assumption, not proof of clear space or all failure detection. |
| Relative gyro turn assistance, checked IMU frames | `src/ImuManager.cpp`, `src/NavigationManager.cpp` | **SRC strong / SW-REC** | WHO_AM_I and frame plausibility, backoff, motor-stopped configuration writes; not a physical turn-error distribution. |
| Committed-side bounded recovery | `include/StuckRecoveryV2Policy.h`, `src/NavigationManager.cpp`, `RobotConfig.h` | **SRC strong** | 25° step target, seven turn starts, five IR backoff pulses, 30 s mission and 2.5 s cooldown are *configured* bounds, not successful-escape guarantees. |
| Single motor-command authority / control priority | `src/MotorController.cpp`, `src/main.cpp`, `src/DriveCommandHandler.cpp`, `src/WebDashboard.cpp` | **SRC strong** | The paper can more specifically explain mode fencing, stale-command/sequence rejection and priority ordering. |
| Manual forward 20/25 cm guard and dual IR veto | `src/main.cpp` and `RobotConfig.h` | **SRC strong** | Rear movement and swept-turn region are not fully sensed; do not assert universal collision prevention. |
| Command timeout 0.75 s | `RobotConfig.h` and command/session pathways | **SRC strong** | **Configured software expiry**, not measured physical 750 ms stopping latency. |
| Fire/gas/thermal priority and 2 s clear/rescan | `src/SafetyManager.cpp`, `src/NavigationManager.cpp`, `RobotConfig.h` | **SRC strong** | Actual sensor response, false alarm and stopping-time distributions were not instrumented. |
| MQ-2 gas/smoke sensing | `src/SensorManager.cpp` and `RobotConfig.h` | **SRC** | Raw/filtered relative ADC only; no calibrated ppm claim. |
| OLED priority, nRF24 status and dashboard HTTP | `src/DisplayManager.cpp`, `src/RadioManager.cpp`, `src/WebDashboard.cpp`, `src/PriorityWebServer.cpp` | **SRC strong** | nRF24 heartbeat is not autonomous navigation coordination or a peer decision-maker. |
| 267/278 tests, 13 native executables, firmware resource use | User's Nov 9 Codex completion / manuscript §VI-F | **SW-REC** | Must link exact release SHA, test command/output and source; no physical benchmark implied. Nov 7 precommit record instead reports 266 passes, 11 skips for a distinct software state. |
| 500 physical encounters, five variants | Manuscript Tables IV/V; not in supplied 8-Nov project trial data | **OWNER-PHYS / UNRESOLVED provenance** | Historical aggregate observations should be labelled, not presented as independently auditable matched experiment. |
| 38→10 contact events across B0→B1 | Table IV | **OWNER-PHYS** | Arithmetic true, causal isolation limited without comparator source/conditions. |
| "B3/B4 did not move in 20 hazard events" | Table V | **OWNER-PHYS** | Observed event count, not a zero-risk safety guarantee. |
| 5,000 BDT bill of materials | Table III | **CALC / estimate** | Item subtotals sum exactly BDT 5,000, but no receipts or date-specific price validation available. |
| 47 kΩ / 100 kΩ divider electrical safety | §III-C | **CALC flags risk** | Explicitly inspect/correct physical resistor orientation and output voltage; do not use estimated numbers as proof of I/O margin. |

## 4. Data integrity and experimental-method audit

The latest Table IV has five rows × five comparator configurations and is internally **arithmetically consistent**. The totals computed from its cells are:

| Variant | Contact events (not encounters) | Recovered encounters | Nonrecovered | Interventions |
|---|---:|---:|---:|---:|
| B0 | 38 | 64 | 36 | 20 |
| B1 | 10 | 77 | 23 | 20 |
| B2 | 17 | 51 | 49 | 20 |
| B3 | 17 | 51 | 49 | 20 |
| B4 | 19 | 51 | 49 | 0 |

All `recovered + nonrecovered = 100`, and all row-level recovery/nonrecovery pairs sum to 20. Totals across variants: **101 contact events**, **294 recovered encounters**, **206 nonrecoveries** and **80 recorded interventions** in 500 encounters, if the recorded aggregates are correct. Overlapping categories (contact plus recovered, contact plus not recovered, intervention plus not recovered) must not be added as disjoint outcomes.

The B0–B1 difference is **28 fewer reported contact events** (73.7% fewer compared with 38) per equal exposure of 100 encounters. Twenty of those 28 are from the 90° concave corner. This does **not** identify a 73.7% reduction in collision probability and does **not** establish causality without consistent comparator conditions and versions.

Critical trade-off: all 35° comparator configurations report **zero contact events but zero recoveries** on the 90° corner category. This demonstrates why *event count alone is inadequate*: remaining immobile/trapped may give zero contacts while failing the mission. B4 pocket category has **2 contacts, 0 recoveries, 0 interventions**, not 20 successful recoveries. This scientific trade-off should be shown explicitly and may be more insightful than a generic "IR improved performance" line.

The supplied November source archive has no identified encounter-level 500-trial dataset and no exact tested B0–B4 image identities; it does contain a **2026-10-01 owner-reported V3.2 physical functional pass** tied to firmware `41281d8325d88c70e074e008a2550a1164102132`, not the same thing as a complete B0–B4 logged comparison. The **2026-11-07 IMU/OLED software record explicitly says physical validation NOT RUN for that incremental change**. Do not transfer the V3.2 physical pass onto the Nov 9 final binary or vice versa.

**Minimal evidence recovery without repeating 500 tests:** search existing notebooks, original camera videos, spreadsheets, experiment sheets, serial logs, Git tags, test-protocol messages and hardware photos for the comparator dates, settings, battery conditions, placement definitions, and trial recording. Preserve originals and create an evidence index. If genuinely absent, retain the qualified author-aggregate framing and avoid causal or significance claims.

Table V remains ambiguous on: when and where gap was measured; whether "hazard rotation" was observed during the same 20 events as movement counts; stimulus intensity; link-loss trigger type; and how "Forward guard" functions as a "link-loss stop policy". Link-loss 0.75/1 s values are software lease settings, not measured stopping delays. Explicitly separate **configuration settings** from **manual physical observations** in heading or notes.

## 5. Electrical/hardware review — potentially high severity

At the stated divider orientation:

`Vout = Vin * 100/(47+100) = 0.680272... * Vin`.

At 5.0 V: 3.401 V; at 5.2 V: 3.537 V. ±5% resistors can increase output to approximately 3.65 V at fixed 5.2 V. This is above nominal 3.3 V logic and can leave minimal/no margin under applicable ESP32-S3 input absolute maximum specifications. **Do not claim protection is proven**, and do not rely only on this paper calculation as evidence of actual wiring: check which resistor is connected to input vs ground, actual source-voltage range, resistor tolerances, input pin voltage and the relevant official datasheet. The risk concerns both HC-SR04 ECHO and MQ-2 AO. Do not publish a new "safe divider" configuration without confirming that hardware was actually built/tested.

Other honest boundaries: L298N voltage-drop estimate is not measured torque; shared motor PWM means no independent left/right speed control; no encoders, bumper or rear ranging; no hardware emergency stop and no quantified physical latency; 3S power/transients and measured battery endurance are absent; exposed prototype wiring remains lab-grade; `ClearNoEcho` can misclassify some disconnection/stuck-low conditions as free space.

These features are a defensible **cost-constrained integration** contribution, not a new electrical circuit topology.

## 6. Literature novelty and architecture

The main contribution is best framed as **evidence-validity-aware, interruption-safe reactive control with explicit manual/hazard authority on a constrained embedded environmental-inspection platform**. This is stronger and more distinctive than "ESP32 robot with multiple sensors". Specific implemented differentiators worth expanding in §IV:
- **Sensor evidence contract**: request matching, direction, sample freshness, ≥2 accepted observations, median among 3, invalid/old/fault reacquire and no stale actuator remainder.
- **Recovery transaction**: commit to feasible side, 7 cm side veto, bounded state transitions, new front check after each 25° turn, 7 starts/5 IR pulses/30 s timeout/2.5 s cooldown.
- **Operator authority**: STOP/RELEASE and session/sequence/mode fence, 750 ms held manual command expiry, 20/25 cm hysteresis and IR veto.
- **Cooperative resource coordination**: checked IMU transport/retries (not during ongoing motor-unsafe configuration writes), hazard display priority, bounded HTTP service, radio heartbeat as telemetry, not peer-motion control.

**Not established as first-in-field innovations**: individual sensor fusion, reactive avoidance, gyro turns, recovery timeouts, local HTTP dashboard, hazard alarm, ESP32-S3 or motor control. A novel *integration/interaction protocol* requires a precise design argument, comparisons against directly relevant peer-reviewed controllers, and measurable outcomes. The existing citations [3]–[7] concern environmental sensing application domains, [8] classical avoidance, [9] classic fusion and [10] HRI; this is useful context but **not a systematic nearest-prior-art comparison** of local fail-safe, freshness-aware and bounded reactive policies. Add a small, accurate contribution-versus-prior-art comparison or replace generic citations with closely matching verified literature. Scholarly citations/DOIs were not independently verified through an online search in this session.

Figure 4 is source-consistent at the module level (NavigationManager, DriveCommandHandler, MotorController, ServoScanner, ImuManager, DashboardPublisher, RadioManager, DisplayManager), but the printed figure is crowded and its abstraction does not clearly depict the **priority/ownership protocol** that makes the engineering contribution. A single clean control/data-flow figure and 3–4 lines of priority arbitration may help more than an extra application screenshot.

## 7. IEEE formatting, page-by-page review

The supplied final DOCX **independently rendered to six pages**. It uses A4 (8.27×11.69 inches) with a two-column layout. Actual target conference may require **US Letter and its own exact IEEE template**; compliance cannot be confirmed without conference author instructions. No clear page-overflow was found in the render, but the following remain:

| Page | Observation | Proposed single-pass fix |
|---|---|---|
| 1 | Good abstract specificity and actual robot photo. Table I has 10 dense comparison rows; related-work differentiation is brief. | Reduce redundant baseline details, use space for nearest competitive control architecture, confirm double-blind requirement. |
| 2 | Real four-view robot photos and interconnection diagram. Figure 3 labels are too small at column width; Table II wraps GPIO entries. | Rebuild/replace schematic from verified wiring as a vector, clarify actual resistors and physical version; keep all real photos accessible. |
| 3 | Valuable actual navigation/recovery content and restored source-derived Figure 4; Control screenshot tall/visually dense. | Emphasize authority arbitration/timeout transitions in Figure 4 and text; keep full screenshot in public supplement if needed to free print area. |
| 4 | Table IV is now **single column**, 8.5 pt, no full-width spanning; text and totals fit. Fig. 6 correctly counts *contact events*. Sensor dashboard screenshot too small to read all diagnostics. | Prioritize trade-off: contact count plus recovery; ensure all category labels and abbreviations defined; compress screenshot only if a higher-value plot needs space. |
| 5 | System screenshot and hazard Table V. Table V mixes observation and policy labels; missing exact method details; screenshot text tiny. | Clarify conditions and metric definitions; keep full original screenshot in a public artifact/repository if removed from paper. |
| 6 | Verification distinction, realistic conclusions, 15 references and tool acknowledgment. | Verify target template, scholarly DOI/bibliography, [12] unpublished status, permissions/funding acknowledgments and GitHub release citation before submission. |

Text conventions may be template-specific: current first-page labels use `Abstract.` and `Index Terms.` rather than the IEEE-conference template's typical `Abstract—` and `Index Terms—`. Avoid hand-correcting before confirming the selected official Word template.

The dashboard screenshots are real but small and low-contrast when printed; three full-length figures are a **space trade-off** in a six-page research paper. Preserving originals in the project/release while focusing the paper on a readable architecture diagram, methods and results may improve scientific information density.

## 8. Reference integrity and claims

- [12] is described as **"author manuscript, 2026"**. Do not imply it was accepted, published or "in press" if it was rejected. If the work cannot be independently obtained, the V2.0 baseline should have more accessible project documentation or a public versioned artifact.
- Existing related-work references [3] Jin et al. (2018), [4] Geng et al. (2022), [5] Rosero-Montalvo et al. (2023), [6] Madhusanka et al. (2024), [7] Arain et al. (2021), [8] Borenstein/Koren (1989), [9] Hall/Llinas (1997), [10] Goodrich/Schultz (2007) cover relevant backgrounds but do not specifically substantiate first-of-kind novelty.
- The acknowledgment claims **financial support from IUB** and names AI assistance. Verify that funding attribution is true and the target conference's author/AI-disclosure and anonymization policy allows the displayed information.
- A versioned GitHub release / archival DOI can substantially improve *inspectability*, but not retroactively prove the B0–B4 historical firmware. Do not write “publicly available” until it has actually been published.

## 9. One-pass remediation plan (without repeating the entire experiment)

**P0 — Mandatory before calling it reviewer-ready**
1. Verify the physical voltage divider orientation and any measured GPIO levels; correct hardware and the paper together if needed; get exact part datasheet limits.
2. Reconcile the exact 500-encounter provenance with original owner data; if absent, keep caveat prominent, reduce causal language, do not fabricate logs.
3. Clarify B0–B4 historical firmware/configuration/evaluation protocol as far as evidence permits; mark unverifiable fields.
4. Correct/qualify Abstract and Conclusion phrases implying *IR alone caused* a measured general improvement. State descriptive within-condition comparison.
5. Tighten Table V's definitions, units, policy-vs-measurement separation, and stop-time claims.

**P1 — Highest improvement in engineering paper quality**
6. State a precise architecture contribution in 3 numbered pieces (trusted evidence, bounded recovery, safety-supervisory control) and tie each to code paths, source-derived figure and available checks.
7. Show both contact events and recovery outcomes in one compact analytical comparison, with no fake confidence intervals or unsupported statistical inference.
8. Improve direct prior-art positioning using verified close competitors, not only broad environmental-robot references; explain novelty *scope* instead of saying “first”.
9. Use publication space more efficiently: Figure 3/4 legibility and methods/results should outrank three full screenshots; originals remain intact in repository/supplement.
10. Add a traceability footnote or artifact statement after GitHub release, with exact version/tag/SHA and test status; keep historic trials separate.

**P2 — Submission production**
11. Confirm exact conference template, A4/Letter, 6-page limit, fonts, bibliography, blind-review policy and AI acknowledgments.
12. Resolve [12] unpublished self-reference, verify bibliographic metadata and determine whether funding statement is accurate.
13. Render and inspect each page in native Word and PDF before final submission.
14. Freeze *one* working DOCX; make a backup outside active folder, then one consolidated edit pass.

## 10. Bottom line

This is **a credible engineering prototype paper**, with real hardware, nontrivial embedded software and a physically grounded discussion of limitations. The existing manuscript has advanced materially from the older September and team-based versions. The current paper should **not** yet be described as a strong experimentally reproducible validation of the B0–B4 controller contribution or a proven novel control algorithm. Strength comes from precisely articulating source-verified coordination and comparing honest trade-offs, while qualifying legacy aggregates. Do not spend more time repeatedly rebuilding tables; prioritize evidence boundaries, electrical safety, most relevant prior art and methodological definitions.

The likely strongest one-sentence research proposition is: **A low-cost environmental inspection robot with an implemented evidence-validation and control-authority architecture, whose bounded reactive navigation and supervised safety policies are assessed through qualified historical physical comparisons and separate reproducible software verification.**
