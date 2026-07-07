#!/usr/bin/env python3
"""Capture Tab5 USB CDC output immediately after opening the port."""

from __future__ import annotations

import argparse
import time

import serial


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--duration", type=float, default=5.0)
    args = parser.parse_args()

    captured = bytearray()
    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.1
    ser.rtscts = False
    ser.dsrdtr = False
    ser.dtr = False
    ser.rts = False

    with ser:
        deadline = time.monotonic() + args.duration
        while time.monotonic() < deadline:
            waiting = ser.in_waiting
            if waiting:
                captured.extend(ser.read(waiting))
            else:
                time.sleep(0.02)

    text = captured.decode("utf-8", errors="replace")
    print(text, end="" if text.endswith("\n") else "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
