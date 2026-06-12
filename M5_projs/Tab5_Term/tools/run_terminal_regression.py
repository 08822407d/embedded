#!/usr/bin/env python3
"""Replay terminal corpora and assert diagnostic screen-state responses."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
import time

import serial

from send_terminal_test import TESTS


FRAME_PREFIX = b"\x1b]777;"
FRAME_SUFFIX = b"\x07"
STATE_RE = re.compile(rb"TAB5SNAP v=1 (?P<body>[^\r\n]*)")
ROW_RE = re.compile(
    rb"TAB5ROW row=(?P<row>\d+) hash=(?P<hash>[0-9A-F]{8}) text=(?P<text>[0-9A-F]*)"
)
CELL_RE = re.compile(
    rb"TAB5CELL row=(?P<row>\d+) col=(?P<col>\d+) "
    rb"text=(?P<text>[0-9A-F]*) width=(?P<width>[012]) "
    rb"fg=(?P<fg>\d+) bg=(?P<bg>\d+) "
    rb"attrs=(?P<attrs>\d+) dec=(?P<dec>[01])"
)


def private_command(command: str) -> bytes:
    return FRAME_PREFIX + command.encode("ascii") + FRAME_SUFFIX


def parse_fields(body: bytes) -> dict[str, str]:
    fields: dict[str, str] = {}
    for item in body.decode("ascii").split():
        key, value = item.split("=", 1)
        fields[key] = value
    return fields


def collect_responses(raw: bytes) -> tuple[dict[str, str], dict[int, str], dict[tuple[int, int], dict[str, int | str]]]:
    state_match = STATE_RE.search(raw)
    if state_match is None:
        raise ValueError(f"missing TAB5SNAP response in {raw!r}")
    state = parse_fields(state_match.group("body"))

    rows: dict[int, str] = {}
    for match in ROW_RE.finditer(raw):
        row = int(match.group("row"))
        rows[row] = bytes.fromhex(match.group("text").decode("ascii")).decode("utf-8")

    cells: dict[tuple[int, int], dict[str, int | str]] = {}
    for match in CELL_RE.finditer(raw):
        key = (int(match.group("row")), int(match.group("col")))
        cells[key] = {
            "text": bytes.fromhex(match.group("text").decode("ascii")).decode("utf-8"),
            "width": int(match.group("width")),
            "fg": int(match.group("fg")),
            "bg": int(match.group("bg")),
            "attrs": int(match.group("attrs")),
            "dec": int(match.group("dec")),
        }
    return state, rows, cells


def send_chunks(ser: serial.Serial, payload: bytes, chunk_size: int, chunk_delay: float) -> None:
    for start in range(0, len(payload), chunk_size):
        ser.write(payload[start : start + chunk_size])
        ser.flush()
        if chunk_delay:
            time.sleep(chunk_delay)


def read_response_line(
    ser: serial.Serial,
    timeout: float,
    marker: bytes,
) -> bytes:
    response = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            response.extend(ser.read(waiting))
            marker_index = response.find(marker)
            if marker_index >= 0 and b"\n" in response[marker_index:]:
                break
        else:
            time.sleep(0.01)
    return bytes(response)


def query_line(
    ser: serial.Serial,
    request: bytes,
    marker: bytes,
    chunk_size: int,
    chunk_delay: float,
    timeout: float,
) -> bytes:
    send_chunks(ser, request, chunk_size, chunk_delay)
    return read_response_line(ser, timeout, marker)


def assert_case(
    case_name: str,
    expected: dict,
    state: dict[str, str],
    rows: dict[int, str],
    cells: dict[tuple[int, int], dict[str, int | str]],
) -> list[str]:
    failures: list[str] = []
    for key, value in expected.get("state", {}).items():
        actual = state.get(key)
        if actual != str(value):
            failures.append(f"state {key}: expected {value!r}, got {actual!r}")

    for row_text, value in expected.get("rows", {}).items():
        row = int(row_text)
        actual = rows.get(row)
        if actual != value:
            failures.append(f"row {row}: expected {value!r}, got {actual!r}")

    for expected_cell in expected.get("cells", []):
        key = (expected_cell["row"], expected_cell["col"])
        actual_cell = cells.get(key)
        if actual_cell is None:
            failures.append(f"cell {key}: missing response")
            continue
        for field, value in expected_cell.items():
            if field in {"row", "col"}:
                continue
            actual = actual_cell.get(field)
            if actual != value:
                failures.append(
                    f"cell {key} {field}: expected {value!r}, got {actual!r}"
                )
    return failures


def run_case(
    ser: serial.Serial,
    case_name: str,
    expected: dict,
    chunk_size: int,
    chunk_delay: float,
    response_timeout: float,
) -> tuple[list[str], bytes]:
    ser.reset_input_buffer()
    send_chunks(ser, TESTS[case_name], chunk_size, chunk_delay)
    time.sleep(0.2)

    raw = bytearray()
    raw.extend(
        query_line(
            ser,
            private_command("terminal-state?"),
            b"TAB5SNAP ",
            chunk_size,
            chunk_delay,
            response_timeout,
        )
    )
    for row in sorted(int(value) for value in expected.get("rows", {})):
        raw.extend(
            query_line(
                ser,
                private_command(f"terminal-row={row}?"),
                f"TAB5ROW row={row} ".encode("ascii"),
                chunk_size,
                chunk_delay,
                response_timeout,
            )
        )
    for cell in expected.get("cells", []):
        row = cell["row"]
        column = cell["col"]
        raw.extend(
            query_line(
                ser,
                private_command(f"terminal-cell={row},{column}?"),
                f"TAB5CELL row={row} col={column} ".encode("ascii"),
                chunk_size,
                chunk_delay,
                response_timeout,
            )
        )
    try:
        state, rows, cells = collect_responses(bytes(raw))
    except (UnicodeDecodeError, ValueError) as error:
        return [str(error)], bytes(raw)
    return assert_case(case_name, expected, state, rows, cells), bytes(raw)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument(
        "--manifest",
        type=Path,
        default=root / "tests" / "terminal_regression_manifest.json",
    )
    parser.add_argument("--case", action="append", dest="cases")
    parser.add_argument("--open-delay", type=float, default=3.0)
    parser.add_argument("--chunk-size", type=int, default=32)
    parser.add_argument("--chunk-delay", type=float, default=0.03)
    parser.add_argument("--response-timeout", type=float, default=3.0)
    parser.add_argument("--raw-log", type=Path)
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    available = manifest["cases"]
    selected = args.cases or list(available)
    unknown = [name for name in selected if name not in available or name not in TESTS]
    if unknown:
        parser.error(f"unknown regression case(s): {', '.join(unknown)}")
    if args.chunk_size < 1:
        parser.error("--chunk-size must be at least 1")

    raw_log = bytearray()
    failed = 0
    ser = serial.Serial(
        port=args.port,
        baudrate=args.baud,
        timeout=0,
        rtscts=False,
        dsrdtr=False,
    )
    ser.dtr = False
    ser.rts = False
    with ser:
        time.sleep(args.open_delay)
        for case_name in selected:
            failures, raw = run_case(
                ser,
                case_name,
                available[case_name],
                args.chunk_size,
                args.chunk_delay,
                args.response_timeout,
            )
            raw_log.extend(f"\n===== {case_name} =====\n".encode("ascii"))
            raw_log.extend(raw)
            if failures:
                failed += 1
                print(f"FAIL {case_name}")
                for failure in failures:
                    print(f"  {failure}")
            else:
                print(f"PASS {case_name}")

    if args.raw_log is not None:
        args.raw_log.parent.mkdir(parents=True, exist_ok=True)
        args.raw_log.write_bytes(raw_log)
        print(f"raw log: {args.raw_log}")

    print(f"terminal regression: {len(selected) - failed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
