#!/usr/bin/env python3
"""Capture physical keyboard byte sequences through the Tab5 login bridge."""

from __future__ import annotations

import argparse
import re
import time
import uuid

import serial


def write_chunks(
    ser: serial.Serial,
    payload: bytes,
    chunk_size: int,
    chunk_delay: float,
) -> None:
    for start in range(0, len(payload), chunk_size):
        ser.write(payload[start : start + chunk_size])
        ser.flush()
        if chunk_delay:
            time.sleep(chunk_delay)


def shell_single_quote(text: str) -> str:
    return "'" + text.replace("'", "'\\''") + "'"


def build_capture_command(
    device: str,
    key_mode: str,
    duration: int,
    start_marker: str,
    end_marker: str,
) -> str:
    mode_sequence = "\\033[?1h" if key_mode == "app-cursor" else "\\033[?1l"
    screen_text = (
        f"\\033[2J\\033[HStage9 key capture: {device} / {key_mode}\\r\\n"
        f"Capture window: {duration}s\\r\\n"
        "Press the requested physical keys now.\\r\\n\\r\\n"
    )
    return (
        f"printf {shell_single_quote(mode_sequence)}; "
        f"printf {shell_single_quote(screen_text)}; "
        f"printf '\\r\\n{start_marker}\\r\\n'; "
        "stty raw -echo -ixon -ixoff min 1 time 0 2>/dev/null; "
        f"timeout {duration}s cat -vET; rc=$?; "
        "stty sane -ixon -ixoff 2>/dev/null; "
        f"printf '\\r\\n{end_marker}:rc=%s\\r\\n' \"$rc\""
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--device", choices=("usb", "a164"), required=True)
    parser.add_argument("--mode", choices=("normal", "app-cursor"), default="normal")
    parser.add_argument("--duration", type=int, default=60)
    parser.add_argument("--open-delay", type=float, default=2.0)
    parser.add_argument("--chunk-size", type=int, default=96)
    parser.add_argument("--chunk-delay", type=float, default=0.04)
    args = parser.parse_args()

    if args.duration < 5:
        parser.error("--duration must be at least 5 seconds")
    if args.chunk_size < 1:
        parser.error("--chunk-size must be at least 1")
    if args.chunk_delay < 0:
        parser.error("--chunk-delay must be non-negative")

    nonce = uuid.uuid4().hex[:8].upper()
    label = f"{args.device.upper()}_{args.mode.upper().replace('-', '_')}_{nonce}"
    start_marker = f"__TAB5_KEY_START_{label}__"
    end_marker = f"__TAB5_KEY_END_{label}__"
    command = build_capture_command(
        args.device,
        args.mode,
        args.duration,
        start_marker,
        end_marker,
    )
    payload = (command + "\r").encode("utf-8")
    end_re = re.escape(end_marker.encode("ascii")) + rb":rc=([0-9]+)"
    response = bytearray()

    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.2
    ser.rtscts = False
    ser.dsrdtr = False
    ser.dtr = False
    ser.rts = False

    with ser:
        time.sleep(args.open_delay)
        ser.reset_input_buffer()
        ser.write(b"\x03\r")
        ser.flush()
        time.sleep(0.4)
        ser.reset_input_buffer()
        write_chunks(ser, payload, args.chunk_size, args.chunk_delay)

        deadline = time.monotonic() + args.duration + 8.0
        while time.monotonic() < deadline:
            waiting = ser.in_waiting
            if waiting:
                response.extend(ser.read(waiting))
                if re.search(end_re, response):
                    break
            else:
                time.sleep(0.05)

    print(f"captured {len(response)} bytes from {args.device} keyboard ({args.mode})")
    print(f"start_marker={start_marker}")
    print(f"end_marker={end_marker}")
    if response:
        print(repr(bytes(response[-2000:])))

    match = re.search(end_re, response)
    if match is None:
        print("keyboard capture failed: end marker not captured")
        return 1

    rc = int(match.group(1))
    if rc not in (0, 124):
        print(f"keyboard capture failed: cat/timeout rc={rc}")
        return 1

    print(f"keyboard capture completed: rc={rc}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
