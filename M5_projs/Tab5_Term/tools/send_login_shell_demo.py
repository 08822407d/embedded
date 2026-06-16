#!/usr/bin/env python3
"""Send visual terminal demos through the Tab5 USB-to-login-UART bridge."""

from __future__ import annotations

import argparse
import re
import time
import uuid

import serial


VT102_DEMO = r"""sh <<'TAB5VT'
ESC=$(printf '\033')
printf "${ESC}[2J${ESC}[H"
printf "LLM shell -> Tab5 VT102 demo\r\n"
printf "host: $(hostname 2>/dev/null)   kernel: $(uname -r 2>/dev/null)\r\n"
printf "\r\nColors: ${ESC}[31mred${ESC}[0m ${ESC}[32mgreen${ESC}[0m ${ESC}[38;5;226myellow256${ESC}[0m ${ESC}[7minverse${ESC}[0m\r\n"
printf "\r\nDEC graphics:\r\n"
printf "${ESC}(0lqqqqqqk${ESC}(B\r\n"
printf "${ESC}(0x      x${ESC}(B\r\n"
printf "${ESC}(0tqqnqqu${ESC}(B\r\n"
printf "${ESC}(0x      x${ESC}(B\r\n"
printf "${ESC}(0mqqqqqqj${ESC}(B\r\n"
printf "\r\nCursor/edit tests:\r\n"
printf "insert chars: ABCDE${ESC}[3D${ESC}[2@xy\r\n"
printf "delete chars: abcdef${ESC}[3D${ESC}[2P\r\n"
printf "erase chars:  abcdef${ESC}[3D${ESC}[2X\r\n"
printf "\r\nScroll region demo below:\r\n"
printf "${ESC}[17;21r${ESC}[17;1Hregion top${ESC}[21;1Hregion bottom"
for n in 1 2 3 4 5 6; do
  printf "${ESC}[21;1Hregion scroll %s\r\n" "$n"
  sleep 0.20
done
printf "${ESC}[r${ESC}[23;1HEnd of real shell VT102 demo\r\n"
TAB5VT
"""


SHELL_PROBE = r"""printf 'shell-path-ok: %s\r\n' "$(uname -n 2>/dev/null || hostname)"
"""

RECOVER_DEMO = """\x11\x03
stty sane -ixon -ixoff 2>/dev/null; printf 'recover-ok: %s\\r\\n' "$(uname -n 2>/dev/null || hostname)"
"""

SGR_DEMO = r"""sh <<'TAB5SGR'
ESC=$(printf '\033')
printf "${ESC}[2J${ESC}[H"
printf "LLM shell -> Stage 3 SGR check\r\n"
printf "Attrs: normal ${ESC}[1mbold${ESC}[22m ${ESC}[2mdim${ESC}[22m ${ESC}[4munderline${ESC}[24m ${ESC}[7minverse${ESC}[27m hidden:[${ESC}[8mSECRET${ESC}[28m]\r\n"
printf "Color: ${ESC}[31mred${ESC}[0m ${ESC}[92mbright-green${ESC}[0m ${ESC}[38;5;226myellow256${ESC}[0m ${ESC}[38;2;255;128;0mtrue-orange${ESC}[0m\r\n"
printf "BCE: ${ESC}[44mblue-to-eol${ESC}[K${ESC}[0m\r\n"
printf "End of real shell SGR check\r\n"
TAB5SGR
"""

SCROLL_DEMO = r"""sh <<'TAB5SCROLL'
ESC=$(printf '\033')
printf "${ESC}[r${ESC}[2J${ESC}[H"
i=1
while [ "$i" -lt 50 ]; do
  printf 'SCROLL-%02d\r\n' "$i"
  sleep 0.02
  i=$((i + 1))
done
printf 'SCROLL-%02d' "$i"
TAB5SCROLL
"""


DEMOS = {
    "catv": " true; cat -v\n",
    "htop-usb": "htop",
    "less-usb": "less /etc/os-release",
    "probe": SHELL_PROBE,
    "recover": RECOVER_DEMO,
    "scroll": SCROLL_DEMO,
    "sgr": SGR_DEMO,
    "vt102": VT102_DEMO,
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--demo", choices=sorted(DEMOS), default="vt102")
    parser.add_argument(
        "--open-delay",
        type=float,
        default=2.0,
        help="Delay after opening the port; opening CDC can reset ESP32-P4",
    )
    parser.add_argument("--chunk-size", type=int, default=96)
    parser.add_argument("--chunk-delay", type=float, default=0.04)
    parser.add_argument("--capture-window", type=float, default=8.0)
    args = parser.parse_args()

    if args.chunk_size < 1:
        parser.error("--chunk-size must be at least 1")
    if args.chunk_delay < 0:
        parser.error("--chunk-delay must be non-negative")
    if args.capture_window < 0:
        parser.error("--capture-window must be non-negative")

    payload = DEMOS[args.demo].replace("\n", "\r\n").encode("utf-8")
    interactive_catv = args.demo == "catv"
    interactive_app = args.demo in ("htop-usb", "less-usb")
    response = bytearray()
    nonce = uuid.uuid4().hex[:8].upper()
    app_marker = f"__TAB5_{args.demo.upper().replace('-', '_')}_{nonce}__".encode("ascii")
    app_marker_re = re.escape(app_marker) + rb":rc=([0-9]+)"

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
        if interactive_catv:
            ser.write(b"\x03\x15")
            ser.flush()
            time.sleep(0.8)
            ser.reset_input_buffer()
        if interactive_app:
            ser.write(b"\x03\x15")
            ser.flush()
            time.sleep(0.8)
            ser.reset_input_buffer()
            app_command = b" true; " + DEMOS[args.demo].encode("ascii")
            payload = (
                app_command
                + b"; rc=$?; printf '\\r\\n"
                + app_marker
                + b":rc=%s\\r\\n' \"$rc\"\r"
            )
        for start in range(0, len(payload), args.chunk_size):
            ser.write(payload[start : start + args.chunk_size])
            ser.flush()
            if args.chunk_delay:
                time.sleep(args.chunk_delay)

        deadline = time.monotonic() + args.capture_window
        while time.monotonic() < deadline:
            waiting = ser.in_waiting
            if waiting:
                response.extend(ser.read(waiting))
                if interactive_app and re.search(app_marker_re, response):
                    break
            else:
                time.sleep(0.05)

        if interactive_catv:
            ser.write(b"\x03")
            ser.flush()
            deadline = time.monotonic() + 1.0
            while time.monotonic() < deadline:
                waiting = ser.in_waiting
                if waiting:
                    response.extend(ser.read(waiting))
                else:
                    time.sleep(0.05)

    print(f"sent {len(payload)} bytes to {args.port} ({args.demo})")
    print(f"captured {len(response)} bytes from login shell")
    if response:
        print(repr(bytes(response[-1000:])))
    if args.demo == "probe" and not re.search(
        rb"[\r\n]shell-path-ok: [A-Za-z0-9._-]+", response
    ):
        print("probe failed: no executed shell-path-ok marker was captured")
        return 1
    if args.demo == "recover" and not re.search(
        rb"[\r\n]recover-ok: [A-Za-z0-9._-]+", response
    ):
        print("recover failed: no executed recover-ok marker was captured")
        return 1
    if interactive_app:
        rc_match = re.search(app_marker_re, response)
        if rc_match is None:
            print(f"{args.demo} failed: marker not captured; press q on the USB keyboard during the window")
            return 1
        if rc_match.group(1) != b"0":
            rc_value = rc_match.group(1).decode("ascii")
            print(f"{args.demo} failed: app rc={rc_value}")
            return 1
        print(f"{args.demo} passed: app exited through observed shell marker")
    if args.demo == "scroll":
        clear_sequence = b"\x1b[r\x1b[2J\x1b[H"
        rendered_output = bytes(response).rpartition(clear_sequence)[2]
        observed = [
            int(value)
            for value in re.findall(rb"SCROLL-(\d{2})", rendered_output)
        ]
        expected = list(range(1, 51))
        if observed[: len(expected)] != expected:
            print(f"scroll failed: expected 1..50, captured {observed}")
            return 1
        print("scroll data path passed: captured SCROLL-01..SCROLL-50 in order")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
