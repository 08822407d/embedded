# Power Detect Probe

This diagnostic workflow searches for a reliable Tab5 external-power or
charging-state signal with minimal manual observation.

The original `M5.Power.isCharging()` path is not trusted as the sole source for
Tab5. M5Unified implements it through IO expander `E2.P6/CHG_STAT`, and user
testing showed this value staying high across repeated cable insert/remove
cycles. The probe therefore records that signal only as one candidate alongside
INA226 current and voltage.

## Firmware

PlatformIO environment:

```powershell
.\tools\tab5.ps1 build tab5_power_detect_probe
.\tools\tab5.ps1 flash tab5_power_detect_probe -Port COM3
```

The probe firmware:

- samples every 500 ms;
- stores samples in a 4096-entry RAM ring buffer, about 34 minutes of history;
- records sample sequence, `millis()`, `M5.Power.isCharging()`,
  raw `E2.P6/CHG_STAT`, USB CDC connection state, INA226 battery current,
  INA226 battery voltage, and battery percentage;
- keeps the login UART bridge, Tab5 keyboard, and formal status-bar battery
  polling disabled;
- continues recording while USB CDC is disconnected, as long as the Tab5
  remains powered from its battery or another source.

CDC commands accepted by the probe firmware:

```text
PWRLOG?
PWRLOG STATUS
PWRLOG DUMP
PWRLOG CLEAR
PWRLOG HELP
```

`PWRLOG DUMP` returns a CSV block between `PWRLOG BEGIN` and `PWRLOG END`.

## Host Workflow

Run:

```powershell
.\tools\tab5.ps1 power-detect -Port COM3 -DurationSeconds 180
```

After the command starts, manually plug and unplug the power/data cable or the
charging cable several times during the capture window. If the USB CDC port
disappears, the host script waits for it to return. At the end of the window it
reopens the port, dumps the ring buffer, and analyzes the data.

Output files are written under `.logs`:

- `power-detect-*.serial.log`: live CDC transcript captured during the window.
- `power-detect-*.dump.log`: raw `PWRLOG DUMP` output.
- `power-detect-*.csv`: parsed samples.
- `power-detect-*.analysis.json`: automatic candidate-rule analysis.
- `power-detect-*.log`: console log from `tools/tab5.ps1`.

## Analysis Criteria

The host script automatically reports:

- whether `isCharging()` changed;
- whether raw `CHG_STAT` changed;
- whether USB CDC connected/disconnected samples were recorded;
- current and voltage min/max/mean/median/stdev;
- current statistics grouped by API state, raw pin state, and USB state;
- a two-cluster current split, if present;
- a candidate current threshold and confidence level when the split is strong
  enough.

A production rule should not be added until the log shows a stable signal.
Expected useful outcomes:

- best case: a hardware/API state changes cleanly with cable insertion;
- likely case: INA226 current forms two well-separated clusters, allowing a
  debounced threshold/hesis rule;
- failure case: `CHG_STAT` remains stuck and current/voltage do not separate
  enough, meaning firmware cannot reliably infer the state without a different
  hardware signal.

## Captured Runs

2026-06-15, `.logs/power-detect-20260615-143030.*`:

- 361 samples over 180 seconds, no dropped samples.
- User unplugged USB data, cycled the charge cable five times, then reconnected
  USB data.
- `api` and raw `CHG_STAT` both changed only once near the beginning for 14
  samples and did not follow the repeated charge-cable cycles.
- INA226 current formed two clear clusters:
  - no external power: 46 samples around `-979mA`;
  - external power / charging: 315 samples around `+199mA`.
- Host analysis proposed `current_ma > -389.9` as a high-confidence
  external-power threshold.

This run supports a future debounced INA226-current heuristic. It is not yet an
accepted production rule for the status-bar lightning icon.

## Restore

After probing, restore the formal terminal firmware:

```powershell
.\tools\tab5.ps1 build tab5_min_uart_terminal
.\tools\tab5.ps1 flash tab5_min_uart_terminal -Port COM3
.\tools\tab5.ps1 probe -Port COM3
```
