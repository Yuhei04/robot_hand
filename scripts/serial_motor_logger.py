#!/usr/bin/env python3
"""Send simple serial servo commands and log responses to CSV."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import pathlib
import sys
import time


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send serial servo test commands and log TX/RX lines."
    )
    parser.add_argument("--port", required=True, help="Serial port path.")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate.")
    parser.add_argument(
        "--command",
        action="append",
        default=[],
        help="Command line to send. Can be passed multiple times.",
    )
    parser.add_argument(
        "--duration",
        type=float,
        default=10.0,
        help="Seconds to continue reading after sending commands.",
    )
    parser.add_argument(
        "--log",
        type=pathlib.Path,
        default=None,
        help="CSV log path. Defaults to logs/servo_test_<timestamp>.csv.",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=0.1,
        help="Serial read timeout in seconds.",
    )
    parser.add_argument(
        "--no-stop-on-exit",
        action="store_true",
        help="Do not send STOP before closing the serial port.",
    )
    return parser.parse_args()


def default_log_path() -> pathlib.Path:
    stamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    return pathlib.Path("logs") / f"servo_test_{stamp}.csv"


def import_serial():
    try:
        import serial  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency: pyserial. Install it with `python3 -m pip install pyserial`."
        ) from exc
    return serial


def write_row(writer: csv.writer, started_at: float, direction: str, payload: str) -> None:
    now = time.time()
    writer.writerow(
        [
            dt.datetime.fromtimestamp(now).isoformat(timespec="milliseconds"),
            f"{now - started_at:.3f}",
            direction,
            payload,
        ]
    )


def send_line(ser, writer: csv.writer, started_at: float, line: str) -> None:
    ser.write((line + "\n").encode("utf-8"))
    ser.flush()
    write_row(writer, started_at, "tx", line)


def main() -> int:
    args = parse_args()
    serial = import_serial()

    log_path = args.log or default_log_path()
    log_path.parent.mkdir(parents=True, exist_ok=True)

    started_at = time.time()
    with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser:
        with log_path.open("w", newline="") as log_file:
            writer = csv.writer(log_file)
            writer.writerow(["timestamp", "elapsed_s", "direction", "payload"])

            time.sleep(1.5)
            ser.reset_input_buffer()

            for command in args.command:
                line = command.strip()
                if not line:
                    continue
                send_line(ser, writer, started_at, line)
                time.sleep(0.1)

            try:
                deadline = time.time() + max(args.duration, 0.0)
                while time.time() < deadline:
                    raw = ser.readline()
                    if not raw:
                        continue
                    text = raw.decode("utf-8", errors="replace").rstrip()
                    write_row(writer, started_at, "rx", text)
                    print(text)
            finally:
                if not args.no_stop_on_exit:
                    send_line(ser, writer, started_at, "STOP")

    print(f"log: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
