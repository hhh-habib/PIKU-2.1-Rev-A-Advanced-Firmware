# Download, build and verify

Version: `revA-standalone-software-checkpoint-20261113`. The source package is complete for the ESP32-S3 firmware build. Historical tests additionally need Git history and host tools.

## Firmware from a source download

Download the versioned GitHub Release source ZIP and extract it, or clone:

```sh
git clone https://github.com/hhh-habib/PIKU-2.1-Rev-A-Advanced-Firmware.git
cd PIKU-2.1-Rev-A-Advanced-Firmware
git checkout revA-standalone-software-checkpoint-20261113
pio run -e esp32-s3-n16r8
```

For ZIP downloads, run the last command inside the extracted folder containing `platformio.ini`. Install PlatformIO Core or its IDE extension first. Initial framework/toolchain/library resolution needs internet. Normal build output is `.pio/build/esp32-s3-n16r8/`.

The delivered `include/Secrets.h` intentionally contains owner-authorized robot SoftAP defaults and is tracked. This package needs no template-copy step. The unchanged example header's ignored/private comment reflects the earlier policy. Keep personal home/cloud credentials outside public commits when customizing deployment.

The profile configures 16 MB flash, `default_16MB.csv`, `qio_opi`, `BOARD_HAS_PSRAM` and environment `esp32-s3-n16r8`. Libraries are pinned in `platformio.ini`; `platform = espressif32` is not pinned. Verified resolved versions are in [release checks](release/verification_20261113.json). Matching source does not guarantee byte-identical future dependency resolution.

To upload to a connected, prepared robot yourself:

```sh
pio run -e esp32-s3-n16r8 -t upload --upload-port COM7
pio device monitor --port COM7 --baud 115200
```

Replace `COM7` with the actual port. Build-only commands do not flash. Release preparation did not upload, operate motors or access hardware. Connect using the delivered SoftAP configuration and address reported by serial/dashboard status; existing STA/Home Wi-Fi provisioning remains available.

## Full host checks

Use a **full Git clone**, Python 3.11+, Git, a native C++ compiler, Deno and Chromium/Chrome/Edge. Capture tests need pyserial (available in the verified PlatformIO Python environment). First use may need access to Deno scripts' declared browser imports. Set `PIKU_HEADLESS_BROWSER` for a custom browser path. Expose `g++` or `clang++` on PATH, or use the existing PlatformIO MinGW compiler on Windows.

```sh
python -B tools/verify_release.py
```

The wrapper reconstructs non-secret V3.2 reference sources from commit `93a44eff1445fb675fdf737645988436b1883cbc` into ignored `.pio/`, then runs the unchanged candidate runner. No sibling/Downloads folder is needed. Logs/results default to `.pio/release-verification/`; browser outputs stay in `.pio/reliability-candidate-evidence/`. Optional `--output .pio/my-verification` selects another ignored result directory. `PIKU_V32_REFERENCE` is an explicit reference override.

Shallow clones lack needed history. GitHub source ZIPs omit `.git`: they build firmware but cannot run historical blob-comparison tests. A full clone supplies those frozen revisions without shipping a duplicate active source tree.

## Interpret results

The supported candidate runner discloses seven superseded snapshot/pixel checks and four absent-CPS skips. Replacement source invariants/live browser/native checks remain active. Exact counts are in [release checks](release/verification_20261113.json). No assertion was removed and publication adds no skip.

The generic historical `python -B tests/run_host_checks.py` remains for provenance and is **not** the passing current entrypoint. Its retained previous run reported 278 tests, 113 failures, 2 errors, 4 skips, including repeated browser subtest and retired-identity mismatches. Do not describe that run as green. The supported wrapper uses documented candidate applicability rules, preserving historical expectations.

Hardware doubles do not prove physical latency, angle accuracy, electrical safety, displacement or RF reliability. No new physical pass follows from a build/tag.

## Identify experimental runs

Record the exact release commit, source manifest, toolchain, full image SHA-256 and device upload/run evidence. Reused runtime label `2.1.0-revA-stage2` cannot uniquely identify a Git revision; it remains unchanged. [Source hashes](release/source_identity_20261113.json) identify production inputs, and [citation guidance](CITING_AND_EVIDENCE.md) distinguishes these from historical deployed images. Repository publication does not establish a DOI or paper acceptance.
