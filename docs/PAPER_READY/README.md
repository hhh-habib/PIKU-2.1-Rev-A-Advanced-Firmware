# PIKU standalone research documentation

Five editable Word documents explain the current PIKU 2.1 Rev A standalone firmware and its evidence. Start with Document 01, use Documents 02 and 03 for algorithms/control, Document 04 for independently checked software results, and Document 05 for manuscript evidence and owner measurement forms.

| Word document | Purpose |
| --- | --- |
| [01 Firmware Architecture](01_Firmware_Architecture.docx) | Modules, scheduling, interfaces, current architecture diagram and resource evidence |
| [02 AUTO Navigation and Recovery](02_AUTO_Navigation_and_Recovery.docx) | Current thresholds, fresh scanning, decisions, gyro turns, bounded recovery and interruptions |
| [03 Manual Communication and Safety](03_Manual_Communication_and_Safety.docx) | Command/session lifecycle, release/STOP/deadman, mode ordering, Wi-Fi, diagnostics and usability |
| [04 Software Verification and Test Results](04_Software_Verification_and_Test_Results.docx) | Current execution results, all 11 inherited skips and exact software/physical evidence boundaries |
| [05 Research Evidence and Methodology](05_Research_Evidence_and_Methodology.docx) | Supported manuscript claims, experimental protocols, missing measurements and blank data forms |

## Source identity and qualifications

The source is the restored V3.2 navigation plus later sonar/control/session/readability corrections, with uncommitted working development beyond frozen HEAD `fa375a7bf739fc306060048b500c2acf413db7a7`. Frozen tag: `revA-standalone-v32-restored-physical-validated-20261008`. The legacy Stage 2 firmware string is not a unique revision identifier.

Non-secret production fingerprint: `f979ca73108a358b06e3f2f733f97bcb9ecde6d7a69f7323cd5c9ec3251f34b6`.

Read [source identity](evidence/source_identity.json), [verification results and exact skips](evidence/verification_results.json), and [full navigation trace identifiers](evidence/navigation_trace_summary.json). These small records support reading without copying firmware, large datasets, private backups or configured binaries.

Evidence categories are **IMPLEMENTED**, **SOFTWARE VERIFIED**, **OWNER-REPORTED PHYSICAL TESTING**, **FUTURE WORK** and **NOT YET MEASURED**. The latest owner report says operation is successful. No physical latency, turn-angle error, reverse distance, RF reliability statistic or independent flashed-binary match was supplied or measured in this task. Software categories overlap; never add 207 Python passes to browser/native/trace counts as independent trials.

## Figures

Four clearly labelled current-source software diagrams are included below, with editable DOT/SVG and PNG for Word embedding. They are newly authored schematics, not synthetic robot photographs. Owner originals remain unchanged in the software project's `figures/` and `architecture_source/`; Document 05 inventories their suitability and historical mismatches.

| Diagram | Files |
| --- | --- |
| Standalone module architecture | [PNG](figures/01_current_architecture.png), [SVG](figures/01_current_architecture.svg), [DOT](figures/01_current_architecture.dot) |
| Ordinary AUTO decisions | [PNG](figures/02_ordinary_auto.png), [SVG](figures/02_ordinary_auto.svg), [DOT](figures/02_ordinary_auto.dot) |
| Current recovery and retry states | [PNG](figures/03_recovery_state_flow.png), [SVG](figures/03_recovery_state_flow.svg), [DOT](figures/03_recovery_state_flow.dot) |
| Manual command and safe readiness lifecycle | [PNG](figures/04_manual_command_lifecycle.png), [SVG](figures/04_manual_command_lifecycle.svg), [DOT](figures/04_manual_command_lifecycle.dot) |

## Original references

Every Word document includes internal source/evidence citations using project-relative paths and function names. Primary implementation references are `src/main.cpp`, `src/NavigationManager.cpp`, `src/WebDashboard.cpp`, `src/WebDashboardAssets.cpp`, the sensor/safety/motor modules and `include/RobotConfig.h`, `StuckRecoveryV2Policy.h`, `V2EscapeDecision.h`, `ManualMotionGate.h` and `ModeTransitionFence.h`. Primary reproduction references are `tools/verify_reliability_candidate.py`, current browser/native fixtures and the exact hashes in source_identity.json.

The preserved physical record `docs/PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md` describes owner-reported 2026-10-01 functional results for an earlier executable. `docs/PIKU_2.1_REV_A_TESTING.md` and `docs/TESTING.md` retain older observations. The V3.3 physical checklist is historically pending and has older six-turn semantics. Do not treat those as measurements for this current source.

Full source/tests and historical records remain in the standalone software project and are not duplicated here. The package is self-contained for reading; reproducing tests requires that separately preserved source/toolchain. Its bibliography paths are provenance identifiers, not claims that source code is included in this folder.

## Measurements still needed

Document 05 provides blank owner forms for run identity and conditions, request RTT, device application delay, independently measured physical stop, angle error, recovery starts/pulses/retry count and observed outcome. Record real runs, instruments, clock alignment, units and exclusions before making a quantitative manuscript claim. Blank fields are not zeros.

## Copying to Physical Development

The owner may manually copy this entire PAPER_READY directory into `PIKU 2.1 Rev A Physical Development/Docs/PAPER_READY`, keeping `figures/` and `evidence/` beside the five DOCX files and this README. No other project was modified by this task. The Word files contain their diagrams and citations, so they remain readable independently; keep the companion files for editable diagram sources and exact result identifiers. Do not copy the software working tree, local private configuration, `.pio`, private archives or configured firmware binaries. Public distribution remains subject to the separate repository/credential security review.

## Package verification

All five DOCX files opened and rendered successfully; all 38 final pages were visually inspected, including diagrams, equations and tables. Internal source citations resolve to their bibliographies. Package links and the current project-local Markdown targets were checked with no broken targets found. The package passed configured-credential, private-path, token and embedded-object checks. The public product title is retained.

Before and after organization, the unchanged suite reported 207 Python passes and 11 disclosed skips out of 218, 13 native policy executables, 55 browser scenarios, 75 layout cases and 34 identical navigation traces covering 3161 modeled control passes. The ESP32-S3 build passed with 58996 bytes static RAM and 1059977 bytes application flash; both deltas are zero. These categories overlap and are not independent physical trials.
