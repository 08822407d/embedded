import tempfile
import struct
import time
import unittest
from pathlib import Path

from revcount.protocol import (
    MCP_FAULT_ACK,
    MCP_START,
    MCP_STOP,
    REG_FAULTS_FLAGS,
    REG_I_Q_MEAS,
    REG_SPEED_MEAS,
    REG_STATUS,
)
from revcount.runner import run_powered, run_read_only


class FakeReadOnlyMCP:
    def __init__(self):
        self.index = 0
        self.calls = []

    def get_registers(self, registers):
        self.calls.append(registers)
        angle = (self.index * 256) & 0xFFFF
        if angle >= 0x8000:
            angle -= 0x10000
        self.index += 1
        return [angle, 0, 0, 0]


class FakeClock:
    def __init__(self, step=0.001):
        self.value = 0.0
        self.step = step

    def __call__(self):
        value = self.value
        self.value += self.step
        return value


class RunnerTests(unittest.TestCase):
    def test_read_only_path_uses_only_get_and_writes_csv(self):
        client = FakeReadOnlyMCP()
        clock = FakeClock()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trial.csv"
            summary = run_read_only(
                client, path, duration_s=0.020, expected_turns=0,
                clock=clock,
            )
            self.assertTrue(summary.valid)
            self.assertTrue(path.exists())
            self.assertTrue(path.with_suffix(".summary.json").exists())
        self.assertGreater(len(client.calls), 5)
        self.assertTrue(all(len(call) == 4 for call in client.calls))

    def test_powered_mock_sequence_reaches_stop_and_disables_stream(self):
        client = FakePoweredMCP()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "powered.csv"
            summary = run_powered(
                client, path, turns=0.003, rpm=500, lead=0,
                stop_method="direct",
            )
            self.assertTrue(summary.valid)
            self.assertTrue(path.exists())
        self.assertEqual(client.commands[:2], [MCP_FAULT_ACK, MCP_START])
        self.assertIn(MCP_STOP, client.commands)
        self.assertEqual(client.ramps, [(500, 1000)])
        self.assertTrue(client.async_configured)
        self.assertTrue(client.async_disabled)

    def test_powered_mock_stops_if_state_leaves_run(self):
        client = DroppingStateMCP()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "state-drop.csv"
            summary = run_powered(
                client, path, turns=0.1, rpm=500, lead=0,
                stop_method="direct",
            )
            self.assertFalse(summary.valid)
            self.assertIn("left RUN unexpectedly", summary.invalid_reason)
        self.assertIn(MCP_STOP, client.commands)
        self.assertTrue(client.async_disabled)


class FakeAsyncEndpoint:
    def __init__(self, owner):
        self.owner = owner
        self.tick = 1000
        self.angle = 0

    def next_async(self, timeout=1.0):
        time.sleep(0.05)
        if self.owner.running:
            self.angle = (self.angle + 1000) & 0xFFFF
        signed = self.angle - 0x10000 if self.angle >= 0x8000 else self.angle
        payload = (struct.pack("<Ihh", self.tick, signed, 0) + bytes((1, 0)))
        self.tick = (self.tick + 30) & 0xFFFFFFFF
        return payload


class FakePoweredMCP:
    def __init__(self):
        self.commands = []
        self.ramps = []
        self.running = False
        self.async_configured = False
        self.async_disabled = False
        self.aspep = FakeAsyncEndpoint(self)

    def command(self, command, timeout=1.0):
        self.commands.append(command)
        if command == MCP_START:
            self.running = True
        elif command == MCP_STOP:
            self.running = False

    def get_registers(self, registers):
        values = []
        for register, _ in registers:
            if register == REG_STATUS:
                values.append(6 if self.running else 0)
            elif register == REG_FAULTS_FLAGS:
                values.append(0)
            elif register == REG_SPEED_MEAS:
                values.append(500 if self.running else 0)
            elif register == REG_I_Q_MEAS:
                values.append(100 if self.running else 0)
            else:
                raise AssertionError(f"unexpected register 0x{register:04X}")
        return values

    def get_register(self, register, fmt):
        return 3

    def configure_async_hall(self, **kwargs):
        self.async_configured = True

    def disable_async(self):
        self.async_disabled = True

    def set_speed_ramp(self, rpm, duration_ms):
        self.ramps.append((rpm, duration_ms))


class DroppingStateMCP(FakePoweredMCP):
    def __init__(self):
        super().__init__()
        self.running_status_reads = 0

    def get_registers(self, registers):
        values = super().get_registers(registers)
        if self.running:
            self.running_status_reads += 1
            if self.running_status_reads >= 2:
                for index, (register, _) in enumerate(registers):
                    if register == REG_STATUS:
                        values[index] = 0
        return values


if __name__ == "__main__":
    unittest.main()
