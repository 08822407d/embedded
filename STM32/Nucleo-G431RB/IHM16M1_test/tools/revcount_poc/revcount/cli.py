"""CLI with a hard separation between read-only and motor-control modes."""

from __future__ import annotations

import argparse
from dataclasses import asdict
from datetime import datetime
import json
from pathlib import Path
import sys

from .protocol import (
    ASPEPClient,
    MCPClient,
    REG_FAULTS_FLAGS,
    REG_HALL_EL_ANGLE,
    REG_SPEED_MEAS,
    REG_STATUS,
)
from .runner import get_telemetry, run_powered, run_read_only
from .tracker import required_sample_rate
from .transport_win32 import Win32Serial, resolve_port


DEFAULT_SN = "002A00403234510E33353533"


def _default_log(mode: str) -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return Path(__file__).resolve().parents[1] / "logs" / f"{stamp}_{mode}.csv"


def _add_connection(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--port", default="auto", help="COM port or auto (default: auto)")
    parser.add_argument("--sn", default=DEFAULT_SN, help="ST-LINK serial number for --port auto")


def _connect(args) -> tuple[Win32Serial, MCPClient, str]:
    port = resolve_port(args.port, args.sn)
    serial = Win32Serial(port, baudrate=1_843_200)
    aspep = ASPEPClient(serial)
    try:
        aspep.connect()
    except Exception:
        serial.close()
        raise
    return serial, MCPClient(aspep), port


def _print_summary(summary) -> None:
    print(json.dumps(asdict(summary), indent=2, ensure_ascii=False))


def command_probe(args) -> int:
    serial, client, port = _connect(args)
    try:
        angle, status, faults, speed = client.get_registers([
            (REG_HALL_EL_ANGLE, "h"),
            (REG_STATUS, "B"),
            (REG_FAULTS_FLAGS, "I"),
            (REG_SPEED_MEAS, "i"),
        ])
        print(json.dumps({
            "port": port,
            "sn": args.sn,
            "mode": "read-only GET only",
            "hall_el_angle_raw": angle,
            "status": status,
            "faults": f"0x{faults:08X}",
            "speed_rpm": speed,
        }, indent=2))
    finally:
        serial.close()
    return 0


def command_hand_turn(args) -> int:
    if args.turns <= 0 or args.duration <= 0:
        raise ValueError("turns and duration must be positive")
    if not 1 <= args.design_rpm <= 1000:
        raise ValueError("design-rpm must be in 1..1000")
    expected = args.turns * (-1 if args.direction == "negative" else 1)
    csv_path = args.csv or _default_log("hand_turn")
    serial, client, port = _connect(args)
    print(f"Connected read-only on {port}; sampling for {args.duration:g}s")
    print("No MCP SET, Ack, Start, Ramp, or Stop command is available in this path.")
    try:
        summary = run_read_only(
            client, csv_path, args.duration, expected_turns=expected,
            max_rpm=args.design_rpm,
        )
    finally:
        serial.close()
    _print_summary(summary)
    required = required_sample_rate(args.design_rpm)
    min_hz = summary.sampling.get("min_hz")
    if min_hz is None or min_hz < required:
        print(
            f"FAIL: worst-case sampling {min_hz} Hz is below required {required:.1f} Hz",
            file=sys.stderr,
        )
        return 2
    if not summary.valid or (summary.error_turns is not None
                             and abs(summary.error_turns) > 0.25):
        return 2
    return 0


def command_run(args) -> int:
    confirmations = (
        args.allow_motor_control,
        args.confirm_bus_powered,
        args.confirm_user_present,
    )
    if not all(confirmations):
        print(
            "Refusing motor control. Required together: --allow-motor-control "
            "--confirm-bus-powered --confirm-user-present",
            file=sys.stderr,
        )
        return 3
    csv_path = args.csv or _default_log("powered")
    serial, client, port = _connect(args)
    print(f"Authorized powered run on {port}; emergency action remains Stop + bus disconnect.")
    try:
        summary = run_powered(
            client, csv_path, turns=args.turns, rpm=args.rpm,
            lead=args.lead, stop_method=args.stop_method,
            acceleration_ms=args.acceleration_ms,
            stop_ramp_ms=args.stop_ramp_ms,
        )
    except KeyboardInterrupt:
        try:
            client.command(0x0021, timeout=0.5)
        except Exception:
            pass
        print("Interrupted; Stop was attempted. Disconnect motor bus now.", file=sys.stderr)
        return 130
    finally:
        serial.close()
    _print_summary(summary)
    return 0 if summary.valid else 2


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="GM16020-06 PC-side Hall revolution-counting PoC"
    )
    sub = parser.add_subparsers(dest="command", required=True)

    probe = sub.add_parser("probe", help="read STATUS/FAULT/speed once; GET only")
    _add_connection(probe)
    probe.set_defaults(func=command_probe)

    hand = sub.add_parser("hand-turn", help="USB-only timed hand-turn trial; GET only")
    _add_connection(hand)
    hand.add_argument("--turns", type=float, required=True,
                      help="number of manually aligned mechanical turns")
    hand.add_argument("--direction", choices=("positive", "negative"), required=True)
    hand.add_argument("--duration", type=float, default=20.0,
                      help="sampling window in seconds")
    hand.add_argument("--design-rpm", type=float, default=1000.0,
                      help="sampling-gap validity design speed (default: 1000)")
    hand.add_argument("--csv", type=Path, help="CSV output path")
    hand.set_defaults(func=command_hand_turn)

    run = sub.add_parser("run", help="powered N-turn trial; sends motor-control commands")
    _add_connection(run)
    run.add_argument("--turns", type=float, default=5.0)
    run.add_argument("--rpm", type=int, default=500)
    run.add_argument("--lead", type=float, default=0.0)
    run.add_argument("--stop-method", choices=("ramp", "direct"), default="ramp")
    run.add_argument("--acceleration-ms", type=int, default=1000)
    run.add_argument("--stop-ramp-ms", type=int, default=1500)
    run.add_argument("--csv", type=Path, help="CSV output path")
    run.add_argument("--allow-motor-control", action="store_true")
    run.add_argument("--confirm-bus-powered", action="store_true")
    run.add_argument("--confirm-user-present", action="store_true")
    run.set_defaults(func=command_run)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        return args.func(args)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
