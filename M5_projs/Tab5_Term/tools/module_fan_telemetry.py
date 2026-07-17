#!/usr/bin/env python3
"""Control and read Tab5 Module Fan telemetry over USB Serial/JTAG."""

from __future__ import annotations

import argparse
import time

import serial


FRAME_PREFIX = b"\x1b]777;"
FRAME_TERMINATOR = b"\x07"
FAN_LINE_MARKER = b"TAB5FAN "


def command_frame(command: str) -> bytes:
    return FRAME_PREFIX + command.encode("ascii") + FRAME_TERMINATOR


def send_command(ser: serial.Serial, command: str) -> None:
    ser.write(command_frame(command))
    ser.flush()


def read_fan_line(
    ser: serial.Serial,
    deadline: float | None,
    expected_prefix: str | None = None,
) -> str:
    while deadline is None or time.monotonic() < deadline:
        raw = ser.readline()
        marker = raw.find(FAN_LINE_MARKER)
        if marker < 0:
            continue

        line = raw[marker:].decode("ascii", errors="replace").strip()
        if expected_prefix is None or line.startswith(expected_prefix):
            return line

    raise TimeoutError(f"missing Module Fan response: {expected_prefix or 'TAB5FAN'}")


def open_serial(args: argparse.Namespace) -> serial.Serial:
    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.2
    ser.write_timeout = args.timeout
    ser.rtscts = False
    ser.dsrdtr = False
    ser.dtr = False
    ser.rts = False
    ser.open()
    time.sleep(args.open_delay)
    ser.reset_input_buffer()
    return ser


def run_one_shot(ser: serial.Serial, action: str, timeout: float) -> None:
    commands = {
        "start": ("module-fan-log=start", "TAB5FAN LOG enabled=1"),
        "stop": ("module-fan-log=stop", "TAB5FAN LOG enabled=0"),
        "status": ("module-fan-log?", "TAB5FAN LOG "),
    }
    command, expected = commands[action]
    send_command(ser, command)
    print(read_fan_line(ser, time.monotonic() + timeout, expected))


def stream(ser: serial.Serial, timeout: float) -> None:
    send_command(ser, "module-fan-log=start")
    print(read_fan_line(ser, time.monotonic() + timeout, "TAB5FAN LOG enabled=1"))

    try:
        while True:
            print(read_fan_line(ser, None))
    except KeyboardInterrupt:
        pass
    finally:
        try:
            send_command(ser, "module-fan-log=stop")
            print(read_fan_line(ser, time.monotonic() + timeout, "TAB5FAN LOG enabled=0"))
        except (OSError, TimeoutError, serial.SerialException):
            pass


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("start", "stop", "status", "stream"))
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--open-delay", type=float, default=8.0)
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args()

    with open_serial(args) as ser:
        if args.action == "stream":
            stream(ser, args.timeout)
        else:
            run_one_shot(ser, args.action, args.timeout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
