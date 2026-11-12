# Documentation guide ? 13 November 2026

Current software guides coexist with preserved dated evidence. A historical document's ?current? wording refers to its original task/date. This entrypoint and the root README identify the new package; original measurements, manifests and test assertions are not rewritten.

## Current package

- [Root README](../README.md) and [execution guide](../SOFTWARE_EXPLANATION.md): current behavior, hardware and module ownership.
- [Build and reproduce](REPRODUCIBILITY.md): download/build/upload instructions, prerequisites and full-clone regression entrypoint.
- [Release record](RELEASE_20261113.md), [verification summary](release/verification_20261113.json) and [source hashes](release/source_identity_20261113.json).
- [Paper citation and provenance](CITING_AND_EVIDENCE.md), [CITATION.cff](../CITATION.cff).
- [Complete delivered-file inventory](REPOSITORY_CONTENTS_20261113.md) and [current schematic](../architecture_source/current_standalone_20261113.mmd).
- [Optional navigation trace format](NAV_TRACE_SCHEMA.md): telemetry semantics, with dated validation wording bounded by the evidence below.

## Dated implementation evidence

Latest implementation layer: [IMU/OLED, 7 November](IMU_OLED_SOFTWARE_VERIFICATION_20261107.md). Earlier layers: [19 October response repair](STANDALONE_RESPONSE_SOFTWARE_VERIFICATION_20261019.md), [18 October comparison](FINAL_RELEASE_REVIEW_20261018.md), [reliability candidate](STANDALONE_RELIABILITY_CANDIDATE_20261018.md). HEAD/uncommitted wording and numerical totals are historical snapshots; the implementation was subsequently committed.

Restored navigation: [verification](V32_RESTORATION_SOFTWARE_VERIFICATION.md), [preflight](V32_RESTORATION_PREFLIGHT_REPORT.md), [source audit](V32_RESTORATION_SOURCE_AUDIT.json), [trace summary](V32_RESTORATION_TRACE_SUMMARY.json). The [8 October freeze](STANDALONE_FREEZE_20261008.md), [manifest](STANDALONE_FREEZE_20261008_MANIFEST.json) and [verification extract](STANDALONE_FREEZE_20261008_VERIFICATION.txt) remain immutable historical identities.

Collision provenance: [system-state verification](SYSTEM_STATE_COLLISION_VERIFICATION.md), [closure](COLLISION_CLOSURE_SOFTWARE_VERIFICATION.md), [matrix guide](COLLISION_CLOSURE_MATRIX_GUIDE.md), [ledger](COLLISION_CLOSURE_F4_LEDGER.md) and JSON/CSV companions. These provide historical regression context, not new hardware validation.

## Physical evidence and paper methodology

- [Tasks 1?2](RESEARCH_AUDIT/06_TASKS_01_02_EVIDENCE_PRESERVATION_20261109.md), [original hashes](RESEARCH_AUDIT/06_TASKS_01_02_SHA256_20261109.csv).
- [Task 3](RESEARCH_AUDIT/07_TASK_03_METHODOLOGY_AND_TURN_ANGLES_20261109.md): T01?T20, separate T21?T22, four ten-turn groups, and unverified exact deployed-image linkage.
- [Research entrypoint](RESEARCH_AUDIT/00_READ_FIRST.md), [controller provenance](RESEARCH_AUDIT/01_CONTROLLER_PROVENANCE.md), [source identity](RESEARCH_AUDIT/03_SOURCE_IDENTITY.md), claim registers and source/results matrices.
- [Task 4 literature/evidence](RESEARCH_AUDIT/08_TASK_04_CONTRIBUTIONS_AND_RELATED_WORK_20261111.md): dated research reconciliation, not a new implementation/publication claim.
- [Historical V3.2 physical report](PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md), [earlier Rev A tests](PIKU_2.1_REV_A_TESTING.md), [Stage 1](PIKU_2_1_REV_A_STAGE1.md), [Stage 2](PIKU_2_1_REV_A_STAGE2.md), [older V2.0.1 tests](TESTING.md).
- [V3.3 physical checklist](PIKU_2.1_REVA_V33_PHYSICAL_VALIDATION.md) and [response checklist](STANDALONE_RESPONSE_PHYSICAL_CHECKLIST_20261019.md) retain their original pending/task scope.

Tracked original XLSX/reviewer Markdown remain unchanged. Original video and ZIP evidence packages stay local with their recorded hashes; a manifest entry does not promise those recordings inside the source ZIP.

## Historical algorithms and reading packages

[V3](RECOVERY_V3_SOFTWARE_VERIFICATION.md), [V3.1](RECOVERY_V31_SOFTWARE_VERIFICATION.md), [V3.2](RECOVERY_V32_SOFTWARE_VERIFICATION.md), [V3.3](RECOVERY_V33_SOFTWARE_VERIFICATION.md), [V3.3.1](V33_1_HARDENING_SOFTWARE_VERIFICATION.md), [V4.5](V45_SOFTWARE_VERIFICATION.md), [V4.6](V46_SOFTWARE_VERIFICATION.md), [core difference](STAGE5D_CORE_DIFFERENCE_AUDIT.md), [approved V2 execution](APPROVED_V2_EXECUTION.md), [focused correction](FOCUSED_CORRECTION_PASS.md) and [communication audit](COMMUNICATION_LATENCY_AUDIT.md) remain dated evidence. Intermediate six-turn/45-second/resume descriptions do not describe final restored navigation.

The five [PAPER_READY Word documents](PAPER_READY/README.md), diagrams and JSON evidence describe the **18 October documentation snapshot**. Fingerprints/counts and uncommitted wording are not upgraded to November. Later Markdown records govern subsequent changes and supplied measurements. Original Mermaid/image assets remain inventoried without claiming every old diagram is current.

[18 October classification](DOCUMENTATION_INDEX.csv) and [cleanup manifest](FINAL_RELEASE_CLEANUP_MANIFEST_20261018.csv) are historical. Use the new complete inventory for this release. Ignored `docs/archive/`, `.log` files, local preservation receipts and machine paths in old records are provenance references, not missing build inputs/promised public downloads. All dated manifests, binary documents, original figures and historical assertions retain pre-publication bytes.
