"""Electrical-angle unwrapping, validity checks, and sampling statistics."""

from __future__ import annotations

from dataclasses import dataclass, field
import math
import statistics


COUNTS_PER_ELECTRICAL_REV = 65_536
HALF_ELECTRICAL_REV = 32_768


class InvalidSample(RuntimeError):
    pass


@dataclass(frozen=True)
class TrackingPoint:
    raw_angle: int
    delta_counts: int
    accumulated_counts: int
    mechanical_turns: float


@dataclass
class AngleTracker:
    pole_pairs: int = 4
    max_rpm: float = 1000.0
    hf_clock_hz: float = 30_000.0
    expected_tick_step: int | None = None
    previous_angle_u16: int | None = None
    previous_time: float | None = None
    previous_tick: int | None = None
    accumulated_counts: int = 0
    sample_times: list[float] = field(default_factory=list)
    device_ticks: list[int] = field(default_factory=list)
    invalid_reason: str | None = None

    @property
    def mechanical_turns(self) -> float:
        return self.accumulated_counts / (
            self.pole_pairs * COUNTS_PER_ELECTRICAL_REV
        )

    def _invalidate(self, reason: str) -> None:
        self.invalid_reason = reason
        raise InvalidSample(reason)

    def update(self, raw_angle: int, host_time: float,
               device_tick: int | None = None) -> TrackingPoint:
        if self.invalid_reason:
            raise InvalidSample(self.invalid_reason)
        angle_u16 = raw_angle & 0xFFFF

        if self.previous_angle_u16 is None:
            self.previous_angle_u16 = angle_u16
            self.previous_time = host_time
            self.previous_tick = device_tick
            self.sample_times.append(host_time)
            if device_tick is not None:
                self.device_ticks.append(device_tick)
            return TrackingPoint(raw_angle, 0, 0, 0.0)

        if device_tick is not None and self.previous_tick is not None:
            tick_delta = (device_tick - self.previous_tick) & 0xFFFFFFFF
            if self.expected_tick_step is not None and tick_delta != self.expected_tick_step:
                self._invalidate(
                    f"device sample gap: expected {self.expected_tick_step} ticks, got {tick_delta}"
                )
            elapsed = tick_delta / self.hf_clock_hz
        else:
            if self.previous_time is None:
                self._invalidate("missing previous host timestamp")
            elapsed = host_time - self.previous_time
            if elapsed <= 0:
                self._invalidate(f"non-positive sample interval {elapsed}")

        # At max_rpm, an interval admitting >= half an electrical revolution is
        # ambiguous: modular angle alone cannot prove whether a wrap was lost.
        max_counts = (self.max_rpm / 60.0 * self.pole_pairs
                      * COUNTS_PER_ELECTRICAL_REV * elapsed)
        if max_counts >= HALF_ELECTRICAL_REV:
            self._invalidate(
                f"sampling gap {elapsed * 1000:.3f} ms admits "
                f"{max_counts:.0f} counts at {self.max_rpm:g} rpm"
            )

        delta = ((angle_u16 - self.previous_angle_u16 + HALF_ELECTRICAL_REV)
                 % COUNTS_PER_ELECTRICAL_REV) - HALF_ELECTRICAL_REV
        if abs(delta) >= HALF_ELECTRICAL_REV:
            self._invalidate(f"ambiguous adjacent angle delta {delta} counts")

        self.accumulated_counts += delta
        self.previous_angle_u16 = angle_u16
        self.previous_time = host_time
        self.previous_tick = device_tick
        self.sample_times.append(host_time)
        if device_tick is not None:
            self.device_ticks.append(device_tick)
        return TrackingPoint(raw_angle, delta, self.accumulated_counts,
                             self.mechanical_turns)

    def stats(self) -> dict[str, float | int | None]:
        intervals: list[float]
        source: str
        if len(self.device_ticks) >= 2:
            intervals = [
                ((b - a) & 0xFFFFFFFF) / self.hf_clock_hz
                for a, b in zip(self.device_ticks, self.device_ticks[1:])
            ]
            source = "device"
        else:
            intervals = [b - a for a, b in zip(self.sample_times, self.sample_times[1:])]
            source = "host"
        if not intervals:
            return {
                "samples": len(self.sample_times), "rate_source": source,
                "mean_hz": None, "min_hz": None, "p95_interval_ms": None,
                "max_interval_ms": None,
            }
        ordered = sorted(intervals)
        p95_index = min(len(ordered) - 1, math.ceil(0.95 * len(ordered)) - 1)
        return {
            "samples": len(self.sample_times),
            "rate_source": source,
            "mean_hz": 1.0 / statistics.fmean(intervals),
            "min_hz": 1.0 / max(intervals),
            "p95_interval_ms": ordered[p95_index] * 1000.0,
            "max_interval_ms": max(intervals) * 1000.0,
        }


def required_sample_rate(rpm: float, pole_pairs: int = 4,
                         design_margin: float = 4.0) -> float:
    return design_margin * pole_pairs * abs(rpm) / 60.0
