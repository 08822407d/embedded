#!/usr/bin/env python3
"""Collect and analyze Tab5 power-detection probe logs."""

from __future__ import annotations

import argparse
import csv
import json
import statistics
import sys
import time
from pathlib import Path

import serial
from serial.tools import list_ports


def port_present(port: str) -> bool:
    wanted = port.upper()
    return any(candidate.device.upper() == wanted for candidate in list_ports.comports())


def wait_for_port(port: str, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if port_present(port):
            return
        time.sleep(0.2)
    raise RuntimeError(f"Serial port {port} did not appear within {timeout:.1f}s")


def open_serial(port: str, baud: int, open_delay: float) -> serial.Serial:
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.1
    ser.write_timeout = 2.0
    ser.rtscts = False
    ser.dsrdtr = False
    ser.dtr = False
    ser.rts = False
    ser.open()
    time.sleep(open_delay)
    return ser


def read_available(ser: serial.Serial) -> bytes:
    waiting = ser.in_waiting
    if waiting:
        return ser.read(waiting)
    return b""


def send_line(ser: serial.Serial, command: str) -> None:
    ser.write(command.encode("ascii") + b"\r\n")
    ser.flush()


def collect_for_window(
    port: str,
    baud: int,
    duration: float,
    open_delay: float,
    final_wait: float,
    initial_ser: serial.Serial | None = None,
) -> tuple[serial.Serial, bytes]:
    transcript = bytearray()
    ser: serial.Serial | None = initial_ser
    connected = port_present(port)
    deadline = time.monotonic() + duration

    while time.monotonic() < deadline:
        present = port_present(port)
        if present and ser is None:
            try:
                ser = open_serial(port, baud, open_delay)
                print(f"[power-detect] port opened: {port}")
            except serial.SerialException as exc:
                print(f"[power-detect] open failed: {exc}")
                ser = None
        elif not present and connected:
            print(f"[power-detect] port disappeared: {port}")

        connected = present

        if ser is not None:
            try:
                transcript.extend(read_available(ser))
            except (OSError, serial.SerialException) as exc:
                print(f"[power-detect] read interrupted: {exc}")
                try:
                    ser.close()
                except serial.SerialException:
                    pass
                ser = None

        time.sleep(0.05)

    if ser is not None and ser.is_open:
        return ser, bytes(transcript)

    print("[power-detect] waiting for final port reconnect...")
    wait_for_port(port, final_wait)
    ser = open_serial(port, baud, open_delay)
    print(f"[power-detect] final port opened: {port}")
    transcript.extend(read_available(ser))
    return ser, bytes(transcript)


def read_until_end(ser: serial.Serial, timeout: float) -> bytes:
    captured = bytearray()
    deadline = time.monotonic() + timeout
    marker = b"PWRLOG END"
    while time.monotonic() < deadline:
        try:
            data = read_available(ser)
        except (OSError, serial.SerialException) as exc:
            raise RuntimeError(f"Serial read failed during dump: {exc}") from exc
        if data:
            captured.extend(data)
            if marker in captured:
                return bytes(captured)
        else:
            time.sleep(0.02)
    raise RuntimeError(
        "PWRLOG dump did not finish before timeout. "
        f"Captured tail: {bytes(captured[-300:])!r}"
    )


def query_dump(ser: serial.Serial, timeout: float) -> bytes:
    ser.reset_input_buffer()
    send_line(ser, "PWRLOG DUMP")
    return read_until_end(ser, timeout)


def clear_probe(ser: serial.Serial) -> None:
    ser.reset_input_buffer()
    send_line(ser, "PWRLOG CLEAR")
    time.sleep(0.3)
    _ = read_available(ser)


def parse_samples(text: str) -> list[dict[str, int]]:
    lines = text.splitlines()
    try:
        start = lines.index("PWRLOG BEGIN")
        end = lines.index("PWRLOG END")
    except ValueError as exc:
        raise RuntimeError("PWRLOG BEGIN/END markers were not found") from exc

    csv_lines = lines[start + 1 : end]
    reader = csv.DictReader(csv_lines)
    samples: list[dict[str, int]] = []
    for row in reader:
        if not row or row.get("seq") is None:
            continue
        samples.append({key: int(value) for key, value in row.items()})
    return samples


def series_stats(values: list[int]) -> dict[str, float | int | None]:
    if not values:
        return {
            "count": 0,
            "min": None,
            "max": None,
            "mean": None,
            "median": None,
            "stdev": None,
        }
    return {
        "count": len(values),
        "min": min(values),
        "max": max(values),
        "mean": statistics.fmean(values),
        "median": statistics.median(values),
        "stdev": statistics.pstdev(values) if len(values) > 1 else 0.0,
    }


def group_stats(samples: list[dict[str, int]], key: str, value_name: str) -> dict[str, dict[str, float | int | None]]:
    result: dict[str, dict[str, float | int | None]] = {}
    for flag in sorted({sample[key] for sample in samples}):
        values = [sample[value_name] for sample in samples if sample[key] == flag]
        result[str(flag)] = series_stats(values)
    return result


def kmeans_two(values: list[int]) -> dict[str, object] | None:
    if len(values) < 8:
        return None

    ordered = sorted(values)
    low = float(ordered[len(ordered) // 10])
    high = float(ordered[len(ordered) * 9 // 10])
    if low == high:
        return None

    for _ in range(30):
        low_cluster: list[int] = []
        high_cluster: list[int] = []
        for value in values:
            if abs(value - low) <= abs(value - high):
                low_cluster.append(value)
            else:
                high_cluster.append(value)
        if not low_cluster or not high_cluster:
            return None
        new_low = statistics.fmean(low_cluster)
        new_high = statistics.fmean(high_cluster)
        if abs(new_low - low) < 0.1 and abs(new_high - high) < 0.1:
            break
        low, high = new_low, new_high

    if low > high:
        low_cluster, high_cluster = high_cluster, low_cluster
        low, high = high, low

    low_stats = series_stats(low_cluster)
    high_stats = series_stats(high_cluster)
    separation = float(high_stats["mean"]) - float(low_stats["mean"])
    noise = max(float(low_stats["stdev"]), float(high_stats["stdev"]), 1.0)
    threshold = (float(low_stats["mean"]) + float(high_stats["mean"])) / 2.0
    confidence = "low"
    if separation >= max(150.0, noise * 6.0):
        confidence = "high"
    elif separation >= max(80.0, noise * 4.0):
        confidence = "medium"

    return {
        "low_cluster": low_stats,
        "high_cluster": high_stats,
        "separation_ma": separation,
        "threshold_ma": threshold,
        "confidence": confidence,
    }


def analyze(samples: list[dict[str, int]]) -> dict[str, object]:
    currents = [sample["current_ma"] for sample in samples]
    voltages = [sample["voltage_mv"] for sample in samples]
    api_values = sorted({sample["api"] for sample in samples})
    pin_values = sorted({sample["pin"] for sample in samples})
    usb_values = sorted({sample["usb"] for sample in samples})

    analysis: dict[str, object] = {
        "sample_count": len(samples),
        "duration_ms": samples[-1]["ms"] - samples[0]["ms"] if len(samples) >= 2 else 0,
        "api_values": api_values,
        "pin_values": pin_values,
        "usb_values": usb_values,
        "current_ma": series_stats(currents),
        "voltage_mv": series_stats(voltages),
        "current_by_api": group_stats(samples, "api", "current_ma"),
        "current_by_pin": group_stats(samples, "pin", "current_ma"),
        "current_by_usb": group_stats(samples, "usb", "current_ma"),
    }

    current_clusters = kmeans_two(currents)
    analysis["current_clusters"] = current_clusters

    suggestions: list[str] = []
    if len(api_values) == 1:
        suggestions.append("api/isCharging did not change during this capture.")
    if len(pin_values) == 1:
        suggestions.append("raw CHG_STAT pin did not change during this capture.")

    if current_clusters is None:
        suggestions.append("current_ma did not form two usable clusters.")
    else:
        confidence = str(current_clusters["confidence"])
        threshold = float(current_clusters["threshold_ma"])
        if confidence == "low":
            suggestions.append(
                "current_ma split is weak; do not use it as a production rule yet."
            )
        else:
            suggestions.append(
                f"candidate current threshold: {threshold:.1f} mA "
                f"(confidence={confidence})."
            )

    if set(usb_values) == {0, 1}:
        usb_stats = analysis["current_by_usb"]
        mean0 = usb_stats["0"]["mean"]
        mean1 = usb_stats["1"]["mean"]
        if mean0 is not None and mean1 is not None:
            delta = float(mean1) - float(mean0)
            suggestions.append(
                f"usb-connected current mean delta: {delta:+.1f} mA "
                "(positive means host-connected samples read higher current)."
            )
    else:
        suggestions.append(
            "usb connection flag did not record both states; use cable cycles "
            "that make the CDC port disappear and reappear if possible."
        )

    analysis["suggestions"] = suggestions
    return analysis


def write_csv(path: Path, samples: list[dict[str, int]]) -> None:
    if not samples:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(samples[0].keys()))
        writer.writeheader()
        writer.writerows(samples)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--duration", type=float, default=180.0)
    parser.add_argument("--open-delay", type=float, default=1.0)
    parser.add_argument("--final-wait", type=float, default=180.0)
    parser.add_argument("--dump-timeout", type=float, default=30.0)
    parser.add_argument("--output-root", default=".logs")
    parser.add_argument("--no-clear", action="store_true")
    args = parser.parse_args()

    output_root = Path(args.output_root)
    output_root.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    transcript_path = output_root / f"power-detect-{stamp}.serial.log"
    dump_path = output_root / f"power-detect-{stamp}.dump.log"
    csv_path = output_root / f"power-detect-{stamp}.csv"
    analysis_path = output_root / f"power-detect-{stamp}.analysis.json"

    wait_for_port(args.port, args.final_wait)
    ser = open_serial(args.port, args.baud, args.open_delay)
    print(f"[power-detect] connected to {args.port}")
    if not args.no_clear:
        clear_probe(ser)
        print("[power-detect] probe log cleared")
    print(
        "[power-detect] start cable plug/unplug cycles now; "
        f"collection window is {args.duration:.1f}s"
    )

    try:
        ser, transcript = collect_for_window(
            args.port,
            args.baud,
            args.duration,
            args.open_delay,
            args.final_wait,
            ser,
        )
        dump = query_dump(ser, args.dump_timeout)
    finally:
        try:
            ser.close()
        except Exception:
            pass

    transcript_path.write_bytes(transcript)
    dump_path.write_bytes(dump)
    text = dump.decode("utf-8", errors="replace")
    samples = parse_samples(text)
    write_csv(csv_path, samples)
    analysis = analyze(samples)
    analysis_path.write_text(
        json.dumps(analysis, indent=2, sort_keys=True),
        encoding="utf-8",
    )

    print(f"[power-detect] samples: {len(samples)}")
    print(f"[power-detect] serial log: {transcript_path}")
    print(f"[power-detect] dump log: {dump_path}")
    print(f"[power-detect] csv: {csv_path}")
    print(f"[power-detect] analysis: {analysis_path}")
    for item in analysis["suggestions"]:
        print(f"[power-detect] {item}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("Interrupted.", file=sys.stderr)
        raise SystemExit(130)
