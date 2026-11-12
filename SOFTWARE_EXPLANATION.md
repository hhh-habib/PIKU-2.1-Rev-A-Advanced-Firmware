# How the current PIKU 2.1 Rev A firmware works

Scope: standalone checkpoint, 13 November 2026. Production implementation remains `7ee2b03b17657c0e2d79813560130594af73c5c2`. This guide supersedes the former mixed V3.3/V4.5 walkthrough; [dated records](docs/README.md) retain their algorithms and evidence.

## Control and ownership

`src/main.cpp` services inputs, sensing, safety/control, motors/navigation, networking and presentation cooperatively. Modules are not independent real-time threads. `WebDashboard` manages HTTP commands/sessions/sequences; `DriveCommandHandler` and `NavigationManager` dispatch admitted Manual/AUTO work through `MotorController`. Current authority, safety and obstacle gates can stop/reject motion. Presentation never creates movement authority. See the [module schematic](architecture_source/current_standalone_20261113.mmd).

STOP/release retire held/intended work. A delayed request, stale session or successful heartbeat cannot replay an old direction. The STOP/mode fence protects ordering across outstanding responses. Restored Manual readiness requires confirmed current authority and a fresh user press; its serviced command deadman is 750 ms.

## Sensor evidence

`SensorManager` tracks validity, timestamps and orientation/generation for sonar. Trusted finite distance, operational clear/no-echo, fault and stale evidence are distinct. Scan decisions require compatible new evidence; an incidental sample cannot replace quorum. Front IR remains independent local obstacle evidence. Failed environmental reads neither renew successful timestamps nor clear confirmed hazards.

Manual forward hysteresis blocks at 20 cm or less, clears at 25 cm or more and retains state in between. Trusted no-echo is operational open; unavailable sonar is unknown. Front IR and independent safety still apply. Backward/turns remain supervised: there is no complete swept-volume or rear view.

## Restored ordinary AUTO and recovery

Final navigation follows restored V3.2 behavior. Ordinary front/IR interventions use existing bounded reverse, fresh directional scans and nominal 35-degree turns, with fresh accepted evidence before forward admission.

`StuckRecoveryV2Policy` and `V2EscapeDecision` support committed-side recovery. A chosen side at or below the configured 7 cm veto is unusable. Each mission permits at most seven started nominal 25-degree increments, five IR reverse pulses and 30 seconds elapsed time. STOP, authority, hazards or sensing can interrupt sooner. Exhaustion stops motors; after 2.5 seconds new scan/retry can be admitted. Repeated bounded missions are not a lifetime escape guarantee.

Sonar faults stop/cancel actuator execution, center/reacquire fresh front evidence and retain only context allowed by restored policy. Confirmed environmental hazards abort old recovery motion. Unfinished environmental recovery needs a new compatible scan after interruption. Intermediate V4.5 six-turn/45-second/saved-turn-resumption descriptions are historical.

Nominal angle, internal gyro completion threshold and measured chassis turn are distinct. Existing completion addition is 1 degree (26/36 degrees in the applicable gyro path), with bounded deadlines. Constants do not establish physical accuracy; [Task 3](docs/RESEARCH_AUDIT/07_TASK_03_METHODOLOGY_AND_TURN_ANGLES_20261109.md) qualifies source behavior and independent protractor readings.

## Manual hazard permission

Explicit Manual Override bypasses only eligible flame/gas inhibition. Critical temperature, STOP, session/deadman, known obstacles and other gates remain. Permission can persist across release/temporary link loss, while each movement still requires current authority. Mode exit, reboot, stable hazard clear and applicable non-overridable safety revoke it. Permission does not declare environmental safety.

## IMU and display

`ImuManager` checks identity, transaction/byte counts and complete data-ready frames. Zero/FF/incomplete frames cannot renew health/age; stationary zero gyro with gravity remains valid. Boot absence/runtime faults retry cooperatively. Reset/configuration waits for existing stopped-motor state; health restoration needs new accepted samples. The manager commands no movement.

`DisplayManager` shows compact distance, temperature/humidity, relative gas, flame, radio and cached STA address. Unchanged text avoids redraws; normal evaluation is capped at one second. Confirmed environmental alert transitions bypass routine display deferral. Distance/IR do not create environmental full-screen alerts. Module presence alone does not prove a live radio peer.

## Communication

The onboard browser uses existing HTTP command/session/mode paths, `/ping` and `/data`. Live/Stale/Offline describes response/telemetry evidence, never motion authority. Request admission/write budgets retire failed sockets; STA application retry preserves association/scan state. These are configured software budgets, not measured whole-loop deadlines. `RadioManager` remains a bounded secondary experimental path.

The package includes owner-authorized robot SoftAP defaults unchanged. Personal STA credentials may be provisioned locally and are outside published defaults. Use the delivered configuration and addresses reported by serial/dashboard/OLED status.

## Verification boundaries

[Reproduction](docs/REPRODUCIBILITY.md) describes the supported suite and full-clone dependencies. Hardware doubles execute production bodies without proving real stop latency, RF reliability or sensor calibration. [Release checks](docs/release/verification_20261113.json) identify current software verification.

Historical V3.2 owner reports and November pilot/corridor/angle records retain separate scope. Current hashes, successful builds and reused Stage-2 runtime labels do not independently identify historical experiment images. Preserve those distinctions in the paper.
