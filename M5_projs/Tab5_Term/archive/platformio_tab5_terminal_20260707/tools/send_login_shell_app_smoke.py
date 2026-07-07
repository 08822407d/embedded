#!/usr/bin/env python3
"""Run real terminal app smoke tests through the Tab5 login-shell bridge."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import re
import time
import uuid

import serial


DEFAULT_APPS = ("clear", "reset", "tput", "less", "nano", "vim", "htop")

STAGE9_TUI_APPS = (
    "clear",
    "reset",
    "tput",
    "less",
    "vim",
    "htop",
    "top",
    "tmux",
    "screen",
    "dialog",
    "whiptail",
    "btop",
    "nano",
)


@dataclass(frozen=True)
class AppTest:
    name: str
    command: str
    exit_bytes: bytes = b""
    view_time: float = 0.8
    timeout: float = 8.0


APP_TESTS = {
    "clear": AppTest(
        name="clear",
        command="clear; printf 'clear drew through terminfo\\r\\n'",
        view_time=0.5,
        timeout=5.0,
    ),
    "reset": AppTest(
        name="reset",
        command="reset; printf 'reset returned\\r\\n'",
        view_time=0.5,
        timeout=8.0,
    ),
    "tput": AppTest(
        name="tput",
        command="clear; tput cup 5 10; printf 'tput cup/sgr smoke'; tput sgr0",
        view_time=1.0,
        timeout=5.0,
    ),
    "less": AppTest(
        name="less",
        command="less /etc/os-release",
        exit_bytes=b"q\r",
        view_time=3.0,
        timeout=8.0,
    ),
    "nano": AppTest(
        name="nano",
        command=(
            "printf 'Tab5 nano smoke\\nNo changes should be saved.\\n' "
            ">/tmp/tab5_nano_smoke.txt; nano /tmp/tab5_nano_smoke.txt"
        ),
        exit_bytes=b"\x18",
        view_time=2.0,
        timeout=8.0,
    ),
    "vim": AppTest(
        name="vim",
        command=(
            "printf 'Tab5 vim smoke\\nNo modelines.\\n' "
            ">/tmp/tab5_vim_smoke.txt; vim -Nu NONE -n /tmp/tab5_vim_smoke.txt"
        ),
        exit_bytes=b"\x1b:q!\r",
        view_time=2.0,
        timeout=8.0,
    ),
    "htop": AppTest(
        name="htop",
        command="htop",
        exit_bytes=b"q",
        view_time=2.5,
        timeout=8.0,
    ),
    "top": AppTest(
        name="top",
        command="top",
        exit_bytes=b"q",
        view_time=2.0,
        timeout=8.0,
    ),
    "tmux": AppTest(
        name="tmux",
        command=(
            "tmux -L tab5-smoke kill-server >/dev/null 2>&1; "
            "tmux -L tab5-smoke new-session -x {cols} -y {rows} "
            "'printf \"Tab5 tmux smoke\\n\"; sleep 2'"
        ),
        view_time=0.5,
        timeout=10.0,
    ),
    "screen": AppTest(
        name="screen",
        command=(
            "screen -wipe >/dev/null 2>&1; "
            "screen -q -S tab5-smoke sh -c "
            "'printf \"Tab5 screen smoke\\n\"; sleep 2'"
        ),
        view_time=0.5,
        timeout=10.0,
    ),
    "dialog": AppTest(
        name="dialog",
        command="dialog --no-shadow --title 'Tab5 dialog smoke' --msgbox 'Tab5 dialog smoke' 8 42",
        exit_bytes=b"\r",
        view_time=1.2,
        timeout=8.0,
    ),
    "whiptail": AppTest(
        name="whiptail",
        command="whiptail --title 'Tab5 whiptail smoke' --msgbox 'Tab5 whiptail smoke' 8 42",
        exit_bytes=b"\r",
        view_time=1.2,
        timeout=8.0,
    ),
    "btop": AppTest(
        name="btop",
        command="btop",
        exit_bytes=b"q",
        view_time=2.5,
        timeout=8.0,
    ),
}


def read_until(
    ser: serial.Serial,
    marker: bytes,
    timeout: float,
    marker_count: int = 1,
) -> bytes:
    deadline = time.monotonic() + timeout
    response = bytearray()
    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            response.extend(ser.read(waiting))
            if response.count(marker) >= marker_count:
                break
        else:
            time.sleep(0.05)
    return bytes(response)


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


def write_ascii(
    ser: serial.Serial,
    text: str,
    chunk_size: int,
    chunk_delay: float,
) -> None:
    write_bytes(ser, text.encode("ascii"), chunk_size, chunk_delay)


def marker_text(prefix: str, nonce: str, index: int) -> str:
    return f"__TAB5_{prefix}_{nonce}_{index:02d}__"


def marker_command(marker: str) -> str:
    return f"printf '\\r\\n{marker}\\r\\n'"


def send_command_wait(
    ser: serial.Serial,
    command: str,
    marker: str,
    timeout: float,
    chunk_size: int,
    chunk_delay: float,
    marker_count: int,
) -> bytes:
    write_ascii(ser, f"{command}; {marker_command(marker)}\r", chunk_size, chunk_delay)
    return read_until(ser, marker.encode("ascii"), timeout, marker_count)


def parse_availability(text: str) -> dict[str, bool]:
    available: dict[str, bool] = {}
    for match in re.finditer(r"APP_(HAVE|MISS):([a-z0-9_+-]+)", text):
        available[match.group(2)] = match.group(1) == "HAVE"
    return available


def parse_done_rc(text: str, app_name: str) -> int | None:
    match = re.search(rf"APP_DONE:{re.escape(app_name)}:rc=([0-9]+)", text)
    if not match:
        return None
    return int(match.group(1))


def printf_escape(text: str) -> str:
    escaped = (
        text.replace("\\", "\\\\")
        .replace("\x1b", "\\033")
        .replace("\r", "\\r")
        .replace("\n", "\\n")
        .replace("'", "'\\''")
    )
    return f"printf '{escaped}'"


def result_line(result: tuple[str, str, int | None]) -> str:
    app, status, rc = result
    if status == "ok":
        return f"{app}: ok rc={rc}"
    if status == "skip":
        return f"{app}: skipped missing"
    if status == "timeout":
        return f"{app}: timeout"
    if rc is not None:
        return f"{app}: {status} rc={rc}"
    return f"{app}: {status}"


def run_app_test(
    ser: serial.Serial,
    app: AppTest,
    nonce: str,
    index: int,
    rows: int,
    cols: int,
    chunk_size: int,
    chunk_delay: float,
) -> tuple[str, str, int | None, bytes]:
    marker = marker_text("APP", nonce, index)
    app_command = app.command.format(rows=rows, cols=cols)
    command = (
        f"stty clocal 2>/dev/null; {app_command}; rc=$?; "
        "stty echo clocal 2>/dev/null; "
        f"printf '\\r\\nAPP_DONE:{app.name}:rc=%s\\r\\n' \"$rc\""
    )
    write_ascii(ser, f"{command}; {marker_command(marker)}\r", chunk_size, chunk_delay)
    time.sleep(app.view_time)
    if app.exit_bytes:
        ser.write(app.exit_bytes)
        ser.flush()

    response = read_until(ser, marker.encode("ascii"), app.timeout, marker_count=2)
    text = response.decode("utf-8", "ignore")
    if marker.encode("ascii") not in response:
        ser.write(b"\x03\x1b:q!\rstty echo clocal 2>/dev/null\r")
        ser.flush()
        return (app.name, "timeout", None, response)

    rc = parse_done_rc(text, app.name)
    if rc == 0:
        return (app.name, "ok", rc, response)
    return (app.name, "failed", rc, response)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--term", default="xterm-256color")
    parser.add_argument("--rows", type=int, default=32)
    parser.add_argument("--cols", type=int, default=69)
    parser.add_argument("--apps", default=",".join(DEFAULT_APPS))
    parser.add_argument(
        "--profile",
        choices=("stage4", "stage9"),
        default="stage4",
        help="Preset app list to use when --apps is not supplied",
    )
    parser.add_argument("--open-delay", type=float, default=2.0)
    parser.add_argument("--startup-timeout", type=float, default=6.0)
    parser.add_argument("--chunk-size", type=int, default=64)
    parser.add_argument("--chunk-delay", type=float, default=0.04)
    args = parser.parse_args()

    if args.chunk_size < 1:
        parser.error("--chunk-size must be at least 1")
    if args.chunk_delay < 0:
        parser.error("--chunk-delay must be non-negative")

    apps_argument = args.apps
    if apps_argument == ",".join(DEFAULT_APPS) and args.profile == "stage9":
        apps_argument = ",".join(STAGE9_TUI_APPS)

    app_names = [name.strip() for name in apps_argument.split(",") if name.strip()]
    unknown = [name for name in app_names if name not in APP_TESTS]
    if unknown:
        parser.error(f"unknown app tests: {', '.join(unknown)}")

    nonce = uuid.uuid4().hex[:8].upper()
    results: list[tuple[str, str, int | None]] = []
    captured = bytearray()

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
        time.sleep(0.3)

        setup = (
            f"export TERM={args.term}; "
            f"stty echo clocal rows {args.rows} cols {args.cols} 2>/dev/null; "
            "printf '\\r\\nTAB5_STAGE4_APP_SMOKE_START\\r\\n'"
        )
        write_ascii(ser, f"{setup}\r", args.chunk_size, args.chunk_delay)
        time.sleep(1.0)
        waiting = ser.in_waiting
        if waiting:
            captured.extend(ser.read(waiting))

        availability_marker = marker_text("AVAIL", nonce, 1)
        commands = " ".join(app_names)
        availability_command = (
            "for c in "
            + commands
            + "; do "
            + "if command -v \"$c\" >/dev/null 2>&1; then "
            + "printf 'APP_HAVE:%s\\r\\n' \"$c\"; "
            + "else printf 'APP_MISS:%s\\r\\n' \"$c\"; fi; done"
        )
        availability_response = send_command_wait(
            ser,
            availability_command,
            availability_marker,
            args.startup_timeout,
            args.chunk_size,
            args.chunk_delay,
            2,
        )
        captured.extend(availability_response)
        available = parse_availability(availability_response.decode("utf-8", "ignore"))

        for index, name in enumerate(app_names, start=2):
            if not available.get(name, False):
                results.append((name, "skip", None))
                continue
            app_result = run_app_test(
                ser,
                APP_TESTS[name],
                nonce,
                index,
                args.rows,
                args.cols,
                args.chunk_size,
                args.chunk_delay,
            )
            results.append(app_result[:3])
            captured.extend(app_result[3])

        is_stage9 = args.profile == "stage9"
        summary_title = "Stage9 TUI matrix result" if is_stage9 else "Stage4 real app smoke result"
        summary_tail = "End of Stage9 TUI matrix" if is_stage9 else "End of Stage4 real app smoke"
        summary = f"\x1b[2J\x1b[H{summary_title}\r\n"
        summary += f"TERM={args.term} rows={args.rows} cols={args.cols}\r\n\r\n"
        for result in results:
            summary += result_line(result) + "\r\n"
        summary += f"\r\n{summary_tail}\r\n"

        summary_marker = marker_text("SUMMARY", nonce, 99)
        summary_command = f"{printf_escape(summary)}; stty echo 2>/dev/null"
        captured.extend(
            send_command_wait(
                ser,
                summary_command,
                summary_marker,
                5.0,
                args.chunk_size,
                args.chunk_delay,
                2,
            )
        )

    run_label = "Stage9 TUI matrix" if args.profile == "stage9" else "Stage4 app smoke"
    print(f"ran {run_label} on {args.port}")
    for result in results:
        print(result_line(result))
    print(f"captured {len(captured)} bytes from login shell")
    if captured:
        print(repr(bytes(captured[-1200:])))

    return 1 if any(status in {"failed", "timeout"} for _, status, _ in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
