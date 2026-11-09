# B0–B4 controller provenance

## Current firmware-project clarification

The owner confirms that the physical robot's firmware was uploaded from **PIKU 2.1 Rev A Software Development**. Project identity is OWNER-CONFIRMED. Missing identity in the historical table below means the exact experiment-specific source revision or compiled image. It does not mean that the project is unknown. The confirmation does not identify five comparator binaries or map B0–B4 to the newest Git SHA.

The separate T01–T20 pilot, T21–T22 corridor tests and 40 angle-characterization readings are covered by [Task 3](07_TASK_03_METHODOLOGY_AND_TURN_ANGLES_20261109.md). Its owner-confirmed stopwatch limit and 80% primary-pilot setting do not fill missing historical B0–B4 protocol fields.

Read-only review, 9 November 2026. No firmware was reconstructed or changed.

| ID | Team manuscript description | Physical outcome provenance | Tested source/build | Relationship to current source | Status |
|---|---|---|---|---|---|
| B0 | Ultrasonic only; timed 90-degree turns | Owner-confirmed physical; Tables IV/V aggregates | Trial image/hash/date not located | Current source has dual IR, 35-degree gyro-assisted avoidance and hazard/lease logic; not an identified B0 build | AUTHOR_REPORTED physical; UNRESOLVED build |
| B1 | B0 plus dual front IR | Same owner confirmation | Not located | Source supports the feature, not historical variant identity | AUTHOR_REPORTED physical; UNRESOLVED build |
| B2 | Gyro-assisted 35-degree turns | Same owner confirmation | Not located | Angle and feedback both change versus B1, so no isolated gyro-effect inference | AUTHOR_REPORTED physical; UNRESOLVED build |
| B3 | Hazard priority; configured 1-second command expiry | Same owner confirmation | Not located | Current expiry is 0.75 s; cannot silently assign it as the 1-second B3 build | AUTHOR_REPORTED physical; UNRESOLVED build |
| B4 | Bounded recovery; configured 0.75-second expiry | Same owner confirmation | Exact trial build not located | Features exist currently; feature overlap does not identify trial binary | AUTHOR_REPORTED physical; UNRESOLVED build |

Missing files do not establish that physical trials were simulated/fabricated or never happened. Owner confirmation establishes reported provenance, not independent replication or binary identity. Do not equate earlier GitHub code with B0 without evidence.

## Implementation inspected

`include/RobotConfig.h`: ordinary target 35 degrees (line 110), recovery 25 degrees (113), seven turns (117), five IR back-offs (120), configured 30 s mission (121), 0.75 s Manual expiry (84), 2 s environment clear (129). `src/NavigationManager.cpp:982`/`:1016` implement gyro-aware turn handling; `:1351` onward covers committed recovery selection/checks.

Tags `v2.1-revA-final` and `revA-standalone-v3.2-physical-validated-20261001` already contain 35-degree avoidance and 0.75 s expiry. Neither is a five-variant experiment manifest. No B0–B4 implementation/build mapping was located in scoped source/docs/tests. The unrelated `B1 reset` state-case label is not a comparator.

## Owner clarifications on 9 November

Every impact counts, including repeated impacts within one encounter. Thus 38/10/17/17/19 are contact-event totals over 100 reported encounters per configuration, not counts of unique collision-positive encounters. Recovery means clearing the obstacle and continuing; time was not measured. Do not reinstate the original 0.55 m/20 s/40 s criteria as the physical protocol.

Table V angles/gaps were measured using protractor/ruler according to the owner. Instrument resolution, placement/reference points and reading protocol remain unspecified. Historical trial dates, exact tested-build identity, geometry/material, speed/battery and original sheets/videos would strengthen reproducibility if available. Label retrospective recollection as retrospective. Do not create 500 individual trial rows from aggregates.
