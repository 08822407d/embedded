#!/usr/bin/env python3
"""Query or coordinate Tab5 and Linux login-UART baud changes."""

from __future__ import annotations

import argparse
import re
import sys
import time

import serial


FRAME_PREFIX = b"\x1b]777;"
FRAME_TERMINATOR = b"\x07"
STATE_RE = re.compile(
    r"TAB5CFG STATE active=(\d+) persisted=(default|\d+)"
    r" pending=(none|\d+)"
)
SUPPORTED_BAUDS = (115200, 230400, 460800, 921600)
TTY_RE = re.compile(r"^/dev/[A-Za-z0-9._/-]+$")


def open_serial(port: str, open_delay: float) -> serial.Serial:
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 115200
    ser.timeout = 0.1
    ser.write_timeout = 2.0
    ser.rtscts = False
    ser.dsrdtr = False
    ser.dtr = False
    ser.rts = False
    ser.open()
    time.sleep(open_delay)
    ser.reset_input_buffer()
    return ser


def collect(ser: serial.Serial, duration: float) -> bytes:
    response = bytearray()
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            response.extend(ser.read(waiting))
        else:
            time.sleep(0.02)
    return bytes(response)


def send_management(
    ser: serial.Serial, command: str, response_window: float = 0.8
) -> str:
    frame = FRAME_PREFIX + command.encode("ascii") + FRAME_TERMINATOR
    ser.write(frame)
    ser.flush()
    return collect(ser, response_window).decode("utf-8", errors="replace")


def query_state(ser: serial.Serial) -> tuple[int, str, str]:
    response = send_management(ser, "login-uart?")
    matches = list(STATE_RE.finditer(response))
    if not matches:
        raise RuntimeError(
            "Tab5 did not return a login-UART state response. "
            f"Captured: {response[-300:]!r}"
        )
    match = matches[-1]
    return int(match.group(1)), match.group(2), match.group(3)


def send_shell_and_wait(
    ser: serial.Serial, command: str, marker: str, timeout: float
) -> str:
    ser.reset_input_buffer()
    ser.write(command.encode("ascii") + b"\r")
    ser.flush()

    response = bytearray()
    marker_bytes = marker.encode("ascii")
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            response.extend(ser.read(waiting))
            if marker_bytes in response:
                return response.decode("utf-8", errors="replace")
        else:
            time.sleep(0.02)
    raise RuntimeError(
        f"Linux shell marker {marker!r} was not received. "
        f"Captured: {bytes(response[-500:])!r}"
    )


def preflight_linux_baud(
    ser: serial.Serial, tty: str, active_baud: int, target_baud: int
) -> None:
    success_marker = f"TAB5_BAUD_PREFLIGHT_{target_baud}_0"
    command = (
        "sh -c '"
        f"stty -F {tty} {target_baud} 2>/dev/null; rc=$?; "
        "sleep 0.4; "
        f"stty -F {tty} {active_baud} 2>/dev/null; "
        f'printf \"TAB5_BAUD_PREFLIGHT_%s_%s\\\\n\" {target_baud} \"$rc\"'
        "'"
    )
    response = send_shell_and_wait(
        ser, command, success_marker, timeout=4.0
    )
    if success_marker not in response:
        raise RuntimeError(
            f"Linux rejected baud {target_baud}; preflight response: "
            f"{response[-300:]!r}"
        )


def coordinate_switch(
    ser: serial.Serial,
    tty: str,
    active_baud: int,
    target_baud: int,
    persist: bool,
    use_default: bool,
) -> None:
    preflight_linux_baud(ser, tty, active_baud, target_baud)

    armed_marker = f"TAB5_BAUD_ARMED_{target_baud}"
    command = (
        f'printf "TAB5_BAUD_ARMED_%s\\n" {target_baud}; '
        f"sleep 3; stty -F {tty} {target_baud}; exec bash -l"
    )
    send_shell_and_wait(ser, command, armed_marker, timeout=3.0)

    value = "default" if use_default else str(target_baud)
    management = (
        f"login-uart={value},persist={1 if persist else 0},delay=2500"
    )
    response = send_management(ser, management)
    if "TAB5CFG OK scheduled=" not in response:
        raise RuntimeError(
            "Tab5 rejected the scheduled baud change. "
            f"Captured: {response[-300:]!r}"
        )

    time.sleep(3.3)
    verify_marker = f"TAB5_BAUD_VERIFY_{target_baud}"
    verify_command = f'printf "TAB5_BAUD_VERIFY_%s\\n" {target_baud}'
    send_shell_and_wait(ser, verify_command, verify_marker, timeout=3.0)


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Query or change the Tab5 login UART without rebuilding firmware. "
            "A normal set coordinates the external Linux TTY and Tab5."
        )
    )
    parser.add_argument("--port", required=True, help="USB CDC port, e.g. COM3")
    parser.add_argument("--baud", type=int, choices=SUPPORTED_BAUDS)
    parser.add_argument(
        "--default",
        action="store_true",
        help="Switch to 115200 and clear the saved Tab5 baud",
    )
    parser.add_argument(
        "--persist",
        action="store_true",
        help="Save the selected baud in Tab5 NVS",
    )
    parser.add_argument(
        "--tab5-only",
        action="store_true",
        help="Change only Tab5; intended for mismatch recovery",
    )
    parser.add_argument("--linux-tty", default="/dev/ttyS1")
    parser.add_argument("--open-delay", type=float, default=1.5)
    args = parser.parse_args()

    if args.default and args.baud is not None:
        parser.error("--default and --baud cannot be used together")
    if not TTY_RE.fullmatch(args.linux_tty):
        parser.error("--linux-tty must be a simple /dev/... path")

    target_baud = 115200 if args.default else args.baud
    with open_serial(args.port, args.open_delay) as ser:
        active, persisted, pending = query_state(ser)
        print(
            f"Tab5 login UART: active={active} persisted={persisted} "
            f"pending={pending}"
        )

        if target_baud is None:
            return 0
        if pending != "none":
            raise RuntimeError("Tab5 already has a pending baud change")

        if target_baud == active:
            command_value = "default" if args.default else str(target_baud)
            management = (
                f"login-uart={command_value},"
                f"persist={1 if args.persist else 0},delay=0"
            )
            response = send_management(ser, management)
            if "TAB5CFG OK scheduled=" not in response:
                raise RuntimeError(
                    f"Tab5 rejected the update: {response[-300:]!r}"
                )
            time.sleep(0.1)
        elif args.tab5_only:
            command_value = "default" if args.default else str(target_baud)
            management = (
                f"login-uart={command_value},"
                f"persist={1 if args.persist else 0},delay=0"
            )
            response = send_management(ser, management)
            if "TAB5CFG OK scheduled=" not in response:
                raise RuntimeError(
                    f"Tab5 rejected the update: {response[-300:]!r}"
                )
            time.sleep(0.1)
        else:
            coordinate_switch(
                ser,
                args.linux_tty,
                active,
                target_baud,
                args.persist,
                args.default,
            )

        active, persisted, pending = query_state(ser)
        print(
            f"Baud change verified: active={active} persisted={persisted} "
            f"pending={pending}"
        )
        if active != target_baud:
            raise RuntimeError(
                f"Tab5 reports {active}, expected {target_baud}"
            )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, serial.SerialException) as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
