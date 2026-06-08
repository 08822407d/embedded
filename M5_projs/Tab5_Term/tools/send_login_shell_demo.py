#!/usr/bin/env python3
"""Send visual terminal demos through the Tab5 USB-to-login-UART bridge."""

from __future__ import annotations

import argparse
import re
import time

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
    "probe": SHELL_PROBE,
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
