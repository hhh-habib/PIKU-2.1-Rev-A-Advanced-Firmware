# Watchdog episode boundary ledger

Recorded before production correction, 2026-10-05. This is the owner-authorized F4 implementation boundary, not physical evidence.

| Field/context | Expiry / stationary reassessment / provisional AUTO | Fresh blocked L/F/R new-episode boundary | Sustained 5000 ms safe-forward completion |
| --- | --- | --- | --- |
| Accumulated old active work | Freeze and retain; never resume exhausted clock | Replace with new zero active work; start new clock | Clear with old episode |
| Episode start | Retain | Set to boundary time | Clear |
| Six-turn ledger | Retain | Zero for new episode | Clear |
| Five-pulse mission ledger | Retain | Zero for new episode | Clear |
| Event pulse/output | Cancel; no deferred reverse | Clear; only freshly selected output may start | Clear |
| Committed side | Retain as history, no exhausted movement authority | Invalidate; select using existing fresh L/F/R algorithm | Clear |
| Scan/sample targets | Invalidate old movement decisions; request fresh stationary evidence | Use complete post-expiry trusted L/F/R; invalidate old pending outputs | Ordinary front gates remain |
| Gyro/turn/reverse output | Cancel immediately | Reset; recheck current IMU and direction gates | No replay |
| Forward proof | Zero on interruption; provisional handoff retains resources | Zero | Existing 5000 ms navigation evidence completes episode |
| Cooldown | Preserve expiry deadline across holds; no refill | Require existing 2500 ms before boundary; clear for new active episode | Clear |
| Avoidance history | Preserve while exhausted | Clear for distinct episode, using existing policy | Clear |
| Other hold causes | Preserve STOP/re-arm, temperature, sonar, hazard, integration and ownership gates | All must permit; ordinary turn/pulse/sensor holds cannot renew resources | Higher-priority interruption cancels proof |

Each renewed episode retains 30000 ms accumulated active work, six turns and five pulses. Repeated distinct episodes have no claimed lifetime movement bound. No threshold, speed, turn size, pulse duration, packet or operator STOP/RELEASE policy change is authorized.
