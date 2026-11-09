# Standalone responsiveness repair - 2026-10-19

Software verification: PASS. Physical acceptance: NOT RUN. This is a candidate for owner-supervised testing, not a physically accepted release. General documentation synchronization, paper evidence updates, merge/push and Stage 4 migration remain later work.

## Scope and identity

Only `PIKU 2.1 Rev A Software Development` firmware/tests changed. Earlier GitHub firmware remains the read-only behavior baseline. HEAD remains `fa375a7bf739fc306060048b500c2acf413db7a7` on `codex/reva-collision-closure-20261005`; existing dirty files and absent owner figure paths are preserved. The working source, not HEAD alone, identifies this candidate.

- Current 49-file non-secret production fingerprint: `c979e98901bd6b8e634f08c9581b30eb7930a2a0ece0ba79c899673d2ec45ef4`.
- Local configured build binary SHA-256: `0e17297e4f77e78a4157d1492bb2b291ee348b4a5c86fe3236e26adb8e0d6662`. This does not identify the robot's flashed image. No configured binary is published or copied into this report.
- Raw per-file identities/results: `../.pio/standalone-response-20261019/verification-summary.json` and `../tests/reliability_candidate_review.json`.

## Reproduced causes and fixes

| Cause inspected/reproduced | Implemented behavior |
| --- | --- |
| Fresh accepted held directions did not renew their live control session. Ping/data delays could expire it despite current command delivery. The new actual-handler held-session case failed before the patch. | An accepted direction with the current still-live valid token renews session liveness. Rejected/stale sequences, other tokens and expired/revoked sessions cannot renew it. STOP/RELEASE are excluded. The independent movement deadman remains 750 ms. |
| Fresh telemetry was classified Offline when ping alone was old. The actual Chromium probe failed before repair. | A recent successful ping/data/control/mode response proves HTTP node response; fresh telemetry is separately required for Live. Old telemetry with a successful control response is Stale. No recent response remains Offline. Status never restores movement authority. |
| A critical OLED alert could be skipped during busy control and by the routine display interval. | Confirmed critical entry, changed reason and clear receive a transition-only display bypass. Repeated identical alerts do not flood OLED. The alert page says HAZARD ACTIVE, because valid Manual override can permit movement. Sensor filtering/detection rules are unchanged. |
| Ordinary/flash HTTP responses delegated to framework write retries; fragmented input could enter blocking parsing. | Newly accepted input waits across service passes for a complete bounded frame. Absolute admission window250ms; header2048 bytes, body1024 bytes; duplicate length, unsupported transfer-encoding and malformed/oversized frames expire without parsing. Shared write budgets are control25ms, telemetry125ms and flash250ms. Failed sockets are retired once. |
| Framework steady auto-reconnect and application retries overlapped; retries could interrupt association or scan. | Disable steady framework auto-reconnect. Preserve an active attempt for up to15000ms; terminal failures end it and normal backoff can retry. Do not retry during an active scan. Application retries use association begin rather than forced disconnect/reconnect. The framework's built-in first-connect retry can still occur. |
| Routine output competes for service time. | Ordinary OLED interval500ms and routine serial log2000ms. Urgent alerts bypass ordinary output deferral. All sensor acquisition, main-loop safety/motor dispatch and navigation source are byte-identical to pre-edit. |

These source/model results do not establish the cause of every owner-observed interruption or measured Wi-Fi/RF improvement. Browser alert latency still includes sensing/debounce and the next successful HTTP telemetry response. A fixed2049-byte request-peek buffer is allocated with the server, avoiding this scratch allocation on the Arduino loop stack.

## Software verification

| Check | Unchanged baseline | Final candidate |
| --- | --- | --- |
| Applicable Python suite | 208 passed, 11 skips / 219 | 237 passed, 11 skips / 248 |
| Failures/errors | 0/0 | 0/0 |
| Native policy executables | 13 | 13 |
| New live-body response/scope cases | Not present | 29 passed |
| Actual Chromium behavior | 82 recorded in baseline | 85 passed |
| Layout cases | 75 | 75 passed |
| Baseline navigation trace parity | 34 traces /3161 modeled passes | 34 identical traces /3161 modeled passes |
| ESP32-S3 build | PASS | PASS |
| Static RAM | 58996 bytes | 61068 bytes (+2072) |
| Application flash | 1063553 bytes | 1064757 bytes (+1204) |

Counts overlap; do not add them as independent experiments. Native clock/socket/OLED doubles test actual production bodies but do not measure hardware latency, power, STA/RF reliability, turn angle or motor stop.

The first baseline invocation encountered a missing pyserial environment and relocated Downloads reference path. Existing PlatformIO Python and an in-memory reference relocation then passed the unchanged baseline. No host package installation was needed. The reference test now resolves the existing sibling baseline (override with PIKU_V32_REFERENCE). One legacy backend fixture needed the new heartbeat member declaration; no assertion was removed. New before/after behavior tests failed against the archived pre-edit source and pass against current source. The intermediate full-suite fixture compile error was corrected before the final passing run.

The 11 inherited skips remain unchanged: four absent-CPS cases and seven historical identity/pixel checks with previously documented replacements. No new skip or broad assertion disable was added. Exact candidate hashes and live-body invariants were extended only for the reviewed changes. Navigation, sensors, safety, IMU, motor, servo, radio, pin/private Wi-Fi profile code and main.cpp are explicitly hash-guarded. Only declared output/network constants differ.

Reproduction/commands:

```powershell
& 'E:\Development\PlatformIO\penv\Scripts\python.exe' -B tools/verify_reliability_candidate.py
pio run -e esp32-s3-n16r8
```

The runner redirects its historical trace-summary write to ignored evidence. Earlier GitHub files and other projects are not test-output destinations. Reproduction failures and final logs are retained in `../.pio/standalone-response-20261019/`; the original pre-existing generated evidence was privately preserved before tests.

## Preserved behavior and limitations

- V3.2 ordinary AUTO, IR and boxed recovery remain; seven started turns, five pulses, elapsed30s mission bound,2.5s retry cooldown, selected-side<=7cm veto and fresh recovery exit proof retain their original semantics. Mission retries are not a lifetime bound.
- Manual selected once; release/STOP cancels the held intent; safe recovery restores readiness for a new direction without unnecessary mode selection. Genuine command loss, safety/authority revocation and unresolved STOP still inhibit movement. No old held command is replayed.
- Mode freshness fence, command sequencing, Manual override boundaries, sensor validity/freshness and applicable hazard stops remain. No claimed navigation75% reverse cap was introduced.
- No new polling, endpoint, sensor task, Stage4 migration, packet-layout change, firmware flash, motor operation, commit, merge, push or paper change.
- Static RAM/flash are build results. Actual loop deadlines, response quantiles, power and loaded physical behavior remain unmeasured. Large assets can be truncated under a stalled connection after their configured write window; verify loading/reloading on the real home network. Ordinary browser HTTP requests fit the new framing limits; oversized/custom/chunked requests are deliberately not admitted.
- Old PAPER_READY source identities/test totals describe earlier snapshots and were not rewritten here. Public/research documentation synchronization is a later phase.

## Physical handoff and recovery

Use [the owner checklist](STANDALONE_RESPONSE_PHYSICAL_CHECKLIST_20261019.md). Physical acceptance remains pending until owner results identify the uploaded candidate and satisfy comparison against earlier GitHub behavior.

Private pre-edit archive, Development-relative: `Research/Continuity/preservation/PIKU_Standalone_Response_20261019T070309/working-tree.zip`.

Archive SHA-256: `43f439d04c511b2aa4f5dc4f10ea0019547ebd6402f5e90a3704a9d7e2c0ea23`.

The archive was created before edits and every entry reread/hash-verified. It preserves original working bytes, including existing dirty source, but excludes Secrets.h, Git and generated .pio output. The adjacent baseline.json preserves file hashes, original status, refs, index hash and protected-project inventories; previous generated candidate evidence was separately copied privately.

Rollback requires preserving subsequent owner edits first, checking the archive and selected entry hashes, and restoring only the reviewed mission-modified paths. Do not reset/clean the checkout, overlay the whole archive, restore intentionally absent figures or touch private configuration. New test/report files should be preserved as evidence when reverting; do not delete unrelated content. No rollback was performed.
