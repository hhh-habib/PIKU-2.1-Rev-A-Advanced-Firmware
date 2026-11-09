# PIKU standalone documentation

Start with [PAPER_READY](PAPER_READY/README.md): five current-source-backed editable Word documents for manuscript preparation. Current source remains the restored V3.2 navigation plus the October 18 sonar/control/readability corrections. Historical version names and test counts below describe their own dated snapshots.

## Current implementation and verification

- [Current source comparison and release audit](FINAL_RELEASE_REVIEW_20261018.md)
- [Previous conservative cleanup manifest](FINAL_RELEASE_CLEANUP_MANIFEST_20261018.csv)
- [Navigation trace format](NAV_TRACE_SCHEMA.md)
- [Complete file classification and path mapping](DOCUMENTATION_INDEX.csv)
- [Frozen identity and evidence](STANDALONE_FREEZE_20261008.md)

The latest good physical operation is **OWNER-REPORTED PHYSICAL TESTING**. Software tests and build resource values are separate evidence. No new latency, turn-error, displacement or RF benchmark was measured during documentation work.

## Archived intermediate development reports

[Archive index and byte-preserving move hashes](archive/README.md). Two unpinned October 18 development reports moved to `archive/development_history/`; no original report was rewritten. The standalone reliability report stays at its original path because its relative screenshot links depend on that location. The Word package supersedes these for current reading while retaining their unique failure/reproduction history.

## Historical evidence retained at original paths

Most older reports are explicitly named by historical/frozen manifests or documentation. Their original paths and bytes remain; they are organized here by subject rather than moved or duplicated.

## Physical observations and methodology

- [PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md](PIKU_2.1_REVA_V32_PHYSICAL_VALIDATION.md)
- [PIKU_2.1_REVA_V33_PHYSICAL_VALIDATION.md](PIKU_2.1_REVA_V33_PHYSICAL_VALIDATION.md)
- [PIKU_2.1_REV_A_TESTING.md](PIKU_2.1_REV_A_TESTING.md)
- [TESTING.md](TESTING.md)
- [PIKU_2_1_REV_A_STAGE1.md](PIKU_2_1_REV_A_STAGE1.md)
- [PIKU_2_1_REV_A_STAGE2.md](PIKU_2_1_REV_A_STAGE2.md)

## Historical algorithms and verification

- [APPROVED_V2_EXECUTION.md](APPROVED_V2_EXECUTION.md)
- [COMMUNICATION_LATENCY_AUDIT.md](COMMUNICATION_LATENCY_AUDIT.md)
- [FOCUSED_CORRECTION_PASS.md](FOCUSED_CORRECTION_PASS.md)
- [RECOVERY_V3_SOFTWARE_VERIFICATION.md](RECOVERY_V3_SOFTWARE_VERIFICATION.md)
- [RECOVERY_V31_SOFTWARE_VERIFICATION.md](RECOVERY_V31_SOFTWARE_VERIFICATION.md)
- [RECOVERY_V32_SOFTWARE_VERIFICATION.md](RECOVERY_V32_SOFTWARE_VERIFICATION.md)
- [RECOVERY_V33_SOFTWARE_VERIFICATION.md](RECOVERY_V33_SOFTWARE_VERIFICATION.md)
- [V33_1_HARDENING_SOFTWARE_VERIFICATION.md](V33_1_HARDENING_SOFTWARE_VERIFICATION.md)
- [STAGE5D_CORE_DIFFERENCE_AUDIT.md](STAGE5D_CORE_DIFFERENCE_AUDIT.md)
- [V45_SOFTWARE_VERIFICATION.md](V45_SOFTWARE_VERIFICATION.md)
- [V46_SOFTWARE_VERIFICATION.md](V46_SOFTWARE_VERIFICATION.md)

## Collision and state arbitration evidence

- [COLLISION_CLOSURE_F4_LEDGER.md](COLLISION_CLOSURE_F4_LEDGER.md)
- [COLLISION_CLOSURE_MATRIX_GUIDE.md](COLLISION_CLOSURE_MATRIX_GUIDE.md)
- [COLLISION_CLOSURE_PROTECTION.json](COLLISION_CLOSURE_PROTECTION.json)
- [COLLISION_CLOSURE_REPRODUCTION.json](COLLISION_CLOSURE_REPRODUCTION.json)
- [COLLISION_CLOSURE_SOFTWARE_VERIFICATION.md](COLLISION_CLOSURE_SOFTWARE_VERIFICATION.md)
- [COLLISION_CLOSURE_STATE_EVENT_MATRIX.csv](COLLISION_CLOSURE_STATE_EVENT_MATRIX.csv)
- [SYSTEM_STATE_COLLISION_VERIFICATION.md](SYSTEM_STATE_COLLISION_VERIFICATION.md)

## Restoration and immutable freeze evidence

- [V32_RESTORATION_AVAILABILITY.log](V32_RESTORATION_AVAILABILITY.log)
- [V32_RESTORATION_BASELINE_BUILD.log](V32_RESTORATION_BASELINE_BUILD.log)
- [V32_RESTORATION_BASELINE_TESTS.log](V32_RESTORATION_BASELINE_TESTS.log)
- [V32_RESTORATION_BUILD.log](V32_RESTORATION_BUILD.log)
- [V32_RESTORATION_CONTINUITY_UPDATE.json](V32_RESTORATION_CONTINUITY_UPDATE.json)
- [V32_RESTORATION_FINAL_DIFF_STAT.txt](V32_RESTORATION_FINAL_DIFF_STAT.txt)
- [V32_RESTORATION_FINAL_GIT_STATUS.txt](V32_RESTORATION_FINAL_GIT_STATUS.txt)
- [V32_RESTORATION_FINAL_SOURCE.diff](V32_RESTORATION_FINAL_SOURCE.diff)
- [V32_RESTORATION_FIRST_TESTS.log](V32_RESTORATION_FIRST_TESTS.log)
- [V32_RESTORATION_HOST_CHECKS.log](V32_RESTORATION_HOST_CHECKS.log)
- [V32_RESTORATION_PARITY.log](V32_RESTORATION_PARITY.log)
- [V32_RESTORATION_PREFLIGHT_BASELINE.json](V32_RESTORATION_PREFLIGHT_BASELINE.json)
- [V32_RESTORATION_PREFLIGHT_REPORT.md](V32_RESTORATION_PREFLIGHT_REPORT.md)
- [V32_RESTORATION_SOFTWARE_VERIFICATION.md](V32_RESTORATION_SOFTWARE_VERIFICATION.md)
- [V32_RESTORATION_SOURCE_AUDIT.json](V32_RESTORATION_SOURCE_AUDIT.json)
- [V32_RESTORATION_TESTS.log](V32_RESTORATION_TESTS.log)
- [V32_RESTORATION_TRACE_SUMMARY.json](V32_RESTORATION_TRACE_SUMMARY.json)
- [STANDALONE_FREEZE_20261008_MANIFEST.json](STANDALONE_FREEZE_20261008_MANIFEST.json)
- [STANDALONE_FREEZE_20261008_VERIFICATION.txt](STANDALONE_FREEZE_20261008_VERIFICATION.txt)

## Figures tests and source provenance

The six original owner PNGs remain in `figures/`; four original Mermaid files remain in `architecture_source/`. The recovery diagram and some image labels describe historical behavior. New current software diagrams are clearly identified inside PAPER_READY; originals were not edited. All 67 test assets and both tools retain their original paths and bytes. The active source-verification manifest and historical assertions remain unchanged.

The two direct documentation dependencies read/written by the current verification stack are `STANDALONE_FREEZE_20261008_MANIFEST.json` and `V32_RESTORATION_TRACE_SUMMARY.json`. The unchanged candidate runner redirects newly generated traces away from the historical summary. Complete indirect/baseline dependencies are recorded in DOCUMENTATION_INDEX.csv.

## Organization verification 18 October 2026

The classification index records 133 inspected items: 4 current essential, 45 historical research evidence, 2 safely archived, 71 active test dependencies, 10 uncertain preserved and 1 private configuration entry whose content is excluded. The original 50 documentation assets, 67 test assets, 2 tools, 4 Mermaid sources, 6 owner figures and 3 root documents are individually listed. Both archived reports match their verified pre-documentation backup hashes.

All original production/configuration/test/tool/figure bytes, local private configuration and frozen Git identity are preserved. README gained only the navigation pointer above its original bytes. Existing working-tree changes and two originally missing figure paths remain as found. No other project changed. Both complete software-suite/build runs passed with unchanged results and binary identity; see PAPER_READY Document 04 and its compact evidence. All five Word documents opened, rendered and passed visual review across 38 pages. Current local Markdown and package links resolve; package privacy checks passed. Historical source wording and separate publication-security findings remain qualified rather than rewritten. Private preservation and rendering receipts are recorded in Research Continuity V1.
