#!/usr/bin/env python3
"""Run a repeatable login-UART output burst and time the Tab5 render pipeline."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import time
import uuid

import serial


PIPELINE_RE = re.compile(rb"(?:^|[\r\n])TAB5PIPE v=1 (?P<body>[^\r\n]*)(?:\r\n|\r|\n)")
MIRROR_RE = re.compile(rb"(?:^|[\r\n])TAB5PIPE MIRROR enabled=(?P<enabled>[01])(?:\r\n|\r|\n)")
PIPELINE_RESET_MARKER = b"TAB5PIPE OK reset"


def write_bytes(
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


def read_until_marker(
    ser: serial.Serial,
    marker: bytes,
    timeout: float,
) -> tuple[bytes, bool, float | None]:
    deadline = time.monotonic() + timeout
    response = bytearray()
    marker_seen_at: float | None = None
    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            response.extend(ser.read(waiting))
            if marker_seen_at is None and marker in response:
                marker_seen_at = time.monotonic()
                return (bytes(response), True, marker_seen_at)
        else:
            time.sleep(0.01)
    return (bytes(response), False, marker_seen_at)


def management_frame(command: str) -> bytes:
    return b"\x1b]777;" + command.encode("ascii") + b"\x07"


def has_complete_line(payload: bytes, token: bytes) -> bool:
    start = payload.find(token)
    if start < 0:
        return False
    cr = payload.find(b"\r", start)
    lf = payload.find(b"\n", start)
    endings = [index for index in (cr, lf) if index >= 0]
    return bool(endings) and min(endings) > start


def has_expected_management_reply(payload: bytes, command: str) -> bool:
    if has_complete_line(payload, b"TAB5CFG ERR") or has_complete_line(payload, b"TAB5PIPE ERR"):
        return True
    if command == "render-pipeline?":
        return PIPELINE_RE.search(payload) is not None
    if command == "render-pipeline-reset":
        return PIPELINE_RESET_MARKER in payload
    if command == "render-mirror?" or command.startswith("render-mirror="):
        return MIRROR_RE.search(payload) is not None
    return has_complete_line(payload, b"TAB5PIPE")


def extract_management_reply(payload: bytes, command: str) -> bytes:
    def clean_match(match: re.Match[bytes]) -> bytes:
        return match.group(0).lstrip(b"\r\n")

    if command == "render-pipeline?":
        matches = list(PIPELINE_RE.finditer(payload))
        return clean_match(matches[-1]) if matches else payload
    if command == "render-mirror?" or command.startswith("render-mirror="):
        matches = list(MIRROR_RE.finditer(payload))
        return clean_match(matches[-1]) if matches else payload
    if command == "render-pipeline-reset":
        marker_index = payload.rfind(PIPELINE_RESET_MARKER)
        if marker_index >= 0:
            line_end = len(payload)
            for terminator in (b"\r", b"\n"):
                index = payload.find(terminator, marker_index)
                if index >= 0:
                    line_end = min(line_end, index + 1)
            return payload[marker_index:line_end]
    for token in (b"TAB5CFG ERR", b"TAB5PIPE ERR"):
        marker_index = payload.rfind(token)
        if marker_index >= 0:
            line_end = len(payload)
            for terminator in (b"\r", b"\n"):
                index = payload.find(terminator, marker_index)
                if index >= 0:
                    line_end = min(line_end, index + 1)
            return payload[marker_index:line_end]
    return payload


def parse_pipeline_stats(payload: bytes) -> dict[str, int] | None:
    matches = list(PIPELINE_RE.finditer(payload))
    if not matches:
        return None
    match = matches[-1]
    stats: dict[str, int] = {}
    for key, value in re.findall(rb"([A-Za-z0-9_]+)=([0-9]+)", match.group("body")):
        stats[key.decode("ascii")] = int(value)
    return stats


def read_management_reply(
    ser: serial.Serial,
    command: str,
    timeout: float,
) -> bytes:
    ser.write(management_frame(command))
    ser.flush()
    deadline = time.monotonic() + timeout
    response = bytearray()
    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            response.extend(ser.read(waiting))
            if has_expected_management_reply(response, command):
                break
        else:
            time.sleep(0.01)
    return extract_management_reply(bytes(response), command)


def query_pipeline_stats(ser: serial.Serial, timeout: float = 2.0) -> tuple[dict[str, int] | None, bytes]:
    reply = read_management_reply(ser, "render-pipeline?", timeout)
    return parse_pipeline_stats(reply), reply


def reset_pipeline_stats(ser: serial.Serial, timeout: float = 2.0) -> tuple[bool, bytes]:
    reply = read_management_reply(ser, "render-pipeline-reset", timeout)
    return PIPELINE_RESET_MARKER in reply, reply


def set_render_mirror_enabled(
    ser: serial.Serial,
    enabled: bool,
    timeout: float = 2.0,
) -> tuple[bool, bytes]:
    reply = read_management_reply(ser, f"render-mirror={1 if enabled else 0}", timeout)
    match = MIRROR_RE.search(reply)
    return match is not None and match.group("enabled") == (b"1" if enabled else b"0"), reply


def expected_rendered_minimum_bytes(nonce: str, lines: int, line_width: int) -> int:
    total = expected_mirrored_burst_bytes(nonce, lines, line_width)
    total += len(
        f"__TAB5_RENDER_END_{nonce}__:lines={lines}:line_width={line_width}\r\r\n".encode(
            "ascii"
        )
    )
    return total


def wait_for_pipeline_render(
    ser: serial.Serial,
    minimum_rendered_bytes: int,
    timeout: float,
    settle_seconds: float = 0.3,
) -> tuple[bool, dict[str, int] | None, bytes, int, float | None, float | None]:
    deadline = time.monotonic() + timeout
    poll_count = 0
    last_reply = b""
    last_stats: dict[str, int] | None = None
    reached_at: float | None = None
    quiescent_at: float | None = None
    last_rendered = -1
    last_depth = -1
    last_progress_at = time.monotonic()

    while time.monotonic() < deadline:
        stats, reply = query_pipeline_stats(ser, timeout=1.0)
        poll_count += 1
        now = time.monotonic()
        if reply:
            last_reply = reply
        if stats is not None:
            last_stats = stats
            rendered = stats.get("rendered", 0)
            depth = stats.get("sw_depth", 0)
            if rendered != last_rendered or depth != last_depth:
                last_rendered = rendered
                last_depth = depth
                last_progress_at = now
            if rendered >= minimum_rendered_bytes and depth == 0:
                if reached_at is None:
                    reached_at = now
                if now - last_progress_at >= settle_seconds:
                    quiescent_at = now
                    return True, stats, last_reply, poll_count, reached_at, quiescent_at
        time.sleep(0.1)

    return False, last_stats, last_reply, poll_count, reached_at, quiescent_at


def delta_pipeline_stats(
    before: dict[str, int] | None,
    after: dict[str, int] | None,
) -> dict[str, int] | None:
    if before is None or after is None:
        return None
    return {
        key: after.get(key, 0) - before.get(key, 0)
        for key in sorted(set(before) | set(after))
    }


PIPELINE_ERROR_KEYS = (
    "sw_dropped",
    "uart_break",
    "uart_buffer_full",
    "uart_fifo_ovf",
    "uart_frame",
    "uart_parity",
    "uart_other",
)


def pipeline_error_counts(delta: dict[str, int] | None) -> dict[str, int]:
    if delta is None:
        return {}
    return {
        key: value
        for key in PIPELINE_ERROR_KEYS
        if (value := delta.get(key, 0)) > 0
    }


def repeated_body(width: int) -> str:
    alphabet = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+-"
    repeats = (width // len(alphabet)) + 2
    return (alphabet * repeats)[:width]


def expected_mirrored_burst_bytes(nonce: str, lines: int, line_width: int) -> int:
    body = repeated_body(line_width)
    total = len(f"__TAB5_RENDER_START_{nonce}__\r\r\n".encode("ascii"))
    total += len(b"\x1b[2J\x1b[H")
    for line in range(1, lines + 1):
        color = 16 + (line % 216)
        total += len(
            f"\x1b[38;5;{color}m{line:04d} {body}\x1b[0m\r\r\n".encode("ascii")
        )
    total += len(b"\r\r\n")
    return total


def expected_line_bytes(line: int, line_width: int) -> bytes:
    body = repeated_body(line_width)
    color = 16 + (line % 216)
    return f"\x1b[38;5;{color}m{line:04d} {body}\x1b[0m\r\r\n".encode("ascii")


def build_workload(
    nonce: str,
    lines: int,
    line_width: int,
) -> tuple[bytes, bytes, bytes]:
    start_marker = f"__TAB5_RENDER_START_{nonce}__"
    end_marker = f"__TAB5_RENDER_END_{nonce}__"
    body = repeated_body(line_width)
    # The marker is assembled by the shell so interactive input echo cannot
    # contain the full marker and fake a successful measurement.
    command = "; ".join(
        [
            "A='__TAB5_RENDER'",
            "B='_START_'",
            "E='_END_'",
            f"N='{nonce}'",
            "Z='__'",
            "ESC=$(printf '\\033')",
            'printf "\\r\\n%s%s%s%s\\r\\n" "$A" "$B" "$N" "$Z"',
            'printf "${ESC}[2J${ESC}[H"',
            "i=1",
            (
                f'while [ "$i" -le {lines} ]; do '
                "color=$((16 + (i % 216))); "
                f'printf "%s%04d {body}%s\\r\\n" '
                '"${ESC}[38;5;${color}m" "$i" "${ESC}[0m"; '
                "i=$((i + 1)); "
                "done"
            ),
            (
                f'printf "\\r\\n%s%s%s%s:lines={lines}:line_width={line_width}\\r\\n" '
                '"$A" "$E" "$N" "$Z"'
            ),
            "stty echo clocal 2>/dev/null",
            "",
        ]
    )
    return (
        (command + "\r").encode("ascii"),
        start_marker.encode("ascii"),
        end_marker.encode("ascii"),
    )


def write_optional_bytes(path_text: str, payload: bytes) -> None:
    if not path_text:
        return
    path = Path(path_text)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)


def write_optional_json(path_text: str, data: dict[str, object]) -> None:
    if not path_text:
        return
    path = Path(path_text)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def analyze_burst_lines(
    response: bytes,
    start_marker: bytes,
    end_marker: bytes,
    expected_lines: int,
    line_width: int,
) -> dict[str, object]:
    start_index = response.find(start_marker)
    end_index = response.find(end_marker)
    marker_bounded = start_index >= 0 and end_index >= 0 and end_index > start_index
    burst = response[start_index:end_index] if marker_bounded else response
    observed = [
        int(match.group(1))
        for match in re.finditer(rb"\x1b\[38;5;[0-9]{1,3}m([0-9]{4}) ", burst)
    ]
    unique = set(observed)
    missing = [line for line in range(1, expected_lines + 1) if line not in unique]
    duplicate_count = len(observed) - len(unique)
    corrupt_or_missing_exact = []
    exact_duplicate_count = 0
    for line in range(1, expected_lines + 1):
        exact_count = burst.count(expected_line_bytes(line, line_width))
        if exact_count != 1:
            corrupt_or_missing_exact.append(line)
            if exact_count > 1:
                exact_duplicate_count += exact_count - 1
    if not marker_bounded:
        check = "timeout-or-marker-loss"
    elif not missing and duplicate_count == 0 and not corrupt_or_missing_exact:
        check = "ok"
    else:
        check = "loss-or-corruption"
    return {
        "line_sequence_check": check,
        "observed_line_count": len(observed),
        "unique_line_count": len(unique),
        "missing_line_count": len(missing),
        "missing_line_numbers_first": missing[:20],
        "duplicate_line_count": duplicate_count,
        "exact_line_match_count": expected_lines - len(corrupt_or_missing_exact),
        "exact_line_corrupt_or_missing_count": len(corrupt_or_missing_exact),
        "exact_line_corrupt_or_missing_numbers_first": corrupt_or_missing_exact[:20],
        "exact_line_duplicate_count": exact_duplicate_count,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--term", default="xterm-256color")
    parser.add_argument("--rows", type=int, default=32)
    parser.add_argument("--cols", type=int, default=64)
    parser.add_argument("--lines", type=int, default=240)
    parser.add_argument("--line-width", type=int, default=64)
    parser.add_argument("--timeout", type=float, default=45.0)
    parser.add_argument("--open-delay", type=float, default=2.0)
    parser.add_argument("--chunk-size", type=int, default=32)
    parser.add_argument("--chunk-delay", type=float, default=0.08)
    parser.add_argument(
        "--disable-mirror",
        action="store_true",
        help="Temporarily disable CDC login-UART mirroring and time firmware render counters.",
    )
    parser.add_argument("--raw-output", default="")
    parser.add_argument("--json-output", default="")
    args = parser.parse_args()

    if args.lines < 1:
        parser.error("--lines must be at least 1")
    if args.line_width < 1:
        parser.error("--line-width must be at least 1")
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    if args.chunk_size < 1:
        parser.error("--chunk-size must be at least 1")
    if args.chunk_delay < 0:
        parser.error("--chunk-delay must be non-negative")

    nonce = uuid.uuid4().hex[:8].upper()
    ready_marker = f"__TAB5_RENDER_READY_{nonce}__".encode("ascii")
    setup = (
        "\x03\x15"
        f"stty -echo clocal rows {args.rows} cols {args.cols} -ixon -ixoff 2>/dev/null; "
        f"export TERM={args.term}; "
        f"printf '\\r\\n{ready_marker.decode('ascii')}\\r\\n'\r"
    ).encode("ascii")
    payload, start_marker, end_marker = build_workload(
        nonce,
        args.lines,
        args.line_width,
    )

    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.05
    ser.rtscts = False
    ser.dsrdtr = False
    ser.dtr = False
    ser.rts = False

    setup_response = b""
    response = b""
    pipeline_reset_response = b""
    pipeline_before_response = b""
    pipeline_after_response = b""
    pipeline_before: dict[str, int] | None = None
    pipeline_after: dict[str, int] | None = None
    pipeline_supported = False
    start_seen_at: float | None = None
    end_seen_at: float | None = None
    send_done_at: float | None = None
    pipeline_reached_at: float | None = None
    pipeline_quiescent_at: float | None = None
    pipeline_poll_count = 0
    expected_rendered_minimum = expected_rendered_minimum_bytes(
        nonce,
        args.lines,
        args.line_width,
    )
    mirror_set_response = b""
    mirror_restore_response = b""
    mirror_disabled = False
    status = "timeout"

    with ser:
        try:
            time.sleep(args.open_delay)
            ser.reset_input_buffer()
            write_bytes(ser, setup, args.chunk_size, args.chunk_delay)
            setup_response, ready_seen, _ = read_until_marker(ser, ready_marker, 8.0)
            if not ready_seen:
                status = "setup-timeout"
            else:
                time.sleep(0.1)
                ser.reset_input_buffer()
                pipeline_supported, pipeline_reset_response = reset_pipeline_stats(ser)
                if args.disable_mirror:
                    mirror_configured, mirror_set_response = set_render_mirror_enabled(ser, False)
                    mirror_disabled = mirror_configured
                    if not mirror_configured:
                        status = "mirror-config-failed"
                if pipeline_supported:
                    pipeline_before, pipeline_before_response = query_pipeline_stats(ser)
                if status not in ("setup-timeout", "mirror-config-failed"):
                    ser.reset_input_buffer()
                    write_bytes(ser, payload, args.chunk_size, args.chunk_delay)
                    send_done_at = time.monotonic()
                    if args.disable_mirror:
                        (
                            rendered,
                            pipeline_after,
                            pipeline_after_response,
                            pipeline_poll_count,
                            pipeline_reached_at,
                            pipeline_quiescent_at,
                        ) = wait_for_pipeline_render(
                            ser,
                            expected_rendered_minimum,
                            args.timeout,
                        )
                        status = "ok" if rendered else "pipeline-timeout"
                    else:
                        deadline = time.monotonic() + args.timeout
                        buffer = bytearray()
                        while time.monotonic() < deadline:
                            waiting = ser.in_waiting
                            if waiting:
                                buffer.extend(ser.read(waiting))
                                now = time.monotonic()
                                if start_seen_at is None and start_marker in buffer:
                                    start_seen_at = now
                                if end_marker in buffer:
                                    end_seen_at = now
                                    status = "ok"
                                    break
                            else:
                                time.sleep(0.01)
                        response = bytes(buffer)
                        if pipeline_supported:
                            time.sleep(0.2)
                            ser.reset_input_buffer()
                            pipeline_after, pipeline_after_response = query_pipeline_stats(ser)

            if status != "ok":
                ser.write(b"\x03stty echo sane clocal 2>/dev/null\r")
                ser.flush()
        finally:
            if mirror_disabled:
                mirror_restored, mirror_restore_response = set_render_mirror_enabled(ser, True)
                if not mirror_restored and status == "ok":
                    status = "mirror-restore-failed"

    start_index = response.find(start_marker)
    end_index = response.find(end_marker)
    burst_bytes = 0
    if start_index >= 0 and end_index >= 0 and end_index > start_index:
        burst_bytes = end_index - start_index
    if args.disable_mirror:
        line_analysis = {
            "line_sequence_check": "not-captured-mirror-disabled",
            "observed_line_count": 0,
            "unique_line_count": 0,
            "missing_line_count": None,
            "missing_line_numbers_first": [],
            "duplicate_line_count": None,
            "exact_line_match_count": None,
            "exact_line_corrupt_or_missing_count": None,
            "exact_line_corrupt_or_missing_numbers_first": [],
            "exact_line_duplicate_count": None,
        }
    else:
        line_analysis = analyze_burst_lines(
            response,
            start_marker,
            end_marker,
            args.lines,
            args.line_width,
        )
        if status == "ok" and line_analysis["line_sequence_check"] != "ok":
            status = "integrity-failed"
    pipeline_delta = delta_pipeline_stats(pipeline_before, pipeline_after)
    pipeline_errors = pipeline_error_counts(pipeline_delta)
    if status == "ok" and pipeline_errors:
        status = "pipeline-error"
    expected_burst_bytes = expected_mirrored_burst_bytes(nonce, args.lines, args.line_width)
    burst_byte_delta = None if args.disable_mirror else burst_bytes - expected_burst_bytes

    start_to_end = None
    if start_seen_at is not None and end_seen_at is not None:
        start_to_end = end_seen_at - start_seen_at

    send_to_end = None
    if send_done_at is not None and end_seen_at is not None:
        send_to_end = end_seen_at - send_done_at

    send_to_pipeline_minimum = None
    if send_done_at is not None and pipeline_reached_at is not None:
        send_to_pipeline_minimum = pipeline_reached_at - send_done_at

    send_to_pipeline_quiescent = None
    if send_done_at is not None and pipeline_quiescent_at is not None:
        send_to_pipeline_quiescent = pipeline_quiescent_at - send_done_at

    summary: dict[str, object] = {
        "status": status,
        "mirror_mode": "disabled" if args.disable_mirror else "enabled",
        "port": args.port,
        "serial_baud": args.baud,
        "term": args.term,
        "rows": args.rows,
        "cols": args.cols,
        "lines": args.lines,
        "line_width": args.line_width,
        "command_bytes": len(payload),
        "setup_captured_bytes": len(setup_response),
        "captured_bytes": len(response),
        "burst_bytes_between_markers": burst_bytes,
        "expected_mirrored_burst_bytes": expected_burst_bytes,
        "expected_rendered_minimum_bytes": expected_rendered_minimum,
        "burst_byte_delta_from_expected": burst_byte_delta,
        "seconds_start_marker_to_end_marker": start_to_end,
        "seconds_command_sent_to_end_marker": send_to_end,
        "seconds_command_sent_to_pipeline_minimum": send_to_pipeline_minimum,
        "seconds_command_sent_to_pipeline_quiescent": send_to_pipeline_quiescent,
        "approx_burst_bytes_per_second": (
            burst_bytes / start_to_end if start_to_end and start_to_end > 0 else None
        ),
        **line_analysis,
        "pipeline_stats_supported": pipeline_supported,
        "pipeline_poll_count": pipeline_poll_count,
        "pipeline_stats_before": pipeline_before,
        "pipeline_stats_after": pipeline_after,
        "pipeline_stats_delta": pipeline_delta,
        "pipeline_error_counts": pipeline_errors,
        "mirror_set_reply": (
            mirror_set_response.decode("ascii", errors="replace")
            if mirror_set_response
            else None
        ),
        "mirror_restore_reply": (
            mirror_restore_response.decode("ascii", errors="replace")
            if mirror_restore_response
            else None
        ),
        "pipeline_reset_reply": (
            pipeline_reset_response.decode("ascii", errors="replace")
            if pipeline_reset_response
            else None
        ),
        "pipeline_before_reply": (
            pipeline_before_response.decode("ascii", errors="replace")
            if pipeline_before_response
            else None
        ),
        "pipeline_after_reply": (
            pipeline_after_response.decode("ascii", errors="replace")
            if pipeline_after_response
            else None
        ),
        "measurement_boundary": (
            "End-to-end login UART receive, terminal render, and CDC mirror timing. "
            "This is not pure LCD draw time. TAB5PIPE counters, when present, "
            "come from the firmware pipeline and are separate from CDC mirror "
            "line parsing."
        ),
        "raw_output": args.raw_output or None,
        "json_output": args.json_output or None,
    }

    write_optional_bytes(args.raw_output, response)
    write_optional_json(args.json_output, summary)

    print(
        "render latency workload: "
        f"status={status} mirror={summary['mirror_mode']} lines={args.lines} width={args.line_width} "
        f"captured={len(response)} burst={burst_bytes}"
    )
    if args.disable_mirror:
        print(f"expected rendered minimum bytes: {expected_rendered_minimum}")
    else:
        print(
            "expected mirrored burst bytes: "
            f"{expected_burst_bytes} delta={burst_byte_delta}"
        )
    print(f"command bytes: {len(payload)}")
    if start_to_end is not None:
        print(f"start-marker to end-marker: {start_to_end:.3f}s")
    if send_to_end is not None:
        print(f"command-sent to end-marker: {send_to_end:.3f}s")
    if send_to_pipeline_minimum is not None:
        print(f"command-sent to pipeline minimum: {send_to_pipeline_minimum:.3f}s")
    if send_to_pipeline_quiescent is not None:
        print(f"command-sent to pipeline quiescent: {send_to_pipeline_quiescent:.3f}s")
    if summary["approx_burst_bytes_per_second"] is not None:
        print(
            "approx burst mirror throughput: "
            f"{summary['approx_burst_bytes_per_second']:.1f} bytes/s"
        )
    print(
        "line integrity: "
        f"{summary['line_sequence_check']} "
        f"labels={summary['unique_line_count']}/{args.lines} "
        f"exact={summary['exact_line_match_count']}/{args.lines} "
        f"missing={summary['missing_line_count']} "
        f"duplicates={summary['duplicate_line_count']}"
    )
    if pipeline_supported:
        print(f"pipeline before: {pipeline_before}")
        print(f"pipeline after: {pipeline_after}")
        print(f"pipeline delta: {pipeline_delta}")
        if pipeline_errors:
            print(f"pipeline errors: {pipeline_errors}")
        if pipeline_poll_count:
            print(f"pipeline polls: {pipeline_poll_count}")
    else:
        print("pipeline stats: unsupported by current firmware")
    if args.raw_output:
        print(f"raw output: {args.raw_output}")
    if args.json_output:
        print(f"json output: {args.json_output}")
    if response:
        print(repr(response[-500:]))

    return 0 if status == "ok" else 1


if __name__ == "__main__":
    raise SystemExit(main())
