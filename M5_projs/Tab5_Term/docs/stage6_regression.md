# Stage 6 Regression Harness

Stage 6 protects the accepted terminal baseline while compatibility work
continues. It reuses the existing deterministic corpora and login-shell smoke
scripts instead of creating parallel test payloads.

## Accepted Baseline

- Firmware display: DejaVu18, fixed 18x20 cells, `64x32`.
- Display direction: keyboard-mounted landscape.
- Login UART: 921600 8N1 on both Tab5 and Module LLM.
- Linux terminal identity: `xterm-256color`, `COLORTERM=truecolor`.
- Production input: official A164 keyboard.
- USB keyboard: deferred and excluded from Stage 6.

The machine-readable baseline and deterministic expectations are stored in
`tests/terminal_regression_manifest.json`.

## Test Classes

| Class | Firmware | Command | Pass criterion |
| --- | --- | --- | --- |
| Deterministic screen state | `tab5_terminal_regression` | `tools/tab5.ps1 terminal-regression` | Manifest state, row, and cell assertions pass |
| Final framebuffer pixels | `tab5_terminal_regression` or `tab5_screen_capture` | `tools/tab5.ps1 screenshot` | CRC-valid PNG/raw capture; optional pixel comparison passes |
| Existing visual corpus | `tab5_terminal_cdc_inject` | `tools/send_terminal_test.py` | Manual display review where still listed |
| Login-shell probe | `tab5_min_uart_terminal` | `tools/tab5.ps1 probe` | Host marker executes through Tab5 |
| Real applications | `tab5_min_uart_terminal` | `tools/tab5.ps1 app-smoke` | Available app markers return successfully |
| Physical integration | `tab5_min_uart_terminal` | Manual | Status bar, A164, UART, and battery remain responsive |

## Diagnostic Protocol

The `tab5_terminal_regression` environment enables private OSC 777 commands on
USB CDC. They are intercepted before terminal parsing and are not compiled into
the formal firmware behavior.

Commands:

```text
ESC ] 777 ; terminal-state? BEL
ESC ] 777 ; terminal-row=<zero-based-row>? BEL
ESC ] 777 ; terminal-cell=<zero-based-row>,<zero-based-column>? BEL
ESC ] 777 ; screen-capture? BEL
```

Responses begin with `TAB5SNAP`, `TAB5ROW`, or `TAB5CELL`. Row text is UTF-8
encoded as hexadecimal so control bytes cannot corrupt the response framing.
FNV-1a row and buffer hashes include text, DEC charset state, foreground,
background, attributes, and cell width. `TAB5CELL` reports width as `0`
continuation, `1` single, or `2` wide lead.

The host runner sends each state/row/cell query only after receiving the
previous reply. A batched implementation was rejected after hardware runs
showed that the final CDC request could be dropped while the device was
simultaneously printing many diagnostic responses. The runner now retries
read-only OSC 777 diagnostic queries a small number of times before failing a
case; this handles occasional CDC reply drops without changing terminal state.

The screen-capture command returns the current logical M5GFX framebuffer as
RGB565LE with a CRC32. It is also available in the production-like
`tab5_screen_capture` environment so a real login-shell screen can be captured.
The normal `tab5_min_uart_terminal` firmware leaves it disabled. See
`screen_capture.md` for the protocol, limitations, and pixel-comparison
workflow.

## Deterministic Run

Build and flash the regression firmware:

```powershell
.\tools\tab5.ps1 build tab5_terminal_regression
.\tools\tab5.ps1 flash tab5_terminal_regression -Port COM3
```

Run all manifest cases:

```powershell
.\tools\tab5.ps1 terminal-regression -Port COM3
```

Detailed raw serial output is saved under `.logs/`; the console reports only
per-case pass/fail results.

Run the complete automated workflow, including restoration of the formal
firmware, login-shell probe, and real-application smoke:

```powershell
.\tools\run_stage6_regression.ps1 -Port COM3
```

Use `-SkipBuild` only when both firmware artifacts are already current. The
script restores `tab5_min_uart_terminal` in a `finally` block after the
deterministic diagnostic run.

Hardware validation on 2026-06-12 passed this complete workflow with cached
artifacts. It finished with the formal firmware installed, 5/5 deterministic
cases passing, the Module LLM shell probe passing, and all installed app-smoke
targets passing. The fifth corpus is the Stage 7 U1/U2 Unicode-width test.

Do not update expected values merely to make a failure pass. First determine
whether the implementation changed intentionally, the expectation was wrong,
or a real regression occurred.

Known unsupported or incomplete behavior is maintained in
`terminal_known_gaps.md`.

Framebuffer-capture validation on 2026-06-12 produced repeatable 1280x720
captures. Two captures of the same Stage 7 screen had zero different pixels,
and a production-like build captured the real Module LLM login shell before the
formal firmware was restored.
