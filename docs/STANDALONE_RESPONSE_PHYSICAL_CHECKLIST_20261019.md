# Standalone response candidate - owner physical acceptance

Status: NOT RUN. Blank results are unknown, not passes or zeros. This checklist covers the standalone robot only. Software checks/build passed; they do not establish physical performance. Do not merge/push or begin Stage 4 migration on the strength of host tests alone.

Use the project's existing supervised robot and environmental-test procedure. The assistant has not uploaded firmware, opened a hardware serial port or moved the robot. The owner chooses when to upload and test.

## Identify each run

Keep the earlier GitHub firmware as baseline A and the repaired standalone as candidate B. Preserve local credentials/NVS; do not factory-reset between runs. Use the same robot, phone/browser, router/AP, location/orientation, obstacle arrangement, speed and comparable battery/load. Prefer A -> B -> A order to expose changing conditions.

Record privately:

| Item | Baseline A | Candidate B |
| --- | --- | --- |
| Firmware/source identity and upload time | | |
| Run date/time; phone/browser/router | | |
| Battery/load; speed; location | | |
| STA address/AP/channel; network conditions | | |
| Last reset reason / uptime | | |

Candidate source fingerprint: `c979e98901bd6b8e634f08c9581b30eb7930a2a0ece0ba79c899673d2ec45ef4`.

Local build binary SHA-256: `0e17297e4f77e78a4157d1492bb2b291ee348b4a5c86fe3236e26adb8e0d6662`. This is not a read-back verification of the flashed image. Do not publish configured firmware binaries or credentials.

## Test sequence

1. **Home Wi-Fi and page load:** open all dashboard panels on STA. Check normal load and repeated reload. Record Live/Stale/Offline transitions, whether requests still succeed, and STA disconnect count/reason. Live now requires fresh telemetry plus recent successful HTTP response; Stale means telemetry is old while the node still responds; Offline means no recent successful response. None alone grants movement permission.
2. **Select MANUAL once:** repeat Forward10s -> release -> Left1s -> release -> Forward10s -> release three times under the existing supervised motion procedure. Each held action should work while current commands arrive; release should stop; the next fresh direction should work without another MANUAL click. Record every unexpected stop with its displayed inhibit/stop reason and time.
3. **STOP and ordinary HTTP load:** check operator STOP and release during normal telemetry and asset download/reload load. Use an asset-only request for extra load; opening another full controller can legitimately replace the control session and stop old movement. Do not label deliberate session takeover as a connectivity defect. Measure physical stop independently if reporting latency; HTTP acknowledgment is not physical stop proof.
4. **Temporary loss:** under supervision, exercise 5s,10s and15s network outages. Movement must stop on real command loss. After connectivity returns and STOP/current authority are confirmed, the selected MANUAL mode should become ready for a fresh direction without redundant MANUAL selection. Old held input must never replay. Record whether an unresolved STOP, explicit revocation, another owner or an active hazard correctly kept controls inhibited.
5. **Priority alert:** using the existing supervised procedure, confirm flame/gas detection, applicable motor stop, dashboard alert and OLED alert. Check an alert immediately after an ordinary OLED refresh and during held control. New/changed/cleared critical alerts should not wait for the ordinary500ms OLED interval. Browser presentation still depends on sensing/debounce and successful telemetry. During intentionally permitted Manual override the alert should remain visible and say HAZARD ACTIVE, not falsely claim motors stopped.
6. **Ordinary AUTO:** compare clear forward movement, front obstacle, single/both IR, reverse/full scan/turn/front re-evaluation and automatic continuation with A. No new unsolicited initial scan or change to the established navigation policy is intended.
7. **Boxed recovery and sonar:** verify locked-side recovery, selected-side<=7cm veto, fresh>=30cm/OPEN exit, per-mission seven-turn/five-pulse/30s limits and2.5s retry cooldown. Test sonar fault/reacquisition and environmental recovery using the established procedure. These configured limits are not measured angle/distance guarantees or lifetime retry bounds.
8. **Soak and comparison:** run comparable idle/Manual/AUTO sessions for20-30min where practical. Record unintended stops, mode/session interventions, response stalls, page-load failures, resets and actual STA events. Compare typical and tail response times when measured; do not treat RSSI bars, maximum-loop-gap telemetry or subjective smoothness as a latency distribution.

## Results

| Test | A outcome / observations | B outcome / observations | B pass/fail/pending |
| --- | --- | --- | --- |
| STA/page loading | | | |
| One-MANUAL held/release sequence | | | |
| STOP with HTTP load | | | |
| 5s/10s/15s outage recovery | | | |
| Flame/gas/OLED/dashboard alert | | | |
| Ordinary AUTO | | | |
| Boxed/sonar/environment recovery | | | |
| Soak/no unexplained interruptions | | | |

For each problem, return: test number, mode/action, symptom, exact stop/inhibit reason, dashboard request outcomes, STA disconnect count/reason, uptime/reset changes and whether a second controller was open. Keep network credentials and session tokens private.

Acceptance requires candidate B to preserve baseline behavior and show no worse control/connectivity under comparable conditions, with no stale movement after STOP/reconnect and preserved monitoring/recovery. Investigate failures before release. Owner qualitative results are OWNER-REPORTED PHYSICAL TESTING; quantitative performance claims require actual measurements with instruments/methods and exclusions recorded.

## Related evidence

[Software verification and recovery information](STANDALONE_RESPONSE_SOFTWARE_VERIFICATION_20261019.md). General README/PAPER_READY synchronization and manuscript work remain a later phase after physical acceptance.
