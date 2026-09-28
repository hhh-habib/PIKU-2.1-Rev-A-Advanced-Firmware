# Optional navigation trace CSV (schema 1)

The firmware keeps a 16-record RAM ring, starts tracing OFF after every boot, and writes no trace data to flash. Commands on the 115200-baud serial console are `TRACE ON run_id` and `TRACE OFF`; `run_id` accepts at most 16 ASCII letters, digits, underscores or hyphens. Omit it to record `NOT_PROVIDED`. Trace output begins with `NAVCSV,` and includes a header. The serial transmitter sends at most 48 bytes per 20 ms service interval after control and sensing work; dropped records are counted. Each data row has an FNV-1a 32-bit checksum over the ASCII bytes before the final comma. The capture script rejects corrupt or interleaved rows; a rejected row or a ring drop means the dataset is incomplete.

For a supervised capture with pyserial:

```powershell
python tools/capture_nav_trace.py --port COM7 --output run01.csv --run-id run01 --duration 300
```

Use the actual port and a new output filename. Opening a serial port may reboot the ESP32-S3, producing a new boot ID. The script only captures `NAVCSV` lines, sends `TRACE OFF` on exit and never flashes or changes firmware.

| Column(s) | Meaning |
| --- | --- |
| `NAVCSV`, `schema_version` | Fixed record prefix and schema number 1. |
| `firmware_id`, `boot_id`, `uptime_ms`, `run_id` | Stage-2 firmware identity, random boot identifier, device milliseconds since boot, and operator label. Boot ID is not a clock or secret. |
| `kind`, `mode`, `nav_state`, `decision` | `EVENT` for a changed mode/state/decision/hazard/IR/outcome, otherwise a maximum 1 Hz `OBSERVATION`; mode and current firmware state/decision strings. Manual mode records `MANUAL` as nav state. |
| `left_cm`, `front_cm`, `right_cm` | Last AUTO scan distances in cm when finite, fresh and trusted. These are separate servo ray directions, not corridor widths or a map. Manual-mode scan distances are `NOT_MEASURED`. |
| `left_validity`, `front_validity`, `right_validity` | `DISTANCE_VALID`, `CLEAR_NO_ECHO`, `SENSOR_FAULT`, `STALE` or `NOT_SAMPLED`. A non-current nominal distance is marked `STALE`. `CLEAR_NO_ECHO` gives no finite distance. |
| `ir_left`, `ir_right` | Debounced low front IR zone flags (0/1). They are not rear coverage. |
| `gyro_status`, `gyro_relative_est_deg` | IMU health and the firmware's short-term relative gyro estimate. This is not a measured heading or translation. |
| `intended_turn_deg`, `observed_turn_est_deg` | Configured target and gyro relative estimate only for a turn decision. An absent or unusable estimate is `NOT_MEASURED`. The provisional 25-degree V2 motion is ON for the current physical-test build but has no physical validation. |
| `recovery_attempt`, `motor_stop` | V2 attempt count when enabled, otherwise normal obstacle attempt count; motor STOP flag. |
| `flame`, `gas`, `temp` | Distinct debounced critical-hazard flags, not raw gas concentration or fire certification. |
| `elapsed_ms`, `outcome`, `dropped_total` | Milliseconds since previous device record; software outcome `PENDING`, `CLEARANCE_REGAINED`, `TRAP_HOLD` or `ABORTED`; records dropped from the RAM ring before this record. |
| `checksum_fnv1a32` | Eight hexadecimal digits for FNV-1a over the preceding ASCII row bytes; useful for rejecting serial interleaving and truncation. |

`INVALID` in a distance cell means sensing is faulty, stale or otherwise unusable. `NOT_MEASURED` means no finite distance was obtained for that field. `CLEARANCE_REGAINED` is a firmware state after its hazard recovery path, **not** ground-truth robot position or guaranteed traversability. Label outcomes from supervised experiments separately, with external measurements where needed. No gas ppm, battery voltage, position, mapping, TinyML prediction or instrumented STOP latency is inferred from this trace.
