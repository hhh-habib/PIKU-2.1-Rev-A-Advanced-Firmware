#!/usr/bin/env python3
"""Capture opt-in PIKU navigation CSV from a supervised serial session."""
import argparse
import csv
import re
import sys
import time
from pathlib import Path

import serial

HEADER = ("NAVCSV,schema_version,firmware_id,boot_id,uptime_ms,run_id,kind,"
          "mode,nav_state,decision,left_cm,left_validity,front_cm,front_validity,"
          "right_cm,right_validity,ir_left,ir_right,gyro_status,"
          "gyro_relative_est_deg,intended_turn_deg,observed_turn_est_deg,"
          "recovery_attempt,motor_stop,flame,gas,temp,elapsed_ms,outcome,"
          "dropped_total,checksum_fnv1a32")


def fnv1a32(payload: bytes) -> int:
    value = 2166136261
    for byte in payload:
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


def valid_record(line: str) -> bool:
    if not line.startswith("NAVCSV,1,"):
        return False
    try:
        columns = next(csv.reader([line]))
        payload, checksum = line.rsplit(",", 1)
        return (len(columns) == 31 and
                fnv1a32(payload.encode("ascii")) == int(checksum, 16))
    except (ValueError, UnicodeEncodeError):
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="ESP32-S3 serial port, e.g. COM7")
    parser.add_argument("--output", required=True, type=Path, help="new CSV path")
    parser.add_argument("--run-id", required=True, help="1-16 ASCII letters, digits, _ or -")
    parser.add_argument("--duration", type=float, default=0,
                        help="seconds to capture; 0 runs until Ctrl-C")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,16}", args.run_id):
        parser.error("--run-id must be 1-16 ASCII letters, digits, _ or -")
    if args.duration < 0:
        parser.error("--duration must be nonnegative")
    count = 0
    corrupt = 0
    try:
        with args.output.open("x", encoding="utf-8", newline="") as out:
            out.write(HEADER + "\n")
            with serial.Serial(args.port, 115200, timeout=1, write_timeout=1) as link:
                # Many ESP32-S3 serial paths reboot on open.
                time.sleep(2)
                link.write(f"TRACE ON {args.run_id}\n".encode("ascii"))
                deadline = time.monotonic() + args.duration if args.duration else None
                try:
                    while deadline is None or time.monotonic() < deadline:
                        line = link.readline().decode("ascii", errors="replace").strip()
                        if not line.startswith("NAVCSV,1,"):
                            continue
                        if valid_record(line):
                            out.write(line + "\n")
                            out.flush()
                            count += 1
                        else:
                            corrupt += 1
                except KeyboardInterrupt:
                    pass
                finally:
                    link.write(b"TRACE OFF\n")
    except (OSError, serial.SerialException) as error:
        print(f"capture failed: {error}", file=sys.stderr)
        return 2
    print(f"captured {count} valid NAVCSV records; discarded {corrupt} corrupt records in {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
