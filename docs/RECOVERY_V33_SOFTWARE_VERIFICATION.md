# V3.3 software candidate verification

Standalone is the Robot-brain authority. Starting branch `master`, HEAD `93a44eff1445fb675fdf737645988436b1883cbc`; candidate branch `codex/reva-v33-deterministic-recovery-20261003`. V3.2 physical evidence remains historical and does not validate this candidate.

Run `python tests/run_host_checks.py`, `pio run -e esp32-s3-n16r8`, and `git diff --check`. Baseline: ten native C++ policies, three Python scripts, embedded JS parse, firmware build PASS. Candidate: eleven native C++ policy executables, three retained Python scripts, four unittest cases including the actual production NavigationManager with hardware doubles, embedded JS parse PASS. Final firmware memory results are recorded after the final gate below.

Production navigation cases execute normal 35-degree turning, immediate boxed recovery, six started/interrupted attempts, cooldown exhaustion, IR five-pulse exhaustion across retry, sonar fault/stale STOP with two-sample reacquisition, safe same-episode continuation, brief handoff re-block, hazard/critical preemption preserving budget, explicit STOP/mode reset, and sustained forward success. Pure policy tests cover pre-completion attempt charging, pause/retry history, short handoff non-reset and five cancelled pulses exhausting the episode.

The existing `AUTO_OBSTACLE_RETRY_RESET_FORWARD_MS = 5000` is reused. The robot must have a trusted >=30 cm/OPEN handoff followed by 5000 ms actual forward-step time with continued fresh forward-safe evidence and no IR/sonar re-block. Stop/center/sample steps stay nonblocking. A sonar or hazard pause cancels motor execution, preserves episode counts, and restarts from a front checkpoint. A generic turn path is guarded against bypassing an active episode.

Override permission is independent of live motion authorization. Explicit enable checks MANUAL, eligible hazard, critical/fault exclusions, confirmation and live session. STOP/RELEASE and session replacement stop motor/held state but preserve permission. Expired sessions cannot authorize hazardous movement; reconnect needs a fresh press. The safety loop retains critical temperature/flame-sample faults, stable-clear revocation and directional Manual sonar/IR gates.

Only audited gyro and scan cleanup were backported. CPS callbacks/registration/liveness/RF leases stay outside Standalone. See [core audit](STAGE5D_CORE_DIFFERENCE_AUDIT.md).

Documentation classification: README, SOFTWARE_EXPLANATION, CHANGELOG and architecture sources are living and updated; TESTING now points to this current gate while preserving old steps. Versioned V3/V3.1/V3.2 software/physical records, Stage1/2, V2, correction/latency and dated testing evidence are historical. NAV_TRACE_SCHEMA remains a live trace wire-schema description; new rich episode data is on existing `/data`, not a changed trace packet. No tracked figure regeneration workflow exists; diagram sources updated and rendering remains separate. Owner PNGs are preserved.

**Stage 5D / V3.3 / Dashboard v1.0 physical validation NOT RUN.** The [owner checklist](PIKU_2.1_REVA_V33_PHYSICAL_VALIDATION.md) is pending. No physical angle, displacement, performance or reliability claim follows from host checks. RF PA/range engineering NOT RUN and remains deferred.

Final candidate build PASS: RAM 58,892 / 327,680 bytes (baseline 58,740; +152); flash 1,040,581 / 6,553,600 bytes (baseline 1,038,217; +2,364). `git diff --check` PASS. Firmware internal version string remains unchanged; V3.3 is the candidate behavior identifier.
