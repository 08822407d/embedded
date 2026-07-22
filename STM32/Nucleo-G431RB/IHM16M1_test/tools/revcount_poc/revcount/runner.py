"""Read-only sampling and explicitly authorized powered-run orchestration."""

from __future__ import annotations

import csv
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import statistics
import time
from typing import Callable

from .protocol import (
    CONTROL_MODE_SPEED,
    MCPClient,
    MCP_FAULT_ACK,
    MCP_START,
    MCP_STOP,
    ProtocolError,
    ProtocolTimeout,
    REG_CONTROL_MODE,
    REG_FAULTS_FLAGS,
    REG_HALL_EL_ANGLE,
    REG_I_Q_MEAS,
    REG_SPEED_MEAS,
    REG_STATUS,
    STATUS_FAULT_NOW,
    STATUS_FAULT_OVER,
    STATUS_IDLE,
    STATUS_RUN,
    decode_async_hall,
)
from .tracker import AngleTracker, InvalidSample, required_sample_rate


CSV_FIELDS = [
    "host_time_s", "device_tick", "angle_raw", "delta_counts",
    "mechanical_turns", "status", "faults", "speed_rpm", "iq_raw",
    "valid", "event",
]


@dataclass
class Telemetry:
    status: int
    faults: int
    speed_rpm: int
    iq_raw: int = 0


@dataclass
class TrialSummary:
    mode: str
    valid: bool
    invalid_reason: str | None
    requested_turns: float | None
    measured_turns: float
    error_turns: float | None
    stop_trigger_turns: float | None
    coast_turns: float | None
    speed_rpm: int | None
    lead_turns: float | None
    stop_method: str | None
    sampling: dict
    csv_path: str


class CSVRecorder:
    def __init__(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        self.path = path
        self.file = path.open("w", newline="", encoding="utf-8")
        self.writer = csv.DictWriter(self.file, fieldnames=CSV_FIELDS)
        self.writer.writeheader()

    def row(self, **values) -> None:
        row = {field: "" for field in CSV_FIELDS}
        row.update(values)
        self.writer.writerow(row)
        self.file.flush()

    def close(self) -> None:
        self.file.close()

    def __enter__(self) -> "CSVRecorder":
        return self

    def __exit__(self, exc_type, exc, traceback) -> None:
        self.close()


def get_telemetry(client: MCPClient, include_iq: bool = False) -> Telemetry:
    registers = [
        (REG_STATUS, "B"),
        (REG_FAULTS_FLAGS, "I"),
        (REG_SPEED_MEAS, "i"),
    ]
    if include_iq:
        registers.append((REG_I_Q_MEAS, "h"))
    values = client.get_registers(registers)
    return Telemetry(*values)


def _write_summary(path: Path, summary: TrialSummary) -> None:
    path.with_suffix(".summary.json").write_text(
        json.dumps(asdict(summary), indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def run_read_only(client: MCPClient, csv_path: Path, duration_s: float,
                  expected_turns: float | None = None, max_rpm: float = 1000.0,
                  pole_pairs: int = 4,
                  clock: Callable[[], float] = time.monotonic) -> TrialSummary:
    """Poll only GET registers. This function contains no MCP write path."""
    tracker = AngleTracker(pole_pairs=pole_pairs, max_rpm=max_rpm)
    start = clock()
    last_telemetry: Telemetry | None = None
    invalid_reason: str | None = None
    with CSVRecorder(csv_path) as recorder:
        while clock() - start < duration_s:
            host_time = clock()
            angle, status, faults, speed = client.get_registers([
                (REG_HALL_EL_ANGLE, "h"),
                (REG_STATUS, "B"),
                (REG_FAULTS_FLAGS, "I"),
                (REG_SPEED_MEAS, "i"),
            ])
            last_telemetry = Telemetry(status, faults, speed)
            try:
                point = tracker.update(angle, host_time)
                valid = True
                event = "sample"
            except InvalidSample as exc:
                point = None
                valid = False
                invalid_reason = str(exc)
                event = "invalid"
            recorder.row(
                host_time_s=f"{host_time - start:.9f}",
                angle_raw=angle,
                delta_counts="" if point is None else point.delta_counts,
                mechanical_turns=(f"{tracker.mechanical_turns:.9f}"),
                status=status,
                faults=f"0x{faults:08X}",
                speed_rpm=speed,
                iq_raw="",
                valid=int(valid),
                event=event,
            )
            if invalid_reason:
                break

    measured = tracker.mechanical_turns
    error = None
    if expected_turns is not None:
        direction = -1.0 if expected_turns < 0 else 1.0
        error = direction * measured - abs(expected_turns)
    summary = TrialSummary(
        mode="read-only",
        valid=invalid_reason is None,
        invalid_reason=invalid_reason,
        requested_turns=expected_turns,
        measured_turns=measured,
        error_turns=error,
        stop_trigger_turns=None,
        coast_turns=None,
        speed_rpm=None if last_telemetry is None else last_telemetry.speed_rpm,
        lead_turns=None,
        stop_method=None,
        sampling=tracker.stats(),
        csv_path=str(csv_path),
    )
    _write_summary(csv_path, summary)
    return summary


def _check_runtime_safety(telemetry: Telemetry, commanded_sign: int,
                          iq_saturation_since: float | None,
                          iq_limit_raw: int = 3008,
                          require_run: bool = False) -> float | None:
    current_fault = (telemetry.faults >> 16) & 0xFFFF
    if current_fault or telemetry.status in (STATUS_FAULT_NOW, STATUS_FAULT_OVER):
        raise ProtocolError(
            f"fault/state trip: status={telemetry.status}, faults=0x{telemetry.faults:08X}"
        )
    if require_run and telemetry.status != STATUS_RUN:
        raise ProtocolError(
            f"motor left RUN unexpectedly: status={telemetry.status}"
        )
    if telemetry.status == STATUS_RUN and telemetry.speed_rpm * commanded_sign < -60:
        raise ProtocolError(
            f"measured speed has wrong sign: {telemetry.speed_rpm} rpm"
        )
    if abs(telemetry.iq_raw) >= iq_limit_raw:
        iq_saturation_since = iq_saturation_since or time.monotonic()
        if time.monotonic() - iq_saturation_since >= 0.2:
            raise ProtocolError(
                f"Iq remained at software limit for 0.2s: "
                f"raw={telemetry.iq_raw}, limit={iq_limit_raw}"
            )
    else:
        iq_saturation_since = None
    return iq_saturation_since


def _drain_async(client: MCPClient, tracker: AngleTracker, recorder: CSVRecorder,
                 start: float, telemetry: Telemetry, timeout: float = 0.1) -> int:
    payload = client.aspep.next_async(timeout)
    samples = decode_async_hall(payload, expected_mark=1)
    for sample in samples:
        now = time.monotonic()
        point = tracker.update(sample.angle, now, sample.device_tick)
        recorder.row(
            host_time_s=f"{now - start:.9f}",
            device_tick=sample.device_tick,
            angle_raw=sample.angle,
            delta_counts=point.delta_counts,
            mechanical_turns=f"{point.mechanical_turns:.9f}",
            status=telemetry.status,
            faults=f"0x{telemetry.faults:08X}",
            speed_rpm=telemetry.speed_rpm,
            iq_raw=telemetry.iq_raw,
            valid=1,
            event="sample",
        )
    return len(samples)


def run_powered(client: MCPClient, csv_path: Path, turns: float, rpm: int,
                lead: float, stop_method: str = "ramp",
                acceleration_ms: int = 1000, stop_ramp_ms: int = 1500,
                pole_pairs: int = 4) -> TrialSummary:
    if turns <= 0:
        raise ValueError("turns must be positive")
    if not 1 <= abs(rpm) <= 1000:
        raise ValueError("PoC safety bound: abs(rpm) must be 1..1000")
    if not 0 <= lead < turns:
        raise ValueError("lead must satisfy 0 <= lead < turns")
    if stop_method not in {"ramp", "direct"}:
        raise ValueError("stop_method must be ramp or direct")
    if not 100 <= acceleration_ms <= 10_000:
        raise ValueError("acceleration_ms must be 100..10000")
    if not 1000 <= stop_ramp_ms <= 2000:
        raise ValueError("stop_ramp_ms must be 1000..2000")

    direction = 1 if rpm > 0 else -1
    tracker = AngleTracker(
        pole_pairs=pole_pairs,
        max_rpm=1000.0,
        expected_tick_step=30,
    )
    start = time.monotonic()
    telemetry = get_telemetry(client, include_iq=True)
    stop_trigger_turns: float | None = None
    invalid_reason: str | None = None
    async_enabled = False
    motor_started = False
    iq_saturation_since: float | None = None

    with CSVRecorder(csv_path) as recorder:
        try:
            client.command(MCP_FAULT_ACK)
            deadline = time.monotonic() + 2.0
            while time.monotonic() < deadline:
                telemetry = get_telemetry(client, include_iq=True)
                if telemetry.status == STATUS_IDLE and telemetry.faults == 0:
                    break
                time.sleep(0.05)
            else:
                raise ProtocolError(
                    f"not IDLE after Fault Ack: status={telemetry.status}, "
                    f"faults=0x{telemetry.faults:08X}"
                )

            mode = client.get_register(REG_CONTROL_MODE, "B")
            if mode != CONTROL_MODE_SPEED:
                raise ProtocolError(f"control mode is {mode}, expected speed mode 3")

            client.configure_async_hall(hf_divider=30, buffer_size=32, mark=1)
            async_enabled = True
            motor_started = True
            client.command(MCP_START)
            client.set_speed_ramp(rpm, acceleration_ms)

            status_poll_at = time.monotonic()
            run_deadline = time.monotonic() + 2.0
            while telemetry.status != STATUS_RUN:
                _drain_async(client, tracker, recorder, start, telemetry)
                telemetry = get_telemetry(client, include_iq=True)
                iq_saturation_since = _check_runtime_safety(
                    telemetry, direction, iq_saturation_since
                )
                if time.monotonic() >= run_deadline:
                    raise ProtocolError(
                        f"motor did not enter RUN in 2s (status={telemetry.status})"
                    )

            motion_deadline = time.monotonic() + max(
                5.0,
                3.0 * turns * 60.0 / abs(rpm) + acceleration_ms / 1000.0 + 2.0,
            )
            while direction * tracker.mechanical_turns < turns - lead:
                _drain_async(client, tracker, recorder, start, telemetry)
                directed_turns = direction * tracker.mechanical_turns
                if directed_turns < -0.25:
                    raise ProtocolError(
                        f"Hall angle advances opposite the commanded direction: "
                        f"{tracker.mechanical_turns:.6f} turns"
                    )
                if time.monotonic() >= motion_deadline:
                    raise ProtocolError(
                        f"target was not reached before motion timeout: "
                        f"{directed_turns:.6f}/{turns - lead:.6f} turns"
                    )
                if time.monotonic() >= status_poll_at:
                    telemetry = get_telemetry(client, include_iq=True)
                    iq_saturation_since = _check_runtime_safety(
                        telemetry, direction, iq_saturation_since,
                        require_run=True,
                    )
                    status_poll_at = time.monotonic() + 0.05

            stop_trigger_turns = direction * tracker.mechanical_turns
            recorder.row(
                host_time_s=f"{time.monotonic() - start:.9f}",
                mechanical_turns=f"{tracker.mechanical_turns:.9f}",
                status=telemetry.status,
                faults=f"0x{telemetry.faults:08X}",
                speed_rpm=telemetry.speed_rpm,
                iq_raw=telemetry.iq_raw,
                valid=1,
                event="stop-trigger",
            )

            if stop_method == "ramp":
                client.set_speed_ramp(0, stop_ramp_ms)
                ramp_deadline = time.monotonic() + stop_ramp_ms / 1000.0
                while time.monotonic() < ramp_deadline:
                    try:
                        _drain_async(client, tracker, recorder, start, telemetry, 0.1)
                    except ProtocolTimeout:
                        pass
                    if time.monotonic() >= status_poll_at:
                        telemetry = get_telemetry(client, include_iq=True)
                        iq_saturation_since = _check_runtime_safety(
                            telemetry, direction, iq_saturation_since,
                            require_run=True,
                        )
                        status_poll_at = time.monotonic() + 0.05
            client.command(MCP_STOP)
            motor_started = False

            stable_since: float | None = None
            stop_deadline = time.monotonic() + 5.0
            previous_counts = tracker.accumulated_counts
            while time.monotonic() < stop_deadline:
                try:
                    _drain_async(client, tracker, recorder, start, telemetry, 0.1)
                except ProtocolTimeout:
                    pass
                telemetry = get_telemetry(client, include_iq=True)
                iq_saturation_since = _check_runtime_safety(
                    telemetry, direction, iq_saturation_since
                )
                moved = abs(tracker.accumulated_counts - previous_counts)
                previous_counts = tracker.accumulated_counts
                if (telemetry.status == STATUS_IDLE
                        and abs(telemetry.speed_rpm) <= 30 and moved <= 32):
                    stable_since = stable_since or time.monotonic()
                    if time.monotonic() - stable_since >= 0.3:
                        break
                else:
                    stable_since = None
            else:
                raise ProtocolError("motor did not reach the stop/stable criterion in 5s")

        except Exception as exc:
            invalid_reason = str(exc)
            if motor_started:
                try:
                    client.command(MCP_STOP, timeout=0.5)
                except Exception:
                    pass
        except BaseException:
            if motor_started:
                try:
                    client.command(MCP_STOP, timeout=0.5)
                except Exception:
                    pass
            raise
        finally:
            if async_enabled:
                try:
                    client.disable_async()
                except Exception:
                    pass

    measured = direction * tracker.mechanical_turns
    summary = TrialSummary(
        mode="powered",
        valid=invalid_reason is None,
        invalid_reason=invalid_reason,
        requested_turns=turns,
        measured_turns=measured,
        error_turns=measured - turns,
        stop_trigger_turns=stop_trigger_turns,
        coast_turns=(None if stop_trigger_turns is None
                     else measured - stop_trigger_turns),
        speed_rpm=rpm,
        lead_turns=lead,
        stop_method=stop_method,
        sampling=tracker.stats(),
        csv_path=str(csv_path),
    )
    _write_summary(csv_path, summary)
    return summary


def summarize_trials(summaries: list[TrialSummary]) -> dict[str, float | int]:
    errors = [item.error_turns for item in summaries
              if item.valid and item.error_turns is not None]
    if not errors:
        return {"valid_trials": 0}
    return {
        "valid_trials": len(errors),
        "mean_error_turns": statistics.fmean(errors),
        "worst_abs_error_turns": max(abs(value) for value in errors),
        "std_error_turns": statistics.stdev(errors) if len(errors) > 1 else 0.0,
    }
