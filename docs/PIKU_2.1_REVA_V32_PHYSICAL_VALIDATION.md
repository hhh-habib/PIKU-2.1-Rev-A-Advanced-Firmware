# PIKU 2.1 Rev A V3.2 - supervised physical validation

**Date:** 2026-10-01

**Frozen executable firmware:** `41281d8325d88c70e074e008a2550a1164102132`

**Evidence source:** final supervised functional test reported PASS by the project owner. This record does not assert that repository software checks themselves exercised hardware.

The owner approved the V3.2 standalone firmware after final supervised physical testing. The reported functional confirmation covers:

| Area | Owner-reported functional result |
| --- | --- |
| Ordinary AUTO obstacle avoidance | Satisfactory |
| Immediate boxed-in recovery | Satisfactory |
| Recovery direction lock | Satisfactory |
| Recovery exit to normal AUTO | Satisfactory |
| IR-triggered bounded recovery backoff | Satisfactory |
| Manual movement and finite-distance Forward protection/hysteresis | Satisfactory |
| Intentional Manual flame/gas override | Satisfactory |
| `ClearNoEcho` treated operationally as OPEN | Satisfactory |
| Sonar fault/reconnect and automatic front reacquisition | Satisfactory |
| AUTO continuation without reboot or a second AUTO click | Satisfactory |
| Dashboard VALID / OPEN / FAULT / STALE presentation | Satisfactory |
| Operator STOP and control behavior | Satisfactory |

The separate [V3.2 software verification record](RECOVERY_V32_SOFTWARE_VERIFICATION.md) identifies the build, host-test, JavaScript, RAM, and flash evidence. The [2026-09-10 physical testing record](PIKU_2.1_REV_A_TESTING.md) describes an earlier firmware state and preserves its own dated observations; its runtime, angle, and network observations are not assigned to the V3.2 test.

## Evidence boundary

This is supervised **functional physical validation**, not a statistically instrumented benchmark or industrial safety certification. The V3.2 test did not establish response-latency distributions, turn-angle error distributions, reverse displacement, RF reliability percentages, statistical success rates, calibrated gas concentrations, or guaranteed collision clearance. No rear obstacle sensor or encoder verifies the timed reverse distance. HC-SR04 `ClearNoEcho` is operationally open but cannot mathematically prove infinite free space or distinguish every disconnected LOW echo condition. Reflective IR does not cover the full swept volume.

The executable firmware commit above remains frozen. This documentation records the owner's reported outcome; it does not change sensor logic, motor control, navigation, dashboard code, or tests.
