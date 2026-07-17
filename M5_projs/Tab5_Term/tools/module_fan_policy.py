#!/usr/bin/env python3
"""Configure the Tab5 Module Fan automatic policy over USB Serial/JTAG."""

from __future__ import annotations

import argparse
import time

from module_fan_telemetry import open_serial, read_fan_line, send_command


POLICY_PREFIX = "TAB5FAN POLICY"
MUTATING_ACTIONS = {"set", "set-interval", "set-failsafe", "defaults", "load"}


def policy_command(args: argparse.Namespace) -> str:
    if args.action == "get":
        return "module-fan-policy?"
    if args.action == "set":
        return f"module-fan-policy=set,{args.curve}"
    if args.action == "set-interval":
        return f"module-fan-policy-interval=set,{args.interval}"
    if args.action == "set-failsafe":
        return f"module-fan-policy-failsafe=set,{args.failsafe}"
    return f"module-fan-policy={args.action}"


def send_policy_command(
    ser,
    command: str,
    timeout: float,
) -> str:
    send_command(ser, command)
    line = read_fan_line(ser, time.monotonic() + timeout, POLICY_PREFIX)
    print(line)
    return line


def read_samples(ser, count: int, settle: float, timeout: float) -> None:
    if count <= 0:
        return

    time.sleep(settle)
    send_command(ser, "module-fan-log=start")
    print(read_fan_line(ser, time.monotonic() + timeout, "TAB5FAN LOG enabled=1"))
    for _ in range(count):
        print(read_fan_line(ser, time.monotonic() + timeout, "TAB5FAN SAMPLE"))
    send_command(ser, "module-fan-log=stop")
    print(read_fan_line(ser, time.monotonic() + timeout, "TAB5FAN LOG enabled=0"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "action",
        choices=("get", "set", "set-interval", "set-failsafe", "save", "load", "defaults"),
    )
    parser.add_argument("--port", required=True)
    parser.add_argument("--curve")
    parser.add_argument("--interval", type=int)
    parser.add_argument("--failsafe", type=int)
    parser.add_argument("--save", action="store_true")
    parser.add_argument("--samples", type=int, default=0)
    parser.add_argument("--settle", type=float, default=0.5)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--open-delay", type=float, default=8.0)
    parser.add_argument("--timeout", type=float, default=8.0)
    args = parser.parse_args()

    if args.action == "set" and args.curve is None:
        parser.error("set requires --curve")
    if args.action == "set-interval" and args.interval is None:
        parser.error("set-interval requires --interval")
    if args.action == "set-failsafe" and args.failsafe is None:
        parser.error("set-failsafe requires --failsafe")
    if args.save and args.action not in MUTATING_ACTIONS:
        parser.error("--save requires a policy-changing action")

    with open_serial(args) as ser:
        line = send_policy_command(ser, policy_command(args), args.timeout)
        if " ERR " in line:
            return 1

        if args.save:
            line = send_policy_command(ser, "module-fan-policy=save", args.timeout)
            if " ERR " in line:
                return 1

        read_samples(ser, args.samples, args.settle, args.timeout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
