# PIKU 2.1 Rev A Physical Testing Record

## Record identification

- Test date: 2026-09-09
- Product: PIKU 2.1 Rev A
- Firmware version: `2.1.0-revA-stage2`
- Firmware stage: `STAGE-2`
- Repository branch: `master`
- Firmware commit: `4fa34e144bc63a45a2e241b245cf5ff90745586b`
- Commit subject: `Harden PIKU 2.1 sensors dashboard control and autonomous navigation`
- Repository condition: the working tree was clean before this testing record was created; the commit above is the firmware-under-test traceability anchor.

This is a functional prototype testing record. It is not a certification report and does not establish calibrated sensor performance, formal radio range, deterministic control latency, fire-detection suitability, or production readiness.

## Evidence labels

- **PHYSICALLY OBSERVED** — directly reported from operation of the robot.
- **APPROXIMATE / VISUAL ESTIMATE** — observed without calibrated measurement instrumentation.
- **NOT MEASURED** — no quantitative instrument record was obtained.
- **NOT VALIDATED** — the item was outside this test scope or lacks sufficient evidence.
- **HYPOTHESIS / REQUIRES INVESTIGATION** — a possible explanation, not a confirmed cause.

## Test scope

The session exercised the current stable PIKU 2.1 Rev A firmware as an integrated prototype, including:

- runtime and battery-powered operation;
- MANUAL and AUTO modes;
- dashboard control responsiveness and telemetry;
- autonomous obstacle avoidance with gyro-assisted relative turning;
- standalone PIKU SoftAP operation;
- the GPIO6 digital flame-sensor alarm path; and
- qualitative MQ-2 response and alarm clearing.

STA/Home Wi-Fi, formal RF coverage, instrumented turn angle, instrumented control latency, calibrated gas concentration, and certified fire-detection performance were outside this session.

## Test environment

- Indoor prototype operation was reported under normal room lighting and in a dark room.
- Wireless testing used only the PIKU standalone SoftAP.
- Flame tests used a real flame and approximate source-to-sensor distances.
- An orange-colored obstacle was present during the reproducible false-positive investigation.
- MQ-2 response was exercised with lighter gas released very close to the sensor.
- Exact floor properties, ambient temperature, battery chemistry/state of charge, electrical load traces, RF interference, and calibrated reference instruments were not recorded.

Distance, duration, turn-angle, and responsiveness statements in this record retain the evidence qualification supplied by the observer.

## Physical results summary

| Area | Result | Evidence classification |
|---|---|---|
| Integrated runtime | Robot operated successfully for approximately 30 minutes | **PHYSICALLY OBSERVED**; duration approximate |
| First approximately 25 minutes | Normal operation | **PHYSICALLY OBSERVED** |
| MANUAL mode | Worked successfully | **PHYSICALLY OBSERVED** |
| AUTO mode | Worked successfully | **PHYSICALLY OBSERVED** |
| Dashboard control | Very fast and accurate in practical use | **PHYSICALLY OBSERVED**, qualitative; latency **NOT MEASURED** |
| Telemetry | Remained stable during the session | **PHYSICALLY OBSERVED** |
| Gyro-assisted avoidance turn | Appeared close to the intended approximately 20° relative turn | **APPROXIMATE / VISUAL ESTIMATE**; exact angle **NOT MEASURED** |
| Standalone SoftAP | Stable practical control observed to approximately 30 ft (roughly 9–10 m) | **PHYSICALLY OBSERVED** in the tested environment; formal RF range **NOT VALIDATED** |
| Flame response | Detected real flame and invoked stop/alarm behavior | **PHYSICALLY OBSERVED**; distances approximate and uncalibrated |
| Flame false positive | Reproducible response to the tested orange object without fire | **PHYSICALLY OBSERVED**; cause not confirmed |
| MQ-2 response | Raw value increased near released lighter gas and crossed the configured relative alarm threshold | **PHYSICALLY OBSERVED**; qualitative and uncalibrated |

## Power observations

- **PHYSICALLY OBSERVED:** PIKU operated successfully for approximately 30 minutes.
- **PHYSICALLY OBSERVED:** operation was normal for roughly the first 25 minutes.
- **PHYSICALLY OBSERVED:** after about 25 minutes, buck-converter input voltage dropped as battery energy decreased.
- **PHYSICALLY OBSERVED:** the motors then appeared to struggle or push themselves because insufficient power was available under load.
- **NOT MEASURED:** no voltage-versus-time trace, current trace, battery internal-resistance measurement, or minimum stable operating-voltage boundary was recorded.

The late-session motor behavior is recorded as an observed battery/power limitation. The available evidence does not establish it as a firmware failure.

## Manual and AUTO behavior

- **PHYSICALLY OBSERVED:** MANUAL mode operated successfully.
- **PHYSICALLY OBSERVED:** AUTO mode operated successfully.
- **PHYSICALLY OBSERVED:** dashboard commands felt very fast and accurate during practical control.
- **NOT MEASURED:** no instrumented phone-to-handler, handler-to-motor, or release-to-stop latency measurement was performed.
- **PHYSICALLY OBSERVED:** autonomous avoidance behavior improved relative to the earlier short-turn behavior.
- **APPROXIMATE / VISUAL ESTIMATE:** the gyro-assisted avoidance turn appeared close to the intended approximately 20° relative rotation.
- **NOT MEASURED:** exact left/right physical rotation, repeatability, bias, and surface-dependent error were not measured.

The turn observation must not be interpreted as validation of an exact 20° result, compass heading, absolute yaw, localization, or odometry.

## Wireless and telemetry results

- **PHYSICALLY OBSERVED:** the PIKU standalone SoftAP remained stable during the test.
- **PHYSICALLY OBSERVED:** practical dashboard control remained usable to approximately 30 ft, or roughly 9–10 m, in the tested environment.
- **PHYSICALLY OBSERVED:** telemetry remained stable during testing.
- **NOT MEASURED:** packet loss, jitter, latency distribution, RSSI-versus-distance, throughput, interference, and directional coverage were not instrumented.
- **NOT VALIDATED:** the observation is not a formal RF range or coverage result and should not be generalized to other buildings, antenna orientations, interference conditions, or power states.
- **NOT VALIDATED:** STA/Home Wi-Fi was not exercised in this session.

For context only, a 10 m radius corresponds mathematically to approximately 314 m² using πr². This is a geometric approximation, not measured RF coverage.

## Flame sensor results

### Tested hardware state

- Sensor supply: 3.3 V
- Digital output: DO to GPIO6
- Analog output: AO disconnected
- Active polarity: physically corrected and working with the current firmware; no flame produces GPIO6 LOW, while detected stimulus produces GPIO6 HIGH.

### Real-flame observations

- **PHYSICALLY OBSERVED / APPROXIMATE:** under normal room lighting, a real flame was detected from approximately 15 cm. PIKU stopped and raised the flame alarm.
- **PHYSICALLY OBSERVED / APPROXIMATE:** in a dark room, the sensor responded to a real flame from approximately 10 ft away.
- **NOT VALIDATED:** these distances are not calibrated sensitivity, guaranteed detection range, coverage, or certified fire-detection performance.

Lighting, source characteristics, angle, module potentiometer position, reflections, and sensor-to-source geometry can materially affect this comparator-based result.

### Reproducible false-positive observation

- **PHYSICALLY OBSERVED:** during AUTO operation, an orange-colored obstacle caused a flame alarm even though no fire was present.
- **PHYSICALLY OBSERVED:** removing the orange object cleared the alarm.
- **PHYSICALLY OBSERVED:** bringing the same object close to the flame sensor reproduced the alarm.

This establishes a reproducible false positive for that object and tested setup. It does **not** prove that orange color itself causes flame detection.

Root cause: **HYPOTHESIS / REQUIRES INVESTIGATION — NOT YET CONFIRMED**.

Plausible contributors include:

- reflected infrared or other optical energy;
- the object's surface reflectivity or finish;
- ambient-light interaction;
- flame-module comparator sensitivity or potentiometer setting; and
- possible optical interaction or crosstalk from nearby IR obstacle-sensor emitters.

No individual mechanism is confirmed by the present observation.

## MQ-2 results

- **PHYSICALLY OBSERVED:** the MQ-2 responded when lighter gas was released very close to the sensor.
- **PHYSICALLY OBSERVED:** the raw ADC value increased during exposure.
- **PHYSICALLY OBSERVED:** the gas alarm activated after the configured relative threshold was crossed.
- **PHYSICALLY OBSERVED:** after the gas source was removed, raw values gradually decreased.
- **APPROXIMATE / VISUAL ESTIMATE:** the alarm remained active for approximately four seconds before clearing.
- **NOT MEASURED:** the clearing interval was not recorded with timing instrumentation.
- **NOT VALIDATED:** ppm concentration, room-wide gas concentration, exact detection range, selectivity, and calibrated sensitivity were not established.

These results demonstrate qualitative relative prototype behavior only. Lighter gas must be handled with suitable ventilation and ignition controls during any repeat testing.

## Known issues

1. Battery endurance and voltage-under-load limitation becomes apparent late in the observed runtime.
2. The flame module can produce reproducible false positives and remains environmentally sensitive.
3. Exact left/right gyro-assisted avoidance rotation and repeatability remain unmeasured.
4. MQ-2 output remains qualitative and uncalibrated.

## Limitations

- The session was functional prototype testing rather than a controlled validation campaign.
- Approximate duration and distance observations were not backed by synchronized instrumentation.
- The control path was qualitatively responsive, but no latency measurement was performed.
- Only one reported indoor SoftAP environment was exercised.
- Flame behavior depends on a thresholded digital comparator and environmental optical conditions.
- Flame results do not qualify PIKU as certified fire-detection or life-safety equipment.
- MQ-2 readings are relative ADC behavior and cannot be translated to ppm from this evidence.
- Battery voltage and current under motor load were not logged.
- The exact physical avoidance angle was not measured.

## Next validation priorities

1. Perform controlled flame false-positive trials using orange, red, yellow, white, black, and reflective objects while recording object material, finish, distance, orientation, ambient light, and comparator adjustment.
2. Repeat those trials with obstacle-sensor IR emitters active and then temporarily disabled or optically covered to investigate possible IR interaction. Preserve obstacle safety by using a secured, stationary test setup whenever emitters are disabled or covered.
3. Repeat flame and false-positive tests under multiple controlled lighting conditions and object distances.
4. Measure actual left and right avoidance rotation over repeated trials using an overhead angle reference or equivalent instrumented method. Report distributions rather than a single visual estimate.
5. Log battery voltage at the battery and buck input under motor load across useful runtime, together with time, motor command, and observed control stability.
6. Log MQ-2 raw values versus time during clean air, controlled gas exposure, source removal, and recovery. Do not infer ppm without an appropriate calibration method and reference gas.
7. Test STA/Home Wi-Fi separately, including association recovery, telemetry stability, control response, and coexistence with the standalone SoftAP.

## Overall assessment

The tested PIKU 2.1 Rev A firmware was physically observed to provide good practical control responsiveness, stable standalone SoftAP operation, stable telemetry, successful MANUAL and AUTO operation, and visibly improved autonomous avoidance behavior during the approximately 30-minute prototype session.

The firmware is suitable for continued supervised research and tuning on this prototype. The main evidence gaps are battery endurance under load, flame false-positive susceptibility, instrumented avoidance-angle accuracy, calibrated gas behavior, and separate STA/Home Wi-Fi validation. These limitations must remain explicit in future reports and must not be replaced by calibrated, coverage, latency, or safety-certification claims without corresponding measurements.

---

## Post-Test AUTO Navigation Hardening — Build Verification

### Traceability and evidence boundary

- Build-verification date: 2026-09-09
- Working branch: `phase-c-auto-navigation-hardening`
- Base/HEAD firmware anchor: `4fa34e144bc63a45a2e241b245cf5ff90745586b` (`Harden PIKU 2.1 sensors dashboard control and autonomous navigation`)
- Repository state: the Phase C firmware and this record contain uncommitted working-tree changes on the branch above. The HEAD identifier is the stable pre-Phase-C anchor, not a commit containing these changes.
- Phase C status: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**
- Deployment status: firmware was **NOT UPLOADED** and flash was **NOT ERASED** during this repository task.

The historical physical observations above remain evidence for the firmware represented by the stable anchor. They do not validate the new Phase C behavior described below. In particular, the previous approximately 20° visual turn observation must not be presented as physical evidence for the new 35° target.

### Implemented AUTO hardening

- AUTO avoidance relative-turn target changed from 20° to a configurable 35°. The maneuver continues to integrate short-term MPU6050 gyro-Z rate relative to its stopped pre-turn sample; this is not compass heading, absolute yaw, localization, or odometry.
- Normal AUTO left/right choices now command the existing one-wheel `leftTurn()` / `rightTurn()` pivot behavior. Manual PIVOT and SPIN command routes and motor-driver semantics were not changed.
- SG90 scan positions widened to physical LEFT 170°, FRONT 90°, and RIGHT 10°, corresponding to the established mirrored-dashboard logical presentation of left −80°, front 0°, and right +80°. Servo extremes 0° and 180° are not commanded.
- Scanning is now a non-blocking state sequence. Each direction receives a configurable 300 ms servo settle, three HC-SR04 readings with a 45 ms pre-sample gap, and a 300 ms front recenter settle after the full set. The 40 ms configured ultrasonic request timeout retains an additional 20 ms navigation watchdog margin. This reduces software dwell; it does not claim increased SG90 mechanical speed.
- Direction aggregation requires at least two coherent readings. Three finite readings use the median; two finite readings use the conservative lower value. One transient invalid result cannot outweigh two coherent readings, while repeated fault or non-coherent evidence stops navigation.
- LEFT/RIGHT avoidance chooses the greater trustworthy clearance. `CLEAR_NO_ECHO` remains distinct from `SENSOR_FAULT`, is treated as a valid out-of-range clear path, and is preferred over a finite distance. Equal/near-equal sides use a deterministic alternating tie break. Unsafe or invalid side sets stop conservatively.
- AUTO flame/gas recovery now follows: immediate STOP → continuous hazard monitoring → 2,000 ms uninterrupted clear wait → fresh LEFT/FRONT/RIGHT scan → safest valid path → AUTO resume. Hazard reappearance resets the timer and restores `HAZARD_ACTIVE`. Manual STOP, leaving AUTO, nonrecoverable critical conditions, and ultrasonic fault/stale evidence remain authoritative.
- AUTO-only stuck suspicion requires three simultaneous evidence classes: at least 3,000 ms accumulated forward command, at least 1,500 ms of fresh healthy MPU `STILL` classification during forward command, and six finite front-distance readings with no more than 1.5 cm range. This is conservative suspicion, not stall proof or odometry.
- Stuck recovery is bounded to three attempts and follows STOP → 50 ms reverse preparation → 250 ms reverse → 150 ms settle → fresh three-direction scan → clearest valid LEFT/RIGHT → approximately 35° pivot → front revalidation. Five seconds of subsequent forward operation without repeated suspicion clears the retry history. Exhaustion exposes `STUCK_RECOVERY_FAILED` and holds STOP.
- Scan, turn, reverse, hazard wait, and stuck recovery are timestamp/state driven. No long waiting loop or delay was added. The Phase B high-priority HTTP/STOP/mode path, 250 ms telemetry cadence, Manual deadman, and background OLED/DHT/IMU/nRF deferral remain in place.
- Compact Control-tab and JSON diagnostics now expose AUTO state, 35° target/current relative angle, selected path, three scan states/distances including `CLEAR_NO_ECHO`, hazard recovery/timer, stuck status/attempts/reason, and turn completion reason.

### Repository validation result

- `pio run -t clean`: **PASS**
- Clean `pio run` for `esp32-s3-n16r8`: **PASS**, no compiler warnings reported
- RAM: 53,356 bytes of 327,680 bytes (16.3%)
- Flash: 1,007,877 bytes of 6,553,600 bytes (15.4%)
- Embedded JavaScript syntax validation with the installed Deno runtime: **PASS**
- Dashboard DOM/handler validation: **PASS** (128 IDs, 142 referenced IDs, 13 inline handlers; all new AUTO diagnostic IDs present and bound)
- `git diff --check`: **PASS**; Git emitted line-ending normalization notices only, not whitespace errors
- Pin-conflict check: **PASS** (24 allocated nonnegative pins, all unique)
- Static scope review: **PASS**; Stage 4, PIKU 2.0.1, and CPS Node V1 are untouched
- Regression comparison: **PASS**; Manual command/motor routes and the GPIO6 active-HIGH digital flame implementation are unchanged from the anchor

### Required physical validation

Physical validation remains pending for the 35° left/right pivot repeatability, 170°/90°/10° servo mechanical clearance, faster-scan reading stability, finite-distance and `CLEAR_NO_ECHO` route selection, hazard-clear timer reset/resume behavior, stuck-suspicion false-positive rate and bounded recovery, and STOP/dashboard responsiveness during every new AUTO state. No Phase C runtime, angle, distance, timing, or recovery-success claim is made by this build verification.
---

## Phase C Physical Follow-Up and Phase C.1 IR-Zone Hardening - 2026-09-09

### Traceability and evidence boundary

- Working branch: `phase-c1-ir-obstacle-pivot-hardening`
- Base/HEAD firmware anchor: `4fa34e144bc63a45a2e241b245cf5ff90745586b`
- Repository state: Phase A, Phase B, Phase C, and Phase C.1 remain uncommitted working-tree changes based on the anchor above.
- Phase C.1 status: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**
- Deployment status for Phase C.1: **NOT UPLOADED**; flash was **NOT ERASED**.

The observations below were reported from physical operation of the preceding Phase C firmware. They do not physically validate the new Phase C.1 behavior.

### Newly reported Phase C physical observations

- **PHYSICALLY OBSERVED:** Phase C firmware was uploaded to and operated on PIKU before this Phase C.1 repository task.
- **PHYSICALLY OBSERVED:** AUTO navigation generally worked.
- **PHYSICALLY OBSERVED:** Manual control and dashboard responsiveness remained good.
- **NOT MEASURED:** no instrumented command-latency distribution or release-to-stop timing was recorded; the responsiveness result remains qualitative.
- **PHYSICALLY OBSERVED:** the Phase C AUTO 35-degree turn did not reliably reproduce the desired strong/pure Manual-mode pivot behavior.
- **PHYSICALLY OBSERVED:** Manual-mode PIVOT LEFT and PIVOT RIGHT produced the desired physical pivot behavior.

### Physically confirmed environmental-hazard recovery

- **PHYSICALLY OBSERVED:** an active environmental hazard stopped PIKU and produced the alarm state.
- **PHYSICALLY OBSERVED / APPROXIMATE:** after the hazard remained continuously clear for approximately two seconds, PIKU performed a scan and continued AUTO automatically.
- **PHYSICALLY OBSERVED:** the operator did not need to select AUTO again for that recovery.

This confirms the Phase C behavior in the tested setup. It is not an instrumented timing measurement and does not change the prototype/non-certified safety boundary.

### Flame-sensor optical mitigation follow-up

- Hardware remained 3.3 V supply, DO to GPIO6, AO disconnected, active HIGH.
- **PHYSICALLY OBSERVED, EARLIER:** direct/strong sunlight and nearby orange/red/yellow objects could trigger false flame alarms; potentiometer adjustment did not sufficiently remove those responses.
- **PHYSICALLY IMPLEMENTED:** a small black-tape optical shroud was installed around the flame sensor.
- **PHYSICALLY OBSERVED:** behavior improved after the shroud was fitted.
- **PHYSICALLY OBSERVED, LATEST TEST:** nearby orange/red/yellow objects did not trigger the flame alarm.
- **NOT VALIDATED:** sunlight false positives are not established as fully solved, and the shroud is an optical mitigation rather than sensor calibration.
- **NOT VALIDATED:** no calibrated flame distance, coverage, fire-detection, or life-safety claim is made.

The Phase C.1 mission did not change the flame firmware path.

### IR-zone physical observations motivating Phase C.1

- **PHYSICALLY OBSERVED:** sunlight can affect the front IR obstacle sensors.
- **PHYSICALLY OBSERVED:** the separately mounted low FRONT-LEFT and FRONT-RIGHT IR sensors help detect low obstacles that the higher servo-mounted HC-SR04 may miss.
- **PHYSICALLY OBSERVED:** some BOTH_IR situations caused PIKU to stop in the preceding firmware, motivating a bounded recovery sequence.

### Phase C.1 implementation status

The following items are **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**:

- FRONT-LEFT IR evidence stops motion and starts a RIGHT 35-degree relative pivot without requiring HC-SR04 confirmation.
- FRONT-RIGHT IR evidence stops motion and starts a LEFT 35-degree relative pivot without requiring HC-SR04 confirmation.
- BOTH_IR starts STOP -> configurable 220 ms reverse -> 150 ms settle -> fresh LEFT/FRONT/RIGHT scan -> clearest safe-side 35-degree pivot.
- AUTO calls the same `MotorController::leftTurn()` and `MotorController::rightTurn()` primitives used by Manual PIVOT; MPU6050 relative gyro-Z integration controls maneuver duration, with the existing timed fallback and hard timeout.
- Fresh IR status and a fresh front ultrasonic sample are required after every IR pivot before forward motion can resume.
- IR evidence has independent obstacle-zone authority and cannot be cancelled by a clear HC-SR04 reading.
- Opposite-side or BOTH_IR evidence can interrupt an active single-IR escape; repeated IR recovery is bounded to three attempts, cleared after five seconds of forward operation with clear IR zones.
- Known IR evidence resets blind-zone stuck evidence so the stuck detector does not compete for the same event.
- The implementation remains timestamp/state driven; no blocking wait or delay was added.
- Minimal dashboard/JSON diagnostics expose IR zone, avoidance source, and IR recovery attempts without increasing the telemetry interval.

### Phase C.1 repository validation

- `pio run -t clean`: **PASS**
- Clean `pio run` for `esp32-s3-n16r8`: **PASS**, no compiler warnings reported
- RAM: 53,468 bytes of 327,680 bytes (16.3%)
- Flash: 1,008,993 bytes of 6,553,600 bytes (15.4%)
- Embedded JavaScript syntax validation using the installed Deno parser without executing browser code: **PASS**
- Dashboard unique-ID, direct-DOM-reference, inline-handler, and new IR-diagnostic binding checks: **PASS**

Phase C.1 remains pending physical validation. In particular, this record does not yet claim correct 35-degree turn accuracy, successful bounded BOTH_IR recovery, reliable chained IR transitions, sunlight robustness, or measured responsiveness for the new behavior.

---

## Final AUTO Navigation Polish - Physical Stress-Test Record - 2026-09-09

### Traceability and evidence boundary

- Firmware under physical test: the uploaded Phase C.1 working firmware that preceded the final-polish implementation.
- Base/HEAD firmware anchor: 4fa34e144bc63a45a2e241b245cf5ff90745586b.
- Final-polish working branch: final-auto-navigation-polish.
- The final-polish changes described in the following build-verification section were not uploaded during this task and are not physically validated by these observations.

### Newly reported physical observations

- **PHYSICALLY OBSERVED:** extensive, aggressive AUTO testing was performed with many obstacles presented around the front-left and front-right IR sensing zones.
- **PHYSICALLY OBSERVED / APPROXIMATE:** the tested Phase C.1 obstacle response was assessed as approximately 80% healthy under this stress. This is a qualitative success estimate, not a measured detection or recovery rate.
- **PHYSICALLY OBSERVED:** PIKU handled many IR obstacle presentations correctly.
- **PHYSICALLY OBSERVED:** after some repeated obstacle encounters, PIKU stopped and remained stopped until AUTO was selected again manually.
- **PHYSICALLY OBSERVED:** directional inconsistency occurred in some IR cases: a LEFT IR obstacle could result in motion toward LEFT, and a RIGHT IR obstacle could result in motion toward RIGHT.
- These stop-latch and direction observations motivated the final unified STOP -> short reverse -> fresh scan -> safe escape selection -> spin -> fresh re-evaluation architecture. This motivation is not evidence that the new implementation works physically.
- **PHYSICALLY OBSERVED:** the installed reflective IR sensors failed to detect the tested black obstacle.
- This black-target result is recorded as a reflective-sensor/material/geometry limitation, not a software failure. Reflective IR response depends strongly on target reflectivity, surface finish, angle, and geometry.
- The HC-SR04 may detect a particular black object if that target intersects the ultrasonic sensing zone and returns usable acoustic energy, but this record makes no universal black-object coverage claim.
- **PHYSICALLY OBSERVED:** standalone SoftAP dashboard control operated successfully.
- **PHYSICALLY OBSERVED:** STA/Home Wi-Fi dashboard control also operated successfully.
- **PHYSICALLY OBSERVED:** control response and general operation were good on both network paths.
- **NOT MEASURED:** SoftAP and STA command latency, release-to-stop latency, jitter, and packet-loss distributions were not instrumented; the responsiveness evidence remains qualitative.
- **PHYSICALLY OBSERVED / APPROXIMATE:** the existing approximately two-second continuous-clear environmental-hazard recovery continued to stop, wait, scan, and resume successfully.
- **PHYSICALLY OBSERVED:** everything else exercised in this stress session remained generally healthy.

The earlier flame-shroud, flame optical limitation, MQ-2, battery, networking, and navigation observations above remain part of the record and are not superseded by this section.

### Final AUTO Navigation Polish - Build Verification

Status: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**

- Universal ordinary-obstacle pipeline: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. The non-blocking sequence is immediate STOP -> configurable 220 ms short reverse -> 150 ms settle -> fresh 170/90/10-degree LEFT/FRONT/RIGHT scan -> safe side selection -> 35-degree relative SPIN -> STOP/settle -> fresh IR plus front-ultrasonic re-evaluation.
- AUTO Manual-SPIN reuse: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. AUTO calls the same MotorController spinLeft and spinRight primitives used by Manual SPIN. Manual PIVOT and Manual SPIN routes were not modified.
- Direction policy: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. LEFT IR (GPIO7) gives RIGHT escape preference; RIGHT IR (GPIO8) gives LEFT escape preference. Fresh unsafe-side evidence overrides the preference.
- BOTH IR and HC-SR04 recovery: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. Both use the fresh three-direction scan and safe-side spin pipeline.
- Autonomous blocked retry: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. After bounded active recovery attempts, or when neither side is safe, AUTO remains stopped in non-blocking BLOCKED_RETRY_WAIT, periodically rescans, and does not require manual AUTO re-selection for an ordinary recoverable obstacle.
- Fresh post-maneuver arbitration: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. Forward motion is permitted only after current IR is clear and a newly requested front ultrasonic sample is trustworthy and clear.
- Stuck integration: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. Blind-zone/mechanical-restraint suspicion enters the same bounded recovery and stopped autonomous retry path instead of a permanent exhausted-attempt latch.
- Relative spin control: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. The target is configurable at 35 degrees; the short-term gyro-Z accumulator resets at spin start and retains stopped-sample bias correction, deadband, tolerance, timed fallback, and hard timeout. This is relative rotation only, not compass heading, absolute yaw, localization, or odometry.
- Environmental recovery: **PRESERVED / CLEAN-BUILD VERIFIED**. The 2,000 ms uninterrupted hazard-clear wait and fresh recovery scan remain. The earlier physical observation above applies to the preceding uploaded firmware; regression testing of this final build remains pending.
- Flame sensing: **PRESERVED / CLEAN-BUILD VERIFIED**. GPIO6 digital DO, active HIGH, persistence logic, SafetyManager authority, and the installed black optical shroud were not changed by this mission.
- Responsiveness architecture: **PRESERVED / CLEAN-BUILD VERIFIED / PHYSICAL REGRESSION TEST PENDING**. Navigation remains timestamp/state driven with no delay or blocking wait loop added; the Phase B HTTP-first, STOP/mode, deadman, telemetry, OLED, DHT, IMU, and nRF scheduling architecture was not restructured.
- Dashboard diagnostics: **IMPLEMENTED / CLEAN-BUILD VERIFIED / PHYSICAL VALIDATION PENDING**. The compact view and JSON expose AUTO state, IR zone/source, preferred and selected direction, current/target relative spin, completion reason, recovery attempt, blocked retry count, stuck state, and L/F/R scan evidence.

Verification completed on branch final-auto-navigation-polish at HEAD 4fa34e144bc63a45a2e241b245cf5ff90745586b:

- Clean PlatformIO build: **PASS**, zero compiler warnings observed.
- RAM: 53,524 / 327,680 bytes (16.3%).
- Flash: 1,009,317 / 6,553,600 bytes (15.4%).
- Embedded JavaScript syntax: **PASS**.
- DOM/handler and new AUTO binding consistency: **PASS**.
- Pin uniqueness and GPIO6/GPIO7/GPIO8 mapping: **PASS**.
- git diff --check: **PASS**; line-ending notices only.
- Firmware upload: **NOT PERFORMED**.
- Flash erase: **NOT PERFORMED**.
- Commit/push: **NOT PERFORMED**.

Required final physical validation remains the Manual-versus-AUTO SPIN reference, isolated LEFT/RIGHT/BOTH IR cases, HC-SR04 front obstacle, changing/chained obstacles, blocked autonomous retry, black-target limitation, SoftAP and STA control regression, and STOP during every recovery phase. No final-polish physical success, exact angle, measured latency, or guaranteed obstacle coverage is claimed here.

---

## Final PIKU 2.1 Rev A Physical Validation - 2026-09-10

### Evidence scope and tested configuration

- **PHYSICALLY OBSERVED:** the final unified AUTO Navigation Polish firmware was uploaded before this repository-freeze mission and was aggressively exercised for approximately 30 minutes.
- The operator reported satisfaction with the resulting prototype behavior.
- The installed test configuration retained flame DO on GPIO6 with active-HIGH firmware logic and the small black optical shroud, LEFT IR on GPIO7, RIGHT IR on GPIO8, the servo-mounted HC-SR04, MQ-2, and temperature sensing.
- No behavioral firmware change was made during this final evidence and repository-freeze mission.
- Observations in this section are functional prototype evidence. They do not establish production certification, industrial safety certification, calibrated fire or gas detection, formal RF coverage, or deterministic control latency.

### Power and useful runtime

- **PHYSICALLY OBSERVED:** the 18650 batteries were fully charged at the beginning of the final test.
- **PHYSICALLY OBSERVED / APPROXIMATE:** PIKU operated continuously for approximately 30 minutes.
- **PHYSICALLY OBSERVED:** control, telemetry, sensing, and AUTO operation remained functional during the useful battery period.
- **PHYSICALLY OBSERVED / APPROXIMATE:** after approximately 30 minutes, the robot/network disconnected as battery power became low.
- This is an observed prototype battery-runtime limitation for the tested battery and operating conditions.
- **NOT MEASURED:** voltage, current, state of charge, and load versus time were not instrumented, so this result is not a formally characterized endurance curve or guaranteed runtime.

### SoftAP, STA/Home Wi-Fi, telemetry, and responsiveness

- **PHYSICALLY OBSERVED:** the final 30-minute session used STA/Home Wi-Fi.
- **PHYSICALLY OBSERVED / APPROXIMATE:** stable practical control was observed within approximately 30 ft, or about 9 m, in the tested environment.
- **PHYSICALLY OBSERVED:** control response and telemetry were good during the session.
- Earlier testing also physically observed successful standalone SoftAP operation and practical responsiveness.
- Final prototype evidence therefore supports successful operation through both standalone SoftAP and STA/Home Wi-Fi in their tested environments.
- **NOT MEASURED:** no latency distribution, release-to-stop timing distribution, packet-loss study, RF survey, or interference characterization was collected. Responsiveness remains a qualitative physical observation, and the approximate range is not a guaranteed RF radius or coverage specification.

### Unified AUTO obstacle recovery and relative spin

- **PHYSICALLY OBSERVED:** the final unified AUTO obstacle-recovery firmware was stress-tested aggressively for approximately 30 minutes.
- **PHYSICALLY OBSERVED:** PIKU repeatedly detected and recovered from representative obstacles and continued autonomous operation without reproducing the previous ordinary-obstacle permanent AUTO latch problem.
- **PHYSICALLY OBSERVED:** the STOP -> short reverse -> fresh LEFT/FRONT/RIGHT scan -> safe-side selection -> SPIN -> fresh obstacle re-evaluation workflow was exercised.
- **PHYSICALLY OBSERVED:** AUTO spin behavior was improved compared with the preceding pivot implementation.
- Configured relative-rotation target: **35 degrees**.
- **PHYSICALLY OBSERVED / APPROXIMATE:** repeated AUTO avoidance spins were checked at approximately 30-35 degrees.
- The result is short-term relative gyro-Z-assisted rotation only. It is not compass heading, absolute yaw, localization, wheel odometry, or SLAM, and it is not a claim of calibrated angular accuracy.

### IR obstacle sensors

- Authoritative installed mapping: LEFT IR = GPIO7; RIGHT IR = GPIO8.
- **PHYSICALLY OBSERVED / APPROXIMATE:** the installed low-mounted reflective IR sensors detected representative obstacles at approximately 7 cm in the tested setup.
- The approximately 7 cm result depends on the tested targets, environment, sensor adjustment, surface angle, and geometry; it is not a guaranteed range.
- **PHYSICALLY OBSERVED:** the installed reflective IR sensors did not reliably detect the tested black objects.
- This is an explicit known hardware limitation. Reflective IR response depends strongly on surface reflectivity/material, geometry, and optical conditions. No universal black-object detection claim is supported.

### HC-SR04 configured software thresholds

These values are verified firmware configuration, not newly instrumented physical distances:

| Constant | Configured threshold | Role |
| --- | ---: | --- |
| AUTO_OBSTACLE_CM | 20.0 cm | A trustworthy front AUTO distance at or below this value initiates avoidance |
| OBSTACLE_CM | 25 cm | General obstacle/caution threshold |
| NEAR_OBSTACLE_CM | 7 cm | Separate near-obstacle safety role |
| MANUAL_FORWARD_STOP_CM | 25 cm | Manual forward stop threshold |
| MANUAL_FORWARD_CLEAR_CM | 30 cm | Manual forward clear hysteresis threshold |

The final test did not instrument HC-SR04 detection at exactly 20 cm. The 20 cm AUTO value and the separate 25 cm and 7 cm values must not be conflated.

### Environmental sensing and hazard recovery

- **PHYSICALLY OBSERVED:** no false hazard alarms occurred during the final approximately 30-minute session with the current shrouded flame-sensor configuration.
- **PHYSICALLY OBSERVED:** flame sensing worked successfully in the tested configuration.
- **PHYSICALLY OBSERVED:** MQ-2 gas sensing/alarm behavior worked successfully.
- **PHYSICALLY OBSERVED:** temperature sensing/alarm behavior worked successfully.
- **PHYSICALLY OBSERVED / APPROXIMATE:** the environmental recovery sequence remained functional: hazard -> STOP -> continuously clear for approximately two seconds -> fresh scan -> AUTO resumes.
- Earlier observations of sunlight and optical false triggering by the unshielded low-cost flame module remain valid history. The installed black optical shroud improved the tested configuration; it does not prove that future false alarms are impossible.
- MQ-2 evidence remains qualitative and relative. No calibrated ppm claim is made.

### Final assessment and freeze

PIKU 2.1 Rev A standalone firmware is now considered:

**PHYSICALLY VALIDATED AS A FUNCTIONAL RESEARCH PROTOTYPE**

and

**FROZEN FOR THIS DEVELOPMENT STAGE**

This designation does not mean production certified, industrial safety certified, a calibrated fire detector, a calibrated gas detector, or a formally validated RF system.

Explicit remaining limitations are:

1. Approximately 30 minutes of useful runtime was observed in the final battery test; a battery endurance curve was not instrumented.
2. The reflective IR sensors remain weak with the tested black targets.
3. The low-cost flame module retains optical and environmental susceptibility despite the effective shroud in this session.
4. MQ-2 response remains qualitative and uncalibrated.
5. RF range and command latency were not formally instrumented.
6. The platform has no wheel odometry, absolute localization, or SLAM.

Further standalone PIKU 2.1 Rev A feature expansion is intentionally stopped. Future improvements belong to later hardware revisions or CPS integration stages.
