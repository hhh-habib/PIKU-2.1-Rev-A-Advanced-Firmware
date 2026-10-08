# Physically validated standalone freeze - 2026-10-08

This is the current freeze record for the existing PIKU 2.1 Rev A Software Development project. It supersedes the physical-validation-pending status in the earlier restoration report and README for this snapshot; historical records are retained unchanged.

## Commit and tag identity

Annotated tag: `revA-standalone-v32-restored-physical-validated-20261008`.

Reviewed parent: `cbbf3599d0a6192976f40d991250cd38a02c7abb`, on existing branch `codex/reva-collision-closure-20261005`.

The exact creating commit SHA is resolved by `git rev-parse revA-standalone-v32-restored-physical-validated-20261008^{commit}`. The annotated tag message and local post-commit completion receipt record that actual SHA. This document is itself part of the creating commit and cannot embed its own future hash. No second commit or amendment is needed.

## Frozen identity and evidence

**SOURCE VERIFIED.** The 48 non-secret production/configuration files match reviewed fingerprint `3c27ffb727cd1577ca9ba70ae01f20762bd094c88df7753cdb9c1cb5f49aeef3`. The [manifest](STANDALONE_FREEZE_20261008_MANIFEST.json) lists raw and normalized SHA256 values, classification, exact stage allowlist and exclusions. Every staged source/test blob must equal its reviewed raw working-tree bytes. Exact-path `-text` attributes preserve existing CRLF, LF and mixed endings without rewriting source.

**TEST VERIFIED.** Twelve native policies passed; 172 Python tests ran, 168 passed, four CPS-only tests skipped, zero failures/errors. Thirty-four whole chronological V3.2 traces match; nine preserved-interface scenarios include 54 state/event combinations. Actual backend/Chromium STOP/mode ordering and JavaScript parsing passed. Configured ESP32-S3 incremental build passed: RAM 58,908 bytes, flash 1,042,541 bytes. See [verification extract](STANDALONE_FREEZE_20261008_VERIFICATION.txt) and [restoration report](V32_RESTORATION_SOFTWARE_VERIFICATION.md).

**OWNER-REPORTED PHYSICAL PASS.** The owner explicitly confirms that after the October 8 restoration they freshly built and uploaded firmware from this existing project, then extensively tested demanding obstacle configurations and autonomous stuck/boxed-in recovery, including the reported chair-leg scenario. The owner reports all tested scenarios passed and substantially better behavior than the earlier V4.6 navigation candidate. Source-to-flash provenance is supported by that confirmed workflow.

**FLASHED BINARY HASH NOT INDEPENDENTLY VERIFIED.** No robot connection/readback, instrumented latency/clearance/collision/runtime measurements, numerical physical test count or universal safety certification is claimed. The local on-disk firmware artifact SHA256 is recorded as build-artifact metadata only.

## Frozen behavioral decisions

V3.2 unified ordinary AUTO obstacle handling and active stuck/boxed recovery remain unchanged: finite front <=20 cm blocks, eligible ordinary finite side >20 cm, recovery side <=7 cm is vetoed, finite recovery front >=30 cm or accepted OPEN exits. Seven started recovery turns, elapsed 30 s mission, five nominal 200 ms pulses, 2,500 ms cooldown and automatic retries remain frozen. Original OPEN/No Echo interpretation and existing dashboard STOP/mode ordering are unchanged.

The two approved environmental decisions remain:

1. Hazard interruption restores V3.2 abort and mission reset. Reference abort retains its attempts diagnostic while clearing active side/turn/history/pulse state.
2. Sonar interruption preserves V4.6 unfinished environmental-job ownership and requires a fresh environmental scan after reacquisition; this is the explicit parity exception.

No firmware algorithm, interface, timer, threshold, motor output, sensing, safety, networking, dashboard or PlatformIO configuration changed during the freeze.

## Build and reproduction

Environment `esp32-s3-n16r8`: `esp32-s3-devkitc-1`, Arduino, 16 MB flash/`default_16MB.csv`, `qio_opi`, `BOARD_HAS_PSRAM`. The configured board defaults are overridden by unchanged project memory settings. Use the manifest's actual verified platform/framework/toolchain/library versions; `platform = espressif32` itself is unpinned. No dependency or configuration was changed.

Provide unchanged private `include/Secrets.h` locally, outside the freeze Git snapshot. The existing preservation tests pin its hash; the complete suite is qualified by that private prerequisite. Historical private contents may remain in ancestor Git history; no history rewrite or push was performed.

The parity comparator uses the read-only reference at its recorded absolute path. Sixty-one non-secret reference source/test/config files matched frozen V3.2 executable commit `41281d8325d88c70e074e008a2550a1164102132` after CRLF normalization; 90 reference files remained unchanged. Existing historical static/browser guards require the Git objects listed in the manifest. Preserve full local history and provide the existing native/MSVC, Python, Deno and Chromium tools. A shallow checkout or source archive alone does not reproduce every historical guard.

Software models and an incremental build do not establish bit-for-bit rebuild determinism or independently prove the flashed image. Keep private configuration, toolchain and reference prerequisites controlled. Owner physical PASS is qualitative evidence under tested conditions.

## Preservation and handoff

The explicit 117-path stage allowlist includes reviewed source/configuration, 58 active tests/support files, the trace comparison/capture helper, current README, essential restoration evidence, this record/manifest/verification and minimal Git metadata. Existing tracked historical material remains inherited unchanged. The private header is removed only from the index and remains byte-identical locally. Unrelated untracked historical reports, cached Python files, PNG assets and local evidence remain preserved and unstaged.

One local freeze commit and one annotated tag are authorized. No branch switch, reset/clean/stash, firmware rewrite, cleanup, push, robot flashing, Stage 4 or Node V1 work is part of this freeze. Further cleanup and integration require separate authorization.

Staged whitespace review is CRLF-aware because raw line endings are preserved. Two existing blank EOF lines in the backend test fixtures are retained unchanged; this freeze performs no cleanup. All intended staged source/test blobs must match the reviewed raw hashes.
