# PIKU 2.1 Rev A ? standalone robot firmware

Complete ESP32-S3 source, embedded browser dashboard, host tests, architecture assets and dated research evidence for the PIKU environmental-inspection robot. This is the **13 November 2026 standalone software checkpoint**. Stage 4 CPS integration is a separate project.

The current implementation combines restored V3.2 AUTO/recovery with subsequent sensor-evidence, Manual/session/STOP, HTTP/Wi-Fi, dashboard, IMU and OLED corrections. Intermediate V3.3/V4.5 reports retain their historical scope.

| Start here | Contents |
|---|---|
| [Build and reproduce](docs/REPRODUCIBILITY.md) | Download, dependencies, build/upload instructions and host checks |
| [How the firmware works](SOFTWARE_EXPLANATION.md) | Current control, navigation, sensing, communication and display |
| [Documentation guide](docs/README.md) | Current guides and preserved software/physical evidence |
| [Release record](docs/RELEASE_20261113.md) | Scope, preservation and verification results |
| [Paper and citation](docs/CITING_AND_EVIDENCE.md) | Versioned citation and experiment/build provenance |
| [Complete file inventory](docs/REPOSITORY_CONTENTS_20261113.md) | Every delivered file and its role |

## Download and build

Download the versioned GitHub Release source ZIP, or clone this repository. Open the extracted folder containing `platformio.ini` with PlatformIO, or use PlatformIO Core:

```sh
git clone https://github.com/hhh-habib/PIKU-2.1-Rev-A-Advanced-Firmware.git
cd PIKU-2.1-Rev-A-Advanced-Firmware
git checkout revA-standalone-software-checkpoint-20261113
pio run -e esp32-s3-n16r8
```

The package includes `include/Secrets.h` with the owner's intentionally published **robot SoftAP defaults**, preserved unchanged. A download therefore contains the configuration for this checkpoint. Personal home Wi-Fi/cloud credentials are outside that authorization. Use the delivered header; `Secrets.example.h` remains an alternative historical template whose private/ignored-header comment reflects the former policy.

Firmware builds require neither Git history nor a sibling checkout. Full historical tests require a full clone; see [reproduction](docs/REPRODUCIBILITY.md). Precompiled firmware is not distributed by this release.

## Hardware

| Component | Role |
|---|---|
| ESP32-S3 N16R8 | Cooperative control loop, Wi-Fi and dashboard |
| L298N / four-wheel chassis | Direction and selected PWM speed |
| SG90 / HC-SR04 | Scan-head positioning and directional ranging |
| Two low front IR sensors | Local close-chassis obstacle evidence |
| MPU6050 | Checked samples and short-term relative-turn estimate |
| DHT22 / MQ-2 / flame module | Environmental monitoring and hazard handling |
| SH1106 OLED | Compact status and environmental alerts |
| nRF24 | Secondary experimental radio path |

Pins and executable thresholds are in [PinConfig.h](include/PinConfig.h) and [RobotConfig.h](include/RobotConfig.h). The `esp32-s3-n16r8` profile customizes the board for 16 MB flash and OPI PSRAM.

## Current behavior

- **Manual:** select Manual, hold a fresh direction, release to stop. Current session/sequence rules and serviced 750 ms deadman remain. Confirmed STOP/RELEASE can restore readiness for a new press after uncertainty; old held motion is never replayed. A Live badge does not grant movement authority.
- **AUTO:** trusted directional sonar/front IR feed restored reverse/scan/turn navigation. Normal turns have a nominal 35-degree target. STOP, obstacle, environmental and authority gates retain priority.
- **Recovery:** commit a usable side; use nominal 25-degree increments, at most seven started turns and five IR reverse pulses in a 30,000 ms elapsed mission. Exhaustion stops; a 2,500 ms cooldown permits fresh retry. These are per-mission bounds, not an escape guarantee.
- **Evidence:** invalid/stale sonar stops AUTO execution and requires fresh reacquisition. Interrupted actuator work is discarded. Missing DHT/MQ-2 monitoring alone does not inhibit movement; failed reads cannot clear a confirmed hazard. Unfinished environmental recovery needs renewed evidence.
- **IMU/OLED:** checked identity/byte counts/data-ready frames, honest sample age and cooperative fault recovery; reset/configuration waits for stopped motors. Compact fixed OLED rows and priority environmental alerts use cached STA/peer evidence.
- **Communication:** bounded HTTP framing/writes and STA retry handling preserve control responsiveness. They do not establish measured whole-loop latency or RF guarantees. Robot-local motion/safety authority remains standalone.

See the [current software schematic](architecture_source/current_standalone_20261113.mmd). Original images and historical diagrams remain preserved and classified.

## Verification and physical evidence

Fresh counts and build results are in the [release record](docs/RELEASE_20261113.md) and [verification summary](docs/release/verification_20261113.json). The supported suite retains four absent-CPS skips and seven superseded snapshot/style checks. The generic historical runner is not represented as passing; test categories overlap.

Physical evidence is separate: historical V3.2 operation is owner-reported. November records include T01?T20, separate T21?T22 corridor observations and 40 recorded turn-angle measurements. Their protocols/limits remain in [Task 3](docs/RESEARCH_AUDIT/07_TASK_03_METHODOLOGY_AND_TURN_ANGLES_20261109.md). Exact experiment-to-deployed-image mapping is **not independently verified**. This release cannot retroactively identify those binaries.

The robot lacks rear sensing and encoder-confirmed reverse distance. Gas values are relative/raw, not calibrated ppm. Clear/no-echo is not proof of unlimited free space. This is a supervised research prototype.

## Citation and contents

Cite this release and its exact commit, rather than moving `master`. [CITATION.cff](CITATION.cff) and [citation guidance](docs/CITING_AND_EVIDENCE.md) identify the version and evidence boundaries. No DOI is claimed without an issued archive record. No software license has been selected; contact the maintainer about reuse terms.

`src/`, `include/`, `platformio.ini` are the complete firmware/build inputs. `tests/` preserves regressions/hardware doubles; `tools/` provides trace capture and verification. `docs/`, `figures/` and `architecture_source/` preserve documentation, evidence and owner assets. Build caches, generated binaries, machine state and large raw recordings remain outside the source package. Earlier README/guide versions remain in Git history at `8402b462c7b3f9ceaa09875c0ae7335176222887`; dated evidence stays at its existing paths.
