#!/usr/bin/env python3
"""Bake a reviewed launcher preview region into the terminal RGB565 asset."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re

from PIL import Image


WIDTH = 1280
HEIGHT = 720
DEFAULT_REGION = (1127, 151, 1278, 322)
ARRAY_RE = re.compile(
    r"(launcher_bg_terminal_map\[\]\s*=\s*\{\n)(?P<data>.*?)(\n\};)",
    re.DOTALL,
)


def parse_region(value: str) -> tuple[int, int, int, int]:
    try:
        region = tuple(int(part) for part in value.split(","))
    except ValueError as exc:
        raise argparse.ArgumentTypeError("region values must be integers") from exc
    if len(region) != 4:
        raise argparse.ArgumentTypeError("region must be x0,y0,x1,y1")
    x0, y0, x1, y1 = region
    if not (0 <= x0 < x1 <= WIDTH and 0 <= y0 < y1 <= HEIGHT):
        raise argparse.ArgumentTypeError("region lies outside the 1280x720 asset")
    return x0, y0, x1, y1


def read_asset(path: Path) -> tuple[str, re.Match[str], bytes]:
    source = path.read_text(encoding="ascii")
    match = ARRAY_RE.search(source)
    if match is None:
        raise ValueError(f"launcher_bg_terminal_map was not found in {path}")
    raw = bytes(int(token, 16) for token in re.findall(r"0x([0-9a-fA-F]{2})", match["data"]))
    expected = WIDTH * HEIGHT * 2
    if len(raw) != expected:
        raise ValueError(f"expected {expected} RGB565 bytes, found {len(raw)}")
    return source, match, raw


def decode_rgb565(raw: bytes) -> Image.Image:
    rgb = bytearray(WIDTH * HEIGHT * 3)
    target = 0
    for offset in range(0, len(raw), 2):
        value = raw[offset] | (raw[offset + 1] << 8)
        red = (value >> 11) & 0x1F
        green = (value >> 5) & 0x3F
        blue = value & 0x1F
        rgb[target] = (red << 3) | (red >> 2)
        rgb[target + 1] = (green << 2) | (green >> 4)
        rgb[target + 2] = (blue << 3) | (blue >> 2)
        target += 3
    return Image.frombytes("RGB", (WIDTH, HEIGHT), bytes(rgb))


def encode_rgb565(image: Image.Image) -> bytes:
    rgb = image.convert("RGB").tobytes()
    raw = bytearray(WIDTH * HEIGHT * 2)
    target = 0
    for offset in range(0, len(rgb), 3):
        value = (
            ((rgb[offset] >> 3) << 11)
            | ((rgb[offset + 1] >> 2) << 5)
            | (rgb[offset + 2] >> 3)
        )
        raw[target] = value & 0xFF
        raw[target + 1] = value >> 8
        target += 2
    return bytes(raw)


def format_rows(raw: bytes) -> str:
    row_size = WIDTH * 2
    rows = []
    for offset in range(0, len(raw), row_size):
        row = raw[offset : offset + row_size]
        rows.append("  " + ", ".join(f"0x{value:02x}" for value in row) + ",")
    return "\n".join(rows)


def count_changed_pixels(before: bytes, after: bytes, region: tuple[int, int, int, int]) -> tuple[int, int]:
    x0, y0, x1, y1 = region
    inside = 0
    outside = 0
    for pixel in range(WIDTH * HEIGHT):
        offset = pixel * 2
        if before[offset : offset + 2] == after[offset : offset + 2]:
            continue
        x = pixel % WIDTH
        y = pixel // WIDTH
        if x0 <= x < x1 and y0 <= y < y1:
            inside += 1
        else:
            outside += 1
    return inside, outside


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--asset", type=Path, required=True, help="Existing launcher_bg_terminal.c"
    )
    parser.add_argument("--preview", type=Path, required=True, help="Reviewed 1280x720 PNG preview")
    parser.add_argument("--output", type=Path, required=True, help="Destination C asset")
    parser.add_argument(
        "--region",
        type=parse_region,
        default=DEFAULT_REGION,
        help="Exclusive patch rectangle x0,y0,x1,y1",
    )
    parser.add_argument("--merged-png", type=Path, help="Optional merged RGB preview")
    args = parser.parse_args()

    source, match, original_raw = read_asset(args.asset)
    asset_image = decode_rgb565(original_raw)
    with Image.open(args.preview) as preview_source:
        preview = preview_source.convert("RGB")
    if preview.size != (WIDTH, HEIGHT):
        raise ValueError(f"expected a {WIDTH}x{HEIGHT} preview, got {preview.size}")

    asset_image.paste(preview.crop(args.region), args.region[:2])
    updated_raw = encode_rgb565(asset_image)
    changed_inside, changed_outside = count_changed_pixels(original_raw, updated_raw, args.region)
    if changed_outside != 0:
        raise RuntimeError(f"refusing output: {changed_outside} pixels changed outside {args.region}")

    updated_source = (
        source[: match.start("data")]
        + format_rows(updated_raw)
        + source[match.end("data") :]
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(updated_source, encoding="ascii")
    if args.merged_png is not None:
        args.merged_png.parent.mkdir(parents=True, exist_ok=True)
        decode_rgb565(updated_raw).save(args.merged_png)

    print(f"region: {args.region}")
    print(f"changed pixels inside region: {changed_inside}")
    print(f"changed pixels outside region: {changed_outside}")
    print(f"RGB565 SHA256: {hashlib.sha256(updated_raw).hexdigest()}")
    print(f"asset: {args.output.resolve()}")
    if args.merged_png is not None:
        print(f"preview: {args.merged_png.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
