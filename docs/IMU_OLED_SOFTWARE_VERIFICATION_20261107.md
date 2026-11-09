# Standalone IMU and compact OLED — 2026-11-07

Status: **software verification passed; physical validation NOT RUN**. Owner
authorized the combined IMU/OLED mission. This record describes this working-tree
delta, not the historical physically validated V3.2 image. No AI or new research
performance claim is made.

## Scope and preservation

Only eight production files changed: `include/ImuManager.h`,
`src/ImuManager.cpp`, `include/DisplayManager.h`, `src/DisplayManager.cpp`,
`include/WebDashboard.h`, `src/WebDashboard.cpp`, `src/DashboardPublisher.cpp`
and `src/main.cpp`. Existing dirty work was preserved before editing. The exact
before/after hashes and protected production hashes are in the separate
`imu_oled_20261107` layer of
[`reliability_candidate_review.json`](../tests/reliability_candidate_review.json).
Historical October identities remain historical; they were not relabeled as
current physical evidence.

Navigation/recovery algorithms, motor control, servo scan, sensor acquisition,
flame/gas/temperature thresholds and persistence, buzzer, radio packets/peer
logic, HTTP/control handlers, dashboard assets/polling, Wi-Fi reconnect policy,
pins, build profile and private configuration were preserved. Main changes are
limited to passing the existing motor-stopped state into IMU update, copying
actual-sample validity, and computing IMU age across a zero timestamp. The
network bridge reads the existing STA/IP cache; it does not query or configure
Wi-Fi. Publisher sensor assignments remain unchanged.

The pre-edit nonprivate working-tree archive is
`Research/Continuity/preservation/PIKU_IMU_OLED_20261107T005817Z/working-tree.zip`
(Development-relative), SHA-256
`8f0a059714d9914482c4ed35fb8637e7e6af8def68a4fc281af6672e801b0ef2`.
It excludes private configuration and `.git`/`.pio`. Protected private files
were compared by hash only. No private content was copied into the archive.
Mission: `mission-20261107-065827-1c4b363c`.

## Verified defects and implemented behavior

The installed Adafruit MPU6050 2.2.6 implementation does not propagate its
14-byte bus-read result through `getEvent()`. The previous firmware trusted that
success and finite values; a finite all-zero event could renew HEALTHY and the
sample timestamp. It also permanently returned from `update()` after boot
initialization failed. Before editing, the actual previous ImuManager source
with a deterministic event double reproduced `all_zero_frame_accepted_healthy=1`
and `boot_absence_recovered=0`. This is a software defect reproduction, not a
measurement of the owner's electrical fault or proof of its sole root cause.

The project-owned MPU6050 transport now checks WHO_AM_I, bus results and exact
byte counts; it accepts only a complete INT_STATUS/data-ready sensor burst.
Empty-ready, all-zero and all-ones payloads do not renew the sample timestamp.
Zero gyro and repeated stationary readings with gravity remain valid. The same
25 ms requested cadence, ±4g/±500dps ranges, 21 Hz filter, units and valid-frame
roll/pitch/motion calculations remain. MPU6050 library files and dependency
versions were not modified.

Initialization/recovery progresses across service passes, with timed reset
waits instead of `delay()`/reset polling loops. The IMU service performs at most
one checked register read/write transaction per call. Each Wire operation uses
a temporary 5 ms driver timeout and restores the previous timeout; a read has
both address and receive operations. This is not a measured whole-loop latency
bound. Identity, power/range/filter readback and three new valid frames are
required before HEALTHY. Failed boot discovery and repeated runtime read faults
retry with 500 ms exponential backoff capped at 8 s.

Normal sampling and transient read retry continue during movement. Reset and
configuration writes wait until the existing motor state is Stopped. This
manager issues no motor commands, new stop, Manual lock, mode change or session
request. Until the first checked frame, JSON IMU age is `null` and the existing
dashboard shows `--`. A fault retains the last successful values/timestamp with
unhealthy status, allowing their age to grow honestly. Existing navigation
health gates/fallbacks continue to apply.

## OLED behavior

The normal 128×64 screen has seven fixed information rows; values below are
illustrative, not a physical measurement:

```text
PIKU 2.1 Rev A
DIST: 45.0 cm
T:28.4C H:65%
GAS: LOW (850)
FLAME: CLEAR
RADIO: DISCONNECTED
STA: 192.168.43.120
```

Gas uses the existing relative classification and raw ADC value, **not ppm**.
Warm-up and unavailable readings are identified. Radio CONNECTED requires the
existing recent peer ONLINE evidence plus an enabled, initialized, detected
module; module presence alone is insufficient. This is not a new radio session
or packet protocol. STA displays the robot's actual cached address, updates
after association/address changes, and displays DISCONNECTED after loss. No
normal AP, IMU or rotating status pages are shown.

Routine display evaluation is capped at one second and unchanged visible text
does not transfer a new frame. A fixed 176-byte text cache avoids storing a
second telemetry snapshot. Confirmed environmental alert entry, a changed set
of active hazards, and clear bypass that cap and the existing busy-output
deferral. Simultaneous confirmed flame/gas/critical-temperature hazards appear
together; distance/IR never creates a full-screen overlay. This changes display
presentation, not environmental confirmation/debounce timing or obstacle motor
protection. Sensor acquisition and browser polling remain unchanged. Reduced
display work does not establish stronger RF signal or measured CPU headroom.

## Verification evidence

- Unchanged baseline: 13 native policy executables; 248 discovered tests,
  **237 passed**, 0 failures/errors, 11 existing skips.
- Full changed-source regression run: 13 native policies; 277 discovered tests,
  **266 passed**, 0 failures/errors, the same 11 skips. Existing host navigation,
  service dispatch, browser/control and response tests execute live source.
- Final focused run: **30/30 passed**; 29 overlap that full run and one final
  exact bridge-preservation check was added afterward. Actual IMU/OLED C++ is
  compiled against deterministic Wire/OLED doubles. Cases cover missing sensor
  at boot, both addresses, wrong identity, short/NACK reads, zero/FF frames,
  absent data-ready, transient retry while moving, idle-only reset, failed
  readback, stuck reset timeout, bounded backoff, clock wrap, zero timestamp,
  conversion units, stationary readings, layout, redraw suppression, alerts,
  radio/STA transitions, invalid readings and the live cached-network bridge.
- Final `pio run -e esp32-s3-n16r8`: **SUCCESS**, Arduino ESP32 2.0.17;
  static RAM 61,188 / 327,680 bytes; flash 1,061,465 / 6,553,600 bytes. These
  linker figures do not measure runtime CPU load, electrical timing or RF.
- Windows-aware whitespace check: `git -c core.whitespace=cr-at-eol diff --check`.
  Private headers and other protected project files were compared by hash.

Re-run software checks from this folder:

```powershell
python -B tools/verify_reliability_candidate.py
python -B -m unittest discover -s tests -p test_imu_oled.py -v
pio run -e esp32-s3-n16r8
```

Generated local evidence is under `.pio/reliability-candidate-evidence/` and
`.pio/imu-oled-20261107/`; `.pio` is ignored. The configured firmware image is
local and must not be treated as a credential-free publication artifact.
No robot was flashed or operated. No files in the earlier GitHub, Physical
Development/paper, PIKU Stage4, Coordinator or Node standalone project were
changed. Branch/HEAD and staged contents remained unchanged. The raw standalone
Git index hash refreshed while its staged diff stayed empty; a new application
`refs/codex/turn-diffs/captures/.../base` ref was observed in Coordinator, with
its worktree, branch, HEAD and index unchanged. Neither was reset or rewritten.

## Pending owner physical acceptance

1. Check several cold boots. Before a valid IMU frame, age must be `--`, never
   false fresh HEALTHY. After initialization, move/tilt the body under supervision
   and observe changing accel/gyro values. Stationary zero gyro is normal.
2. Check Manual directions/release and STOP, then normal AUTO obstacles and the
   previously reported dual-IR block/clear case in a controlled clear area.
   Record gyro Z, IMU health/age, target/current turn and completion reason.
   Stop the trial with existing controls if repeated large spins reappear.
3. Check the compact normal screen, STA address changes/disconnect indication,
   and radio module-only versus recent peer ONLINE indications. Confirm ordinary
   distance warnings do not replace the screen.
4. Verify environmental alert entry/clear with the existing supervised test
   method. The new display adds no routine one-second wait after a hazard is
   confirmed; existing sensing/persistence and service-loop latency still apply.
5. If an IMU fault recurs, record the dashboard values/ages and whether recovery
   occurs during an existing stopped interval without reboot. Do not unplug or
   short a powered I2C bus to force this test.

Physical freezing with plausible repeated samples, wiring/power faults or a
stuck bus may still require electrical diagnosis. This patch cannot guarantee
always-fresh hardware, eliminate all overspin causes, or establish Wi-Fi/radio
range. Ordinary turn/fallback and recovery policy were deliberately preserved;
the reported overspin outcome remains **UNVERIFIED** until physical acceptance.
