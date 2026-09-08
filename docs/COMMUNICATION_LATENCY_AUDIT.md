# PIKU V2.0.1 Communication Latency Audit

Date: 2026-08-25

**Physical Validation Status: PASSED**

**Physical Validation Date: 25 August 2026**

## 1. Problem statement and scope

Repeated physical testing reported occasional delay between a web-dashboard action and robot response. This audit traces the active V2.0.1 browser, HTTP, firmware, motor, telemetry, Wi-Fi, sensor, autonomous-navigation, alarm, and TFT paths. The validated hardware behavior, dashboard appearance, routes, sensors, GPIO assignments, 250 ms drive heartbeat, and 750 ms dead-man timeout are treated as protected baseline behavior.

This audit combines static code analysis, build and JavaScript syntax verification, and subsequent qualitative validation on the physical PIKU V2.0.1 platform. No instrumented network latency, handler duration, service-loop gap, or physical motor latency benchmark was performed. Numerical latency statements below are configured bounds or engineering estimates, not hardware measurements.

Historical snapshots and recovered research documentation under `archive/` were inventoried and left unchanged.

## 2. Existing communication architecture

### Browser-to-robot control path

```text
pointer event on dashboard direction/STOP control
  -> startMove(), stopMove(), or explicitStop()
  -> sendCmd()
  -> HTTP GET /cmd?move=...
  -> WebServer::handleClient()
  -> WebDashboard::handleCommand()
  -> WebDashboard::_pendingCommand (single latest-command slot)
  -> serviceLoop()/updateManualMode()/NavigationManager abort handling
  -> DriveCommandHandler
  -> MotorController GPIO writes
  -> L298N and motors
```

Directional commands are accepted only in Manual mode. STOP is accepted in every mode. The Manual dashboard refreshes a held direction every 250 ms, while firmware stops active Manual movement after 750 ms without a received command.

### Robot-to-browser telemetry path

```text
sensors / safety / navigation / motor / alarm / Wi-Fi state
  -> DashboardPublisher and DashboardData cache
  -> HTTP GET /data
  -> WebDashboard::sendJson()
  -> browser response.json()
  -> renderTelemetry()
  -> Overview, Control/radar, Environment, and System views
```

Radar has no separate HTTP endpoint. It consumes `/data` distance samples and draws locally with `requestAnimationFrame()`. Manual sensor-head movement uses `/sensor-head`; autonomous scanning uses the same cached telemetry path.

### HTTP and service-loop model

`WebDashboard` uses the synchronous Arduino `WebServer` on port 80. `serviceLoop()` calls `dashboard.loop()`, sensors, safety, buzzer, telemetry publication, and TFT publication. Manual mode calls the service loop twice in a normal top-level loop pass. Autonomous delays and ultrasonic waits are cooperative: `NavigationManager::responsiveDelay()`, `scanAt()`, and `readAndPublishDistance()` repeatedly invoke the service callback.

## 3. Request-flow analysis

### Baseline telemetry behavior

The baseline JavaScript used `setInterval(loadData, 300)`, or 3.33 timer ticks/s. `loadData()` already had a useful `dataRequestInFlight` guard. Consequently, `/data` requests did not overlap with other `/data` requests and slow requests did not create an unbounded telemetry queue; timer ticks during an active request were dropped.

However, `sendCmd()` performed a `/cmd` fetch followed by `loadData()`. A held direction generated commands every 250 ms, so command completions could add as many as four telemetry attempts per second on top of the 3.33/s timer. Fast responses could therefore create substantially more `/data` traffic even though concurrent `/data` calls were guarded.

The implemented poller is response-driven: one `/data` request completes, the UI updates or reports failure, then a 450 ms timeout schedules the next request. The maximum steady polling rate is 2.22/s and the real rate is `1 / (response time + 0.45 s)`. Manual refreshes and state-changing controls may request an immediate update, but they reuse an in-flight telemetry promise and reset the next scheduled poll.

### Baseline drive behavior and ordering hazard

The baseline issued every direction heartbeat as an independent `fetch()`. If a response took longer than 250 ms, another direction fetch could begin. On release, STOP was another independent request and was deliberately delayed until 140 ms after the initial press for short taps. Browser and TCP connection scheduling did not provide an application-level rule preventing an older direction request from reaching the firmware after STOP.

The firmware had one pending command slot, which prevented a firmware queue from growing, but it could not identify whether an arriving HTTP direction was older than an already received STOP. That is a safety-relevant ordering gap.

The implemented path assigns monotonically increasing `seq` values to dashboard `/cmd` requests. Firmware accepts a sequenced direction only when its number is newer than the last accepted command. STOP bypasses an in-flight direction fetch, clears any unsent direction, and is always accepted as a fail-safe. When STOP has a newer sequence, any late older direction is ignored. Non-STOP browser requests are coalesced to one in flight plus the latest unsent direction; no direction queue accumulates. The 140 ms release delay was removed.

### Firmware application priority

The baseline HTTP handler stored a pending STOP, after which the same `serviceLoop()` could perform sensor, safety, buzzer, telemetry, and TFT work before Manual control consumed it. The implemented service loop checks and physically stops the motors immediately after `handleClient()` returns, before those lower-priority tasks. It leaves STOP pending so the existing Manual state update or AUTO stop latch still runs.

## 4. Calculated HTTP request rates

These are maxima derived from source intervals and event callbacks, not packet captures. They assume requests complete quickly enough for every eligible call to start. Slow responses reduce telemetry rate; in the baseline they could allow multiple independent command fetches to remain outstanding.

| Operating case | Baseline | Optimized |
| --- | ---: | ---: |
| Idle `/data` telemetry | up to 3.33/s | up to 2.22/s, response-driven |
| Held direction `/cmd` heartbeat | about 4/s | target about 4/s; at most one non-STOP request in flight and one latest command coalesced |
| Command-triggered `/data` during held movement | up to 4/s additional | 0/s |
| Held movement total | up to 11.33/s | up to 6.22/s |
| AUTO steady state | up to 3.33/s | up to 2.22/s |
| Radar-only extra HTTP load | 0/s | 0/s |
| Manual sensor-head click | 1 `/sensor-head` plus an immediate/reused data update | unchanged |
| Wi-Fi scan while scan is running | up to 1.25 `/wifi/scan` requests/s temporarily | unchanged; scan requests remain serialized |

Rapid direction changes are event-rate dependent and have no meaningful fixed requests/s figure. In the baseline, each press/release could issue a command and each command completion could attempt telemetry. After optimization, unsent directions collapse to the latest value; STOP is never coalesced behind a direction.

The compact `/data` JSON contains 38 fields after adding `commandSequence`. A representative payload constructed with realistic values is approximately 925 bytes before HTTP headers. This is an estimate; actual bytes vary with SSID, status, and navigation strings. The firmware preallocates 1,152 bytes for JSON construction to avoid a likely growth reallocation for normal and maximum-length status content.

## 5. Prioritized findings

### Critical

#### Finding: asynchronous commands had no stale-request rejection

- **Evidence from code:** Baseline `WebDashboardAssets.cpp::sendCmd()` launched independent fetches; `startMove()` used a 250 ms interval; `stopMove()` launched STOP separately. `WebDashboard::applyCommand()` accepted any valid arriving direction in Manual mode and recorded no request order.
- **Why it matters:** A delayed earlier FORWARD/LEFT/RIGHT could arrive after STOP and become the newest firmware command. The dead-man would eventually stop it, but that does not make post-STOP movement acceptable.
- **Estimated effect:** Intermittent and network-dependent; potentially a direction action after operator release. No frequency or latency was measured.
- **Recommended/implemented change:** Sequence dashboard commands, reject stale directions in firmware, coalesce non-STOP requests, and allow STOP to bypass them.
- **Risk of change:** Low to moderate. Legacy `/cmd` and `/control/*` calls remain accepted, and STOP remains fail-safe. Multiple simultaneous dashboard operators still have no ownership arbitration; one active operator remains the intended model.

#### Finding: release STOP included an intentional delay

- **Evidence from code:** Baseline `startMove()` set `minStopAt = Date.now() + 140`; `stopMove()` waited until that time before sending STOP.
- **Why it matters:** A short press added up to 140 ms of known release latency before network and firmware latency.
- **Estimated effect:** Zero to 140 ms depending on press duration; this is a configured delay, not a measured physical stop time.
- **Recommended/implemented change:** Send release STOP immediately. Sequence ordering replaces the earlier timing workaround.
- **Risk of change:** Low. Very short taps may now produce a shorter physical movement pulse, which is consistent with immediate-release semantics and safer STOP behavior.

### Important

#### Finding: drive commands generated redundant telemetry traffic

- **Evidence from code:** Baseline `sendCmd()` called `loadData()` after every `/cmd`; independent `setInterval(loadData, 300)` polling remained active.
- **Why it matters:** Held movement could reach a theoretical 11.33 HTTP requests/s and compete with command handling on a synchronous server.
- **Estimated effect:** Up to four unnecessary `/data` requests/s during a held command, subject to the existing in-flight telemetry guard.
- **Recommended/implemented change:** Remove command-triggered telemetry from drive heartbeats and poll at a response-driven 450 ms interval.
- **Risk of change:** Low. Motor state presentation can lag by at most one normal telemetry cycle; control application is independent of telemetry.

#### Finding: Wi-Fi housekeeping ran at fast-loop frequency

- **Evidence from code:** Baseline `WebDashboard::loop()` called `updateWiFi()` every service pass. All branches called `refreshWiFiTelemetry()`, which repeatedly called `WiFi.status()`, `softAPIP().toString()`, `localIP().toString()`, and, when connected, `WiFi.RSSI()`. `DashboardPublisher::setData()` and `/data` also refreshed it.
- **Why it matters:** Slowly changing Wi-Fi information caused repeated stack calls and `String` assignments at a rate unrelated to its display value.
- **Estimated effect:** Potentially thousands of redundant refresh attempts/s in tight loops; actual loop frequency and cost were not measured.
- **Recommended/implemented change:** Run Wi-Fi state processing and telemetry refresh at 1,000 ms intervals, force immediate refresh on configuration/scan transitions, and reapply cached Wi-Fi values when sensor data replaces the dashboard model.
- **Risk of change:** Low. System-page Wi-Fi information may be up to about one second old; connection retries remain at the existing 15 seconds.

#### Finding: pending STOP was applied after other service work

- **Evidence from code:** Baseline `serviceLoop()` called `dashboard.loop()` and then sensor/safety/buzzer/publish work; Manual command consumption occurred later in `updateManualMode()`.
- **Why it matters:** Synchronous DHT or TFT work could add a bounded but variable delay after receipt.
- **Estimated effect:** Unknown without instrumentation; depends on whether those periodic operations are due.
- **Recommended/implemented change:** Stop motor GPIOs immediately after HTTP service when pending command is STOP, while preserving later state/latch handling.
- **Risk of change:** Low. It only advances the already-requested fail-safe action.

#### Finding: Wi-Fi modem sleep favored power over command latency

- **Evidence from code:** Baseline initialization did not explicitly configure Wi-Fi sleep.
- **Why it matters:** Modem power saving can add packet latency/jitter, especially for interactive STA traffic.
- **Estimated effect:** Hardware/network dependent and not measured.
- **Recommended/implemented change:** Call `WiFi.setSleep(false)` after selecting AP/AP+STA mode. Motors dominate the robot power budget and responsiveness is the stated priority.
- **Risk of change:** Higher ESP32 radio power use and some additional heat. SoftAP and STA behavior require physical verification.

### Minor

#### Finding: fast-loop dashboard getters copied `String` values

- **Evidence from code:** `getControlMode()`, `getTurnMode()`, and `getPendingCommand()` returned `String` by value and are called repeatedly from service, Manual, and AUTO paths.
- **Why it matters:** Avoidable reference-count/copy/allocation activity can add heap churn over long sessions.
- **Estimated effect:** Small compared with HTTP and Wi-Fi work; not measured.
- **Recommended/implemented change:** Return constant references without changing call-site behavior.
- **Risk of change:** Low; callers do not retain mutable references.

#### Finding: telemetry JSON uses dynamic `String` concatenation

- **Evidence from code:** `WebDashboard::sendJson()` constructs JSON with `String` concatenation on every `/data` request.
- **Why it matters:** Temporary strings can fragment heap in long runtimes.
- **Estimated effect:** Minor at the optimized 2.22/s maximum; no heap telemetry exists.
- **Recommended change:** Keep the readable implementation for V2.0.1, retain preallocation, and increase reserve from 960 to 1,152 bytes for the sequence field and long strings. Add heap instrumentation only if physical soak testing shows degradation.
- **Risk of change:** Broadly rewriting JSON generation would add regression risk without current evidence, so it was not done.

#### Finding: synchronous bounded peripheral operations remain

- **Evidence from code:** `MotorController::forward()` preserves a 5 ms coast settle plus 80 ms startup compensation. DHT reads are synchronous every 2,200 ms. TFT SPI updates are synchronous and throttled to 500 ms. Serial telemetry is throttled to 1,000 ms.
- **Why it matters:** These can create short service gaps.
- **Estimated effect:** Forward transition has a configured 85 ms firmware gap. DHT/TFT/Serial durations are not measured in this repository.
- **Recommended change:** Preserve validated motor timing and existing peripherals. Instrument maximum service-loop gap on hardware before changing them.
- **Risk of change:** Motor or display modifications could change validated physical behavior, so none were made.

### Acceptable

#### Finding: telemetry already prevented same-endpoint overlap

- **Evidence from code:** Baseline `loadData()` returned early while `dataRequestInFlight` was true.
- **Why it matters:** Slow `/data` responses did not create an unbounded telemetry request queue.
- **Recommended change:** Preserve the guard and make scheduling explicitly response-driven.

#### Finding: ultrasonic sampling is non-blocking

- **Evidence from code:** `SensorManager` uses an interrupt and state machine; its 30 ms echo window advances through `update()`. Navigation's 40 ms acquisition wait calls the service callback every iteration.
- **Why it matters:** HC-SR04 no-echo cases do not freeze HTTP for the full timeout.
- **Recommended change:** None.

#### Finding: long AUTO and servo waits service communications

- **Evidence from code:** `NavigationManager::responsiveDelay()` and global `responsiveDelay()` call `serviceLoop()` and delay only 1 ms per iteration. Manual 700 ms head settling is a state machine.
- **Why it matters:** AUTO reverse/scan/turn/settle operations remain interruptible by STOP, mode changes, and critical safety.
- **Recommended change:** None.

#### Finding: TFT and sensor update rates are already throttled

- **Evidence from code:** TFT is 500 ms, DHT is 2,200 ms, fast sensors are 50 ms, ultrasonic cadence is 100 ms, and serial logging is 1,000 ms.
- **Why it matters:** Peripheral work is not deliberately executed on every loop.
- **Recommended change:** Keep all intervals and sensors unchanged.

#### Finding: dashboard assets and routes are efficient enough

- **Evidence from code:** HTML, CSS, and JavaScript are stored in PROGMEM and served by fixed routes. Radar drawing is local. Wi-Fi scans are asynchronous and browser scan retries are in-flight guarded.
- **Why it matters:** No dashboard regeneration occurs per request and no new transport/library is required.
- **Recommended change:** None.

## 6. Implemented changes and rationale

1. Replaced 300 ms interval polling with a 450 ms post-response timeout.
2. Removed `/data` refreshes from drive command completion.
3. Coalesced direction/heartbeat fetches to one active and one latest unsent command.
4. Added dashboard `seq` query values, firmware stale-direction rejection, and `commandSequence` telemetry synchronization.
5. Made STOP bypass direction coalescing, always remain accepted, and removed the 140 ms release delay.
6. Applied pending STOP immediately after HTTP handling, before lower-priority service work.
7. Serviced `_server.handleClient()` before periodic Wi-Fi state work.
8. Cached Wi-Fi display information and throttled status/IP/RSSI refresh to 1,000 ms, with forced refresh for configuration transitions.
9. Disabled ESP32 Wi-Fi modem sleep to favor control latency.
10. Returned dashboard state strings by constant reference and enlarged JSON reserve to 1,152 bytes.

No page, visual style, route, sensor, GPIO, motor timing, AUTO behavior, alarm behavior, TFT behavior, AP/STA capability, servo behavior, or safety threshold was removed or redesigned.

## 7. Safety analysis

The 250 ms held-drive heartbeat and 750 ms Manual timeout are unchanged. Firmware still requires continued command receipt and stops after three missed nominal heartbeat periods. The timeout was not increased to mask latency.

For the safety-critical order:

```text
FORWARD seq N starts
FORWARD seq N+1 starts or is coalesced
release sends STOP seq N+2 immediately
```

If a direction reaches firmware first, STOP then stops it. If STOP reaches firmware first, it advances the accepted sequence and a later lower-sequence direction is ignored. STOP is also accepted when its sequence is stale, covering a newly opened dashboard before its first telemetry synchronization. Firmware applies pending STOP before sensor/TFT/telemetry work and then retains the existing Manual state transition or AUTO operator-stop latch.

Legacy `/cmd` without `seq` and `/control/*` routes remain available. They are processed sequentially by `WebServer` but do not carry browser ordering metadata. The intended safety-tested operating model remains one supervised dashboard operator on a trusted network.

Critical hazards, near-obstacle logic, Manual forward guards, AUTO aborts, alarm priority, and motor timeout paths are unchanged.

## 8. Blocking and service analysis

| Operation | Code behavior | HTTP servicing impact |
| --- | --- | --- |
| Normal loop / Manual loop | `serviceLoop()` at least once; Manual normally twice | Frequent service |
| AUTO delays/reverse/turn/settle | Cooperative `responsiveDelay()` with service callback | HTTP serviced approximately once per loop iteration, apart from peripheral work |
| Servo scan settle | 700 ms cooperative wait | HTTP remains serviced |
| Ultrasonic echo window | 30 ms non-blocking state machine | No full-window block |
| Navigation distance wait | Up to configured 40 ms, services loop | HTTP remains serviced |
| Forward motor transition | Preserved 5 ms + 80 ms synchronous startup timing | Configured service gap up to about 85 ms |
| Other motor direction transition | Preserved 5 ms settle | Short service gap |
| DHT22 read | Synchronous every 2,200 ms | Duration unknown; measure on hardware |
| TFT update | Synchronous SPI, throttled to 500 ms and changed fields | Duration unknown; full warning frame may be largest display cost |
| Wi-Fi scan | ESP32 asynchronous scan, browser polls every 800 ms | Radio activity may still affect latency temporarily |
| Preferences writes | Only explicit Wi-Fi provision/forget actions | Not part of normal drive operation |

## 9. Build and static verification

### Baseline build before edits

- PlatformIO environment: `esp32doit-devkit-v1`
- Result: PASS
- RAM: 50,056 / 327,680 bytes (15.3%)
- Flash: 954,133 / 1,310,720 bytes (72.8%)

### Optimized build

- Result: PASS, no compiler errors
- RAM: 50,160 / 327,680 bytes (15.3%), increase 104 bytes
- Flash: 956,485 / 1,310,720 bytes (73.0%), increase 2,352 bytes
- Embedded dashboard JavaScript: PASS using a JavaScript parser (`vm.Script`), 29,862 characters
- Mocked command-order simulation: PASS for direction coalescing, immediate higher-sequence STOP, and deferral of a new direction until active direction/STOP requests complete
- `git diff --check`: PASS

The clean target itself reported SUCCESS, followed by PlatformIO's existing post-exit telemetry-cache permission traceback outside the repository. It did not affect cleaning or compilation.

Static route review confirms the existing routes remain registered:

```text
/
/dashboard.css
/dashboard.js
/data
/mode
/turn-mode
/cmd
/sensor-head
/alarm/enable
/alarm/mute
/alarm/test
/wifi/scan
/wifi/connect
/wifi/forget
/mode/auto
/mode/manual
/control/forward
/control/backward
/control/left
/control/right
/control/stop
```

Static path review also confirms FORWARD, BACKWARD, LEFT, RIGHT, STOP, AUTO, MANUAL, pivot/spin, servo/head control, radar telemetry, alarms, SoftAP, STA, sensors, TFT, and autonomous navigation remain represented. Static review and compilation cannot prove physical timing or radio behavior.

## 10. Physical validation and repeatable test procedure

### Completed physical validation — 25 August 2026

The optimized firmware compiled successfully and was uploaded successfully to the physical PIKU V2.0.1 platform. Dashboard control and movement commands operated correctly, including STOP/release behavior. The dashboard remained functional, the previously problematic communication latency was not observed during this validation, and no obvious functional regression was observed during the tests performed.

This result is a qualitative functional validation. It is not an instrumented latency benchmark and does not establish numerical latency values or formal safety certification.

### Repeatable procedure

Perform motion tests with clear floor space, a manual power cutoff, and the robot under direct supervision. Use browser developer tools Network view where available.

### Test 1 — Idle telemetry soak

1. Open the dashboard for 10 minutes in SoftAP, then repeat on home Wi-Fi.
2. Confirm smooth updates, no progressive slowdown, no ESP32 reset, and no growing pending-request count.
3. Confirm `/data` begins only after the prior response and the approximately 450 ms post-response wait.

### Test 2 — Continuous movement

1. Select Manual and hold FORWARD for 30–60 seconds.
2. Confirm immediate start, uninterrupted motion, approximately 250 ms command heartbeat, and responsive telemetry.
3. Confirm no stack of pending direction requests in browser tools.

### Test 3 — STOP latency

1. Repeat FORWARD then release at least 20 times, including short taps and long holds.
2. Confirm STOP is immediate and reliable with no post-STOP forward pulse.
3. Repeat using the explicit STOP button and while TFT/environment values are actively updating.

### Test 4 — Rapid transition/order test

1. Rapidly issue FORWARD, LEFT, RIGHT, BACKWARD, STOP for at least 20 sequences.
2. Confirm only the latest intended direction is observed and no stale action occurs after STOP.
3. Inspect `/cmd` query sequences and confirm they increase.

### Test 5 — Network conditions

1. Repeat Tests 2–4 on PIKU SoftAP and configured home Wi-Fi.
2. Repeat at increasing range/poorer RSSI and during unrelated network traffic.
3. Compare with and without a user-triggered Wi-Fi scan where scanning is allowed.

### Test 6 — Communication loss/dead-man

1. Hold a direction, then close the dashboard, disable client Wi-Fi, or move out of range.
2. Confirm motors stop within the existing 750 ms firmware timeout from the last received heartbeat.
3. Confirm reconnecting does not restart motion without a new press.

### Test 7 — Full-system operation

Run sensors, alarms, radar, TFT, telemetry, Manual drive, and AUTO navigation together. Trigger safe test conditions for head scan, obstacle handling, alarm test, mode change, and AUTO STOP. Confirm responsiveness and every existing safety response.

### Additional recommended instrumentation run

For diagnosis if physical latency remains, add compile-time-disabled counters/timestamps for command receipt, command application, STOP, dead-man stop, `/data` count, maximum service-loop gap, and handler duration. Log summaries once per second rather than every request. Compare maximum loop gaps with DHT reads, TFT warning redraws, motor startup compensation, scans, and weak-RSSI intervals.

## 11. Remaining limitations

- No physical latency distribution, packet capture, heap trend, RSSI comparison, or maximum-loop-gap measurement exists yet.
- `WebServer` is synchronous and handles one request at a time; the optimized normal load is modest, but a slow client or radio issue can still delay service.
- The validated 85 ms forward startup sequence remains a deliberate synchronous motor-control gap.
- DHT, TFT, and serial durations need hardware instrumentation if latency remains visible.
- User-triggered Wi-Fi scanning can temporarily consume radio resources.
- Disabling Wi-Fi sleep increases ESP32 energy use and should be checked for battery and thermal impact.
- Command sequencing resolves reordering for the active dashboard flow, but this prototype has no authenticated controller ownership or multi-operator arbitration.
- HTTP commands remain unauthenticated and suitable only for a trusted, supervised network.

## 12. HTTP versus WebSocket recommendation

PIKU V2.0.1 should remain HTTP-based. After removing redundant telemetry, preventing direction backlog, sequencing STOP against stale directions, and throttling Wi-Fi work, the expected normal load is small for an ESP32: at most about 2.22 telemetry requests/s idle and about 6.22 total requests/s during a healthy held command. The current request/response model is transparent, debuggable, compatible with all existing routes, and minimizes regression risk for the validated prototype.

A WebSocket migration would add connection lifecycle, reconnect, ownership, message ordering, and safety-state complexity without repository evidence that optimized HTTP is inadequate. It should be reconsidered for a future distributed multi-node CPS version if requirements include multiple publishers/subscribers, higher-rate streaming, authenticated sessions, coordinated timestamps, or bidirectional event delivery. Such a future design should evaluate WebSockets, MQTT, or another protocol against measured latency, loss, security, and multi-node scheduling requirements rather than choosing solely on theoretical overhead.

## 13. Final assessment

The most likely repository-supported contributors were redundant request generation, lack of command ordering semantics, a deliberate short-release STOP delay, fast-loop Wi-Fi telemetry work, and lower-priority service work occurring before pending STOP application. The audit did not find a long blocking servo, ultrasonic, or AUTO wait that completely starves HTTP; those paths are predominantly cooperative. The optimized HTTP architecture is appropriate for PIKU V2.0.1 and passed the reported physical functional validation above. The repeatable procedure remains useful for future regression testing and instrumented characterization.
