#!/usr/bin/env python3
"""Capture the Tab5 framebuffer over the private USB CDC debug protocol."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import time
import zlib

import serial


FRAME = b"\x1b]777;screen-capture?\x07"
BEGIN_RE = re.compile(
    rb"TAB5SHOT BEGIN v=(?P<version>\d+) width=(?P<width>\d+) "
    rb"height=(?P<height>\d+) format=(?P<format>[a-z0-9]+) "
    rb"bytes=(?P<bytes>\d+) rotation=(?P<rotation>\d+)"
)
END_RE = re.compile(rb"TAB5SHOT END crc32=(?P<crc>[0-9A-F]{8})")
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def rgb565_to_rgb(raw: bytes) -> bytearray:
    rgb = bytearray(len(raw) // 2 * 3)
    output = 0
    for offset in range(0, len(raw), 2):
        value = raw[offset] | (raw[offset + 1] << 8)
        red = (value >> 11) & 0x1F
        green = (value >> 5) & 0x3F
        blue = value & 0x1F
        rgb[output] = (red << 3) | (red >> 2)
        rgb[output + 1] = (green << 2) | (green >> 4)
        rgb[output + 2] = (blue << 3) | (blue >> 2)
        output += 3
    return rgb


def write_png(path: Path, width: int, height: int, rgb: bytes) -> None:
    stride = width * 3
    scanlines = bytearray((stride + 1) * height)
    for row in range(height):
        source = row * stride
        target = row * (stride + 1)
        scanlines[target] = 0
        scanlines[target + 1 : target + 1 + stride] = rgb[source : source + stride]
    image = (
        PNG_SIGNATURE
        + png_chunk(
            b"IHDR",
            struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0),
        )
        + png_chunk(b"IDAT", zlib.compress(scanlines, level=9))
        + png_chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(image)


def read_exact(ser: serial.Serial, length: int, deadline: float) -> bytes:
    result = bytearray()
    while len(result) < length and time.monotonic() < deadline:
        chunk = ser.read(min(65536, length - len(result)))
        if chunk:
            result.extend(chunk)
        else:
            time.sleep(0.005)
    if len(result) != length:
        raise TimeoutError(f"framebuffer truncated: expected {length}, got {len(result)}")
    return bytes(result)


def read_matching_line(
    ser: serial.Serial,
    pattern: re.Pattern[bytes],
    deadline: float,
) -> re.Match[bytes]:
    while time.monotonic() < deadline:
        line = ser.readline()
        match = pattern.search(line)
        if match is not None:
            return match
    raise TimeoutError(f"missing response matching {pattern.pattern!r}")


def compare_capture(
    baseline_path: Path,
    current_raw: bytes,
    width: int,
    height: int,
    tolerance: int,
    diff_path: Path,
) -> dict[str, object]:
    baseline = baseline_path.read_bytes()
    if len(baseline) != len(current_raw):
        raise ValueError(
            f"baseline size mismatch: expected {len(current_raw)}, got {len(baseline)}"
        )

    baseline_rgb = rgb565_to_rgb(baseline)
    current_rgb = rgb565_to_rgb(current_raw)
    diff_rgb = bytearray(len(current_rgb))
    changed = 0
    max_delta = 0
    delta_sum = 0
    min_x = width
    min_y = height
    max_x = -1
    max_y = -1
    for pixel in range(width * height):
        offset = pixel * 3
        deltas = [
            abs(current_rgb[offset + channel] - baseline_rgb[offset + channel])
            for channel in range(3)
        ]
        pixel_delta = max(deltas)
        max_delta = max(max_delta, pixel_delta)
        delta_sum += sum(deltas)
        if pixel_delta <= tolerance:
            continue
        changed += 1
        x = pixel % width
        y = pixel // width
        min_x = min(min_x, x)
        min_y = min(min_y, y)
        max_x = max(max_x, x)
        max_y = max(max_y, y)
        diff_rgb[offset] = 255
        diff_rgb[offset + 1] = min(255, deltas[1] * 4)
        diff_rgb[offset + 2] = min(255, deltas[2] * 4)

    write_png(diff_path, width, height, diff_rgb)
    total = width * height
    return {
        "baseline": str(baseline_path.resolve()),
        "different_pixels": changed,
        "different_percent": round(changed * 100.0 / total, 6),
        "channel_tolerance": tolerance,
        "max_channel_delta": max_delta,
        "mean_absolute_channel_delta": round(delta_sum / (total * 3), 6),
        "difference_bbox": (
            None
            if changed == 0
            else {"x": min_x, "y": min_y, "width": max_x - min_x + 1, "height": max_y - min_y + 1}
        ),
        "diff_png": str(diff_path.resolve()),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--output", type=Path, required=True, help="Output PNG path")
    parser.add_argument("--open-delay", type=float, default=2.0)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--baseline", type=Path, help="Prior .rgb565 capture")
    parser.add_argument("--channel-tolerance", type=int, default=0)
    parser.add_argument("--diff-output", type=Path)
    args = parser.parse_args()

    if not 0 <= args.channel_tolerance <= 255:
        parser.error("--channel-tolerance must be between 0 and 255")

    output = args.output.resolve()
    raw_path = output.with_suffix(".rgb565")
    metadata_path = output.with_suffix(".json")
    deadline = time.monotonic() + args.timeout

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
        ser.write(FRAME)
        ser.flush()
        begin = read_matching_line(ser, BEGIN_RE, deadline)
        width = int(begin.group("width"))
        height = int(begin.group("height"))
        byte_count = int(begin.group("bytes"))
        pixel_format = begin.group("format").decode("ascii")
        if pixel_format != "rgb565le" or byte_count != width * height * 2:
            raise ValueError(
                f"unsupported capture format: {pixel_format}, bytes={byte_count}"
            )
        raw = read_exact(ser, byte_count, deadline)
        end = read_matching_line(ser, END_RE, deadline)

    expected_crc = int(end.group("crc"), 16)
    actual_crc = zlib.crc32(raw) & 0xFFFFFFFF
    if actual_crc != expected_crc:
        raise ValueError(
            f"CRC mismatch: device={expected_crc:08X}, host={actual_crc:08X}"
        )

    rgb = rgb565_to_rgb(raw)
    write_png(output, width, height, rgb)
    raw_path.write_bytes(raw)
    metadata: dict[str, object] = {
        "protocol_version": int(begin.group("version")),
        "width": width,
        "height": height,
        "format": pixel_format,
        "rotation": int(begin.group("rotation")),
        "bytes": byte_count,
        "crc32": f"{actual_crc:08X}",
        "sha256": hashlib.sha256(raw).hexdigest(),
        "png": str(output),
        "raw": str(raw_path),
    }

    if args.baseline is not None:
        diff_path = (
            args.diff_output.resolve()
            if args.diff_output is not None
            else output.with_name(f"{output.stem}.diff.png")
        )
        metadata["comparison"] = compare_capture(
            args.baseline.resolve(),
            raw,
            width,
            height,
            args.channel_tolerance,
            diff_path,
        )

    metadata_path.write_text(
        json.dumps(metadata, ensure_ascii=True, indent=2) + "\n",
        encoding="ascii",
    )
    print(
        f"captured {width}x{height} {pixel_format}, crc32={actual_crc:08X}\n"
        f"png: {output}\nraw: {raw_path}\nmetadata: {metadata_path}"
    )
    if "comparison" in metadata:
        comparison = metadata["comparison"]
        assert isinstance(comparison, dict)
        print(
            "comparison: "
            f"{comparison['different_pixels']} pixels "
            f"({comparison['different_percent']}%), "
            f"bbox={comparison['difference_bbox']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
