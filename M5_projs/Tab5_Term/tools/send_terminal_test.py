#!/usr/bin/env python3
"""Send deterministic terminal test byte streams to a Tab5 CDC injection build."""

from __future__ import annotations

import argparse
import time

import serial


def utf8(text: str) -> bytes:
    return text.encode("utf-8")


STAGE1_SMOKE = (
    b"\x1b[2J\x1b[H"
    b"CDC injection render test\r\n"
    b"hello\r\nworld\r\n"
    b"CR+EL: abc\rCR+EL: XYZ\x1b[K\r\n"
    b"Backspace: 12\b3\r\n"
    b"Tabs: A\tB\r\n"
    b"OSC skip: before\x1b]0;ignored title\x07after\r\n"
    b"Cursor left: start\x1b[2D!!\r\n"
    b"Colors: \x1b[31mred\x1b[0m \x1b[32mgreen\x1b[0m "
    b"\x1b[38;5;226myellow256\x1b[0m\r\n"
    b"DEC graphics:\r\n"
    b"\x1b(0lqqqqk\x1b(B\r\n"
    b"\x1b(0x    x\x1b(B\r\n"
    b"\x1b(0mqqqqj\x1b(B\r\n"
    b"G1 graphics: \x1b)0\x0elqqk\x0f back ascii\r\n"
    b"\x1b[16;1HRow 16 via CSI H\r\n"
    b"8-bit CSI clear-line works?\x9b2Kcleared tail\r\n"
    b"End of CDC injection test\r\n"
)

STAGE2_SCREEN = (
    b"\x1b[2J\x1b[H"
    b"VT102 screen model test\r\n"
    b"\x1b[2;1HICH: abcd\x1b[2;8H\x1b[2@XY"
    b"\x1b[3;1HDCH: abcdef\x1b[3;8H\x1b[2P"
    b"\x1b[4;1HECH: abcdef\x1b[4;8H\x1b[3X"
    b"\x1b[6;1HIL base 1"
    b"\x1b[7;1HIL base 2"
    b"\x1b[8;1HIL base 3"
    b"\x1b[7;1H\x1b[LIL inserted"
    b"\x1b[10;1HDL keep 1"
    b"\x1b[11;1HDL delete 2"
    b"\x1b[12;1HDL keep 3"
    b"\x1b[11;1H\x1b[M"
    b"\x1b[14;17r\x1b[?6h\x1b[1;1HOrigin mode row 14"
    b"\x1b[?6l\x1b[r"
    b"\x1b[18;1HTab stop:\x1b[18;20H\x1bH\x1b[18;11HA\tB"
    b"\x1b[20;1HQueries: \x1b[5n\x1b[6n\x1b[c"
    b"\x1b[22;1HEnd of VT102 screen test\r\n"
)

STAGE3_COLOR = (
    b"\x1b[2J\x1b[H"
    b"SGR attributes and color test\r\n"
    b"Attrs: normal \x1b[1mbold\x1b[22m "
    b"\x1b[2mdim\x1b[22m \x1b[4munderline\x1b[24m "
    b"\x1b[7minverse\x1b[27m hidden:[\x1b[8mSECRET\x1b[28m]\r\n"
    b"16 fg: "
    b"\x1b[30m30\x1b[31m 31\x1b[32m 32\x1b[33m 33"
    b"\x1b[34m 34\x1b[35m 35\x1b[36m 36\x1b[37m 37\x1b[0m\r\n"
    b"bright: "
    b"\x1b[90m90\x1b[91m 91\x1b[92m 92\x1b[93m 93"
    b"\x1b[94m 94\x1b[95m 95\x1b[96m 96\x1b[97m 97\x1b[0m\r\n"
    b"256 fg: "
    b"\x1b[38;5;16m016 \x1b[38;5;21m021 \x1b[38;5;46m046 "
    b"\x1b[38;5;82m082 \x1b[38;5;196m196 \x1b[38;5;202m202 "
    b"\x1b[38;5;226m226 \x1b[38;5;231m231\x1b[0m\r\n"
    b"truecolor: \x1b[38;2;255;128;0morange\x1b[0m "
    b"\x1b[38;2;80;180;255msky\x1b[0m "
    b"\x1b[48;5;24m bg256 \x1b[0m "
    b"\x1b[48;2;80;20;120m bgtrue \x1b[0m\r\n"
    b"BCE EL: \x1b[44mblue to EOL\x1b[K\x1b[0m\r\n"
    b"BCE ECH: \x1b[45mabcdef\x1b[6D\x1b[4X\x1b[0m\r\n"
    b"Reset: \x1b[31;44mcolored\x1b[0m default again\r\n"
    b"End of SGR color test\r\n"
)

STAGE4_XTERM = (
    b"\x1b[2J\x1b[H"
    b"Stage4 xterm/DEC essentials test\r\n"
    b"Primary buffer survives alternate screen.\r\n"
    b"\x1b[?1049h"
    b"ALTERNATE SCREEN - should disappear after return\r\n"
    b"\x1b[31mTemporary alternate red text\x1b[0m\r\n"
    b"\x1b[?1049l"
    b"\x1b[4;1HAlt screen restored: primary line above should remain\x1b[K\r\n"
    b"\x1b[?1h\x1b[?66h\x1b[?2004h\x1b[?1000h\x1b[?1006h"
    b"Private modes toggled: app cursor/keypad, bracketed paste, mouse\x1b[K\r\n"
    b"\x1b[?1006l\x1b[?1000l\x1b[?2004l\x1b[?66l\x1b[?1l"
    b"Bracketed paste markers: before \x1b[200~PASTE\x1b[201~ after\x1b[K\r\n"
    b"Replies to host: DA \x1b[c SEC \x1b[>c CPR \x1b[6n DECCPR \x1b[?6n DECID \x1bZ\r\n"
    b"UTF-8 path: cafe=\xc3\xa9 omega=\xce\xa9 cjk=\xe4\xb8\xad\r\n"
    b"Mouse mode off, no raw private-mode text leaked.\r\n"
    b"End of Stage4 xterm/DEC essentials test\r\n"
)

STAGE7_UNICODE = (
    b"\x1b[2J\x1b[H"
    b"U1/U2 Unicode width test\r\n"
    b"ASCII: A\xe4\xb8\xadB\r\n"
    b"Combining: e\xcc\x81X\r\n"
    b"\x1b[4;1HBoundary:\x1b[4;64H\xe4\xb8\xadZ"
    b"\x1b[6;1HErase: A\xe4\xb8\xadBC\x1b[6;10H\x1b[X"
    b"\x1b[7;1HInsert: A\xe4\xb8\xadBC\x1b[7;10H\x1b[@"
    b"\x1b[8;1HDelete: A \xe4\xb8\xadBC\x1b[8;10H\x1b[P"
    b"\x1b[10;12r"
    b"\x1b[11;1HScroll: A\xe4\xb8\xadB"
    b"\x1b[12;1Hbottom\n"
    b"\x1b[r"
    b"\x1b[15;1HInvalid: \xe0\x80\xafOK"
    b"\x1b[16;1HEnd of U1/U2 Unicode width test"
)

STAGE7_UNICODE_GRAPHICS = (
    b"\x1b[2J\x1b[H"
    b"U3 Unicode graphics fallback test\r\n"
    + utf8("Light:   \u250c\u2500\u2500\u2500\u2500\u252c\u2500\u2500\u2500\u2500\u2510\r\n")
    + utf8("         \u2502    \u2502    \u2502\r\n")
    + utf8("         \u251c\u2500\u2500\u2500\u2500\u253c\u2500\u2500\u2500\u2500\u2524\r\n")
    + utf8("         \u2514\u2500\u2500\u2500\u2500\u2534\u2500\u2500\u2500\u2500\u2518\r\n")
    + utf8("Heavy:   \u250f\u2501\u2501\u2501\u2501\u2533\u2501\u2501\u2501\u2501\u2513\r\n")
    + utf8("         \u2503    \u2503    \u2503\r\n")
    + utf8("         \u2517\u2501\u2501\u2501\u2501\u253b\u2501\u2501\u2501\u2501\u251b\r\n")
    + utf8("Rounded: \u256d\u2500\u2500\u2500\u2500\u256e  \u2570\u2500\u2500\u2500\u2500\u256f\r\n")
    + utf8("Blocks:  \u258f\u258e\u258d\u258c\u258b\u258a\u2589 \u2588 \u2580\u2584\u2590 \u2591\u2592\u2593\r\n")
    + b"End of U3 Unicode graphics fallback test"
)

STAGE8_PROTOCOL = (
    b"\x1b[2J\x1b[H"
    b"Stage8 protocol readiness test\r\n"
    b"xterm text area size query: \x1b[18t\r\n"
    b"Expected reply for future mature transports: CSI 8;32;64 t\r\n"
    b"Raw UART login still uses persistent stty rows 32 cols 64\r\n"
    b"End of Stage8 protocol readiness test"
)

FONT_PREVIEW = (
    b"\x1b[2J\x1b[H"
    b"Font debug: DejaVu18 glyphs in fixed 18x20 cells\r\n"
    b"Cell geometry should remain 64 columns x 32 rows.\r\n"
    b"\r\n"
    b"Upper: ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n"
    b"Lower: abcdefghijklmnopqrstuvwxyz\r\n"
    b"Digit: 0123456789  Punct: !?.,:;+-*/=()[]{}<>_#@$%&|\r\n"
    b"Dense: Il1| O0Q 8B S5 Z2 mnmw vwxy  []{} ()<> /\\\\\r\n"
    b"\r\n"
    b"Attrs: normal \x1b[1mbold\x1b[22m \x1b[2mdim\x1b[22m "
    b"\x1b[4munderline\x1b[24m \x1b[7minverse\x1b[27m\r\n"
    b"Color: \x1b[31mred\x1b[0m \x1b[32mgreen\x1b[0m "
    b"\x1b[38;5;226myellow256\x1b[0m \x1b[38;2;80;180;255mtruecolor\x1b[0m\r\n"
    b"\r\n"
    b"DEC graphics:\r\n"
    b"\x1b(0lqqqqqqqqqqk\x1b(B  box with current cell geometry\r\n"
    b"\x1b(0x          x\x1b(B\r\n"
    b"\x1b(0tqqqqnqqqqu\x1b(B\r\n"
    b"\x1b(0x          x\x1b(B\r\n"
    b"\x1b(0mqqqqqqqqqqj\x1b(B\r\n"
    b"\r\n"
    b"UTF fallback: cafe=\xc3\xa9 omega=\xce\xa9 cjk=\xe4\xb8\xad unsupported=\xe2\x98\x83\r\n"
    b"End of font preview\r\n"
)

FONT_PROP_PREVIEW = (
    b"\x1b[2J\x1b[H"
    b"Font preview: DejaVu18 true proportional in 18x20 rows\r\n"
    b"Debug fixed-cell mode should be OFF for this preview.\r\n"
    b"\r\n"
    b"Upper: ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n"
    b"Lower: abcdefghijklmnopqrstuvwxyz\r\n"
    b"Narrow: iiiiii llllll !!!!!! ||||||     Wide: MMMMMM WWWWWW\r\n"
    b"Digit: 0123456789  Punct: !?.,:;+-*/=()[]{}<>_#@$%&|\r\n"
    b"Dense: Il1| O0Q 8B S5 Z2 mnmw vwxy  []{} ()<> /\\\\\r\n"
    b"\r\n"
    b"Attrs: normal \x1b[1mbold\x1b[22m \x1b[2mdim\x1b[22m "
    b"\x1b[4munderline\x1b[24m \x1b[7minverse\x1b[27m\r\n"
    b"Color: \x1b[31mred\x1b[0m \x1b[32mgreen\x1b[0m "
    b"\x1b[38;5;226myellow256\x1b[0m \x1b[38;2;80;180;255mtruecolor\x1b[0m\r\n"
    b"\r\n"
    b"DEC graphics:\r\n"
    b"\x1b(0lqqqqqqqqqqk\x1b(B  box with current cell geometry\r\n"
    b"\x1b(0x          x\x1b(B\r\n"
    b"\x1b(0tqqqqnqqqqu\x1b(B\r\n"
    b"\x1b(0x          x\x1b(B\r\n"
    b"\x1b(0mqqqqqqqqqqj\x1b(B\r\n"
    b"\r\n"
    b"UTF fallback: cafe=\xc3\xa9 omega=\xce\xa9 cjk=\xe4\xb8\xad unsupported=\xe2\x98\x83\r\n"
    b"End of proportional font preview\r\n"
)


TESTS = {
    "font-preview": FONT_PREVIEW,
    "font-prop-preview": FONT_PROP_PREVIEW,
    "stage1-smoke": STAGE1_SMOKE,
    "stage2-screen": STAGE2_SCREEN,
    "stage3-color": STAGE3_COLOR,
    "stage4-xterm": STAGE4_XTERM,
    "stage7-unicode": STAGE7_UNICODE,
    "stage7-unicode-graphics": STAGE7_UNICODE_GRAPHICS,
    "stage8-protocol": STAGE8_PROTOCOL,
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--test", choices=sorted(TESTS), default="stage1-smoke")
    parser.add_argument(
        "--open-delay",
        type=float,
        default=3.0,
        help="Delay after opening the port; opening CDC can reset ESP32-P4",
    )
    parser.add_argument(
        "--chunk-size",
        type=int,
        default=32,
        help="Maximum bytes to write per serial chunk",
    )
    parser.add_argument(
        "--chunk-delay",
        type=float,
        default=0.05,
        help="Delay between serial chunks in seconds",
    )
    parser.add_argument(
        "--read-response-window",
        type=float,
        default=0.4,
        help="Seconds to read terminal query responses after sending",
    )
    args = parser.parse_args()

    payload = TESTS[args.test]
    if args.chunk_size < 1:
        parser.error("--chunk-size must be at least 1")
    if args.chunk_delay < 0:
        parser.error("--chunk-delay must be non-negative")
    if args.read_response_window < 0:
        parser.error("--read-response-window must be non-negative")

    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 1
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
        response = bytearray()
        deadline = time.monotonic() + args.read_response_window
        while time.monotonic() < deadline:
            waiting = ser.in_waiting
            if waiting:
                response.extend(ser.read(waiting))
                deadline = time.monotonic() + 0.05
            else:
                time.sleep(0.01)

    print(
        f"sent {len(payload)} bytes to {args.port} ({args.test}), "
        f"chunk_size={args.chunk_size}, chunk_delay={args.chunk_delay:.3f}s"
    )
    if response:
        print(f"received {len(response)} response bytes: {response!r}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
