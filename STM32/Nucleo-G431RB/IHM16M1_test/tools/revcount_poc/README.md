# revcount_poc

PC-side revolution-counting proof of concept for the existing GM16020-06 Hall FOC firmware. It implements the MCSDK 6.4.2 MCP/ASPEP protocol over the ST-LINK virtual COM port at 1843200 baud. It does not flash, regenerate, or modify firmware.

## Safety boundary

- `probe` and `hand-turn` issue MCP `GET` requests only. They cannot send Ack, Start, Stop, Ramp, `SET`, or asynchronous-log configuration. Use these modes only with the motor bus physically disconnected and the Nucleo powered by USB.
- `run` can drive the motor. It refuses to start unless all three flags are present: `--allow-motor-control --confirm-bus-powered --confirm-user-present`. These flags do not replace the task-book HITL-POWER authorization, CC presence, physical restraint, or one-second bus disconnect capability.
- The firmware's 0.3 A Iq limit is software control saturation, not short-circuit protection. The current setup has no fuse.
- MotorPilot and this tool are mutually exclusive serial clients. Close MotorPilot first.

## Requirements

- Windows and Python 3.10 or newer. Tested offline with Python 3.12.2.
- No third-party Python packages. Serial I/O uses the Win32 API through `ctypes`.
- The default board lookup resolves the VCP from ST-LINK SN `002A00403234510E33353533`; it does not hard-code a COM number.

## Offline tests

From this directory:

```powershell
python -m unittest discover -s tests -v
```

Tests use synthetic angle streams and an in-memory fragmented serial endpoint. They do not enumerate or open hardware.

## USB-only read-only commands

One-shot status probe:

```powershell
python revcount_poc.py probe --port auto
```

Timed hand-turn trial. Align a pointer before launch, manually turn exactly four mechanical revolutions during the 20-second window, then hold the final alignment:

```powershell
python revcount_poc.py hand-turn --turns 4 --direction positive --duration 20
python revcount_poc.py hand-turn --turns 4 --direction negative --duration 20
```

The direction names mean the sign reported by `HALL_EL_ANGLE`, not an assumed physical CW/CCW direction. If the first result has the opposite sign, relabel the physical direction; do not change motor wiring for this test.

`hand-turn` polls angle, STATUS, FAULTS, and speed in one GET transaction. It records every sample to `logs/`, reports worst-case and mean sample rates, and invalidates the trial if a host timing gap could hide half an electrical turn at `--design-rpm` (default 1000 rpm). For four pole pairs, the required 4x-margin rates are 133.3 Hz at 500 rpm and 266.7 Hz at 1000 rpm.

## Powered command

Do not run this command until a new HITL-POWER authorization has been granted for the specific test session:

```powershell
python revcount_poc.py run --turns 5 --rpm 500 --lead 0 `
  --stop-method ramp --stop-ramp-ms 1500 `
  --allow-motor-control --confirm-bus-powered --confirm-user-present
```

The powered path performs Fault Ack, verifies IDLE and speed mode, subscribes to `HALL_EL_ANGLE` plus `HALL_SPEED` at 1 kHz device rate, sends Start and a speed ramp, triggers stopping at `turns - lead`, then requires IDLE with no current fault and stable speed/angle before accepting the result. It stops and invalidates on an unexpected state transition, wrong direction, current saturation, sample loss, or motion timeout. `--stop-method direct` is the comparison path. The PoC enforces `abs(rpm) <= 1000` and `stop-ramp-ms` between 1000 and 2000 ms.

On audible squeal, violent vibration, smell, motor movement, sustained current saturation, wrong speed sign, or repeated fault: Stop and disconnect the motor bus immediately, preserve the CSV, and return the result to CC.

## Counting and loss detection

`HALL_EL_ANGLE` is a signed 16-bit electrical angle. Adjacent values are unwrapped with the shortest modular delta. Four electrical revolutions equal one mechanical revolution:

```text
mechanical turns = accumulated electrical counts / (4 * 65536)
```

A modular value cannot by itself reveal that one or more complete wraps were skipped. The tool therefore also validates time continuity. In asynchronous mode the 30 kHz device timestamp must advance by exactly 30 ticks between 1 kHz samples; in polling mode any host interval that admits 32768 counts at the configured design speed makes the entire trial invalid. An exact half-revolution adjacent delta is also invalid rather than guessed.

Runtime CSV/JSON files under `logs/` are intentionally ignored. Preserve evidence needed for review by referencing its path and summarizing statistics in `docs/REVCOUNT_POC.md`; raw selected evidence can be explicitly force-added if CC requests it.
