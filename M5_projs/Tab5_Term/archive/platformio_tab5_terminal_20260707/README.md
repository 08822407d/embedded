# Tab5 UART Terminal

This PlatformIO project turns M5Stack Tab5 / ESP32-P4 into a small physical
terminal for an external Linux board login UART. It is moving from a UART
viewer toward a practical terminal emulator in staged compatibility layers.

Current firmware includes a character-cell terminal core with C0 handling,
ESC/CSI parsing, cursor movement, clear screen/line, DEC Special Graphics,
VT102-style insert/delete operations, scroll regions, origin mode, delayed
autowrap, basic terminal query replies, SGR attributes, 16-color/256-color/
truecolor foreground/background colors, and background-color erase behavior. It
is not yet a complete `xterm-256color` implementation.

See [docs/terminal_implementation_plan.md](docs/terminal_implementation_plan.md)
and [docs/terminal_validation.md](docs/terminal_validation.md) before extending
terminal compatibility.

For a source-level overview, module map, and extension guidelines, read
[docs/developer_architecture.md](docs/developer_architecture.md).

Current mainline stage and checkpoint are recorded in
[docs/current_work.md](docs/current_work.md). Read it before continuing
implementation so completed work is not repeated or lost.

Stages 1 through 9 are complete at the documented validation levels. Stage 5
official A164 keyboard and integration work and Stage 6 regression
infrastructure were accepted on 2026-06-12; Stage 7 Unicode width/graphics,
Stage 8 protocol-readiness, and Stage 9 TUI/input compatibility hardening were
completed afterward.
The deterministic Stage 1-4 machine assertions and real-shell application
smoke are now automated; see [docs/stage6_regression.md](docs/stage6_regression.md).
Exact final display pixels can also be captured and compared through a
debug-only USB CDC path; see [docs/screen_capture.md](docs/screen_capture.md).

The default display orientation is rotated 180 degrees from the original
landscape direction to match the official companion keyboard installation.
Change `SCREEN_ORIENTATION` in `include/app_config.h` to restore the standard
landscape direction.

## Hardware

Default wiring:

- For M5Stack Module LLM `ttyS1` / M5-Bus `TRM_TXD/TRM_RXD`, the tested
  Tab5 pins are `RX=G7` and `TX=G6` (M-Bus pin 15/16).
- External Linux board TX -> Tab5 RX pin.
- External Linux board GND <-> Tab5 GND
- Tab5 TX is only required for bidirectional shell/login testing.

If bidirectional input is needed later:

- Tab5 TX pin -> external device RX

The earlier generic G38/G37 assumption did not match the Module LLM `ttyS1`
M5-Bus login path on the tested Tab5 + Module LLM stack.

Default UART settings:

- 115200 8N1
- 3.3 V TTL UART only

Do not connect RS-232 level signals or 5 V TTL signals directly to Tab5 GPIO.

USB serial is used only for flashing and debug logs. It is not mixed with the
external login UART.

## Current Verified Status

- PlatformIO env: `tab5_min_uart_terminal`
- Terminal CDC injection debug env: `tab5_terminal_cdc_inject`
- Optional keyboard probe env: `tab5_usb_keyboard_probe`
- Current login UART pins: `RX=G7`, `TX=G6`
- Current installed login UART baud: `921600 8N1`
- USB debug/flash port on the first development machine was `/dev/ttyACM0`
  (`VID:PID 303A:1001`, Espressif USB JTAG/serial). Treat this as a local
  observation, not a fixed upload port.
- The USB monitor bridge is enabled. Typing in `pio device monitor` is forwarded
  to the login UART, and bytes received from the login UART are echoed back to
  the monitor and rendered on the Tab5 display.
- The Module LLM login shell was verified by sending `id` through the Tab5
  bridge and receiving a root shell response.
- Terminal output flows through `src/terminal_core.cpp`, not directly through
  M5GFX print calls.
- USB-A keyboard input is enabled in the formal firmware and shares the same
  normalized input mapper as the official A164 keyboard. The historical
  `tab5_usb_keyboard_probe` environment remains available for USB-only
  isolation.
- The formal firmware now enables the official A164 Tab5 Keyboard through
  Ext.Port1 (`SDA=G0`, `SCL=G1`, `INT=G50`) using the official
  `M5Unit-KEYBOARD` 0.1.0 library in HID mode. The tested keyboard reports I2C
  address `0x6D` and firmware `0x01`.
- The top status bar shows a themed battery area at the right edge. On Tab5 this
  uses M5Unified's INA226-backed 2S Li-Po battery estimate. The battery icon
  fill uses green/yellow/red level bands. The lightning icon uses a debounced
  INA226-current heuristic because the observed IP2326 `CHG_STAT` /
  `M5.Power.isCharging()` path did not reliably follow cable insertion and
  removal on the tested unit.

## Build

For routine work on Windows, use the local wrapper. Detailed compiler output is
saved under `.logs/`, while the console only shows a short result summary:

```powershell
.\tools\tab5.ps1 build tab5_min_uart_terminal
```

Long builds run in a detached worker with direct log files. If the caller stops
waiting, inspect or rejoin the same build without starting a duplicate:

```powershell
.\tools\tab5.ps1 build-status tab5_min_uart_terminal
.\tools\tab5.ps1 build-wait tab5_min_uart_terminal
```

See `docs/build_troubleshooting.md` for the persistent incident log and
evidence-based triage procedure.

The equivalent direct PlatformIO command is:

```sh
pio run -e tab5_min_uart_terminal
```

Experimental USB-A keyboard input build:

```sh
pio run -e tab5_usb_keyboard_probe
```

Terminal parser validation build. In this mode USB CDC input is rendered
directly by the terminal core and is not forwarded to the external login UART:

```sh
pio run -e tab5_terminal_cdc_inject
```

## Login UART Baud

The installed formal firmware can coordinate the Tab5 login UART and Module LLM
`/dev/ttyS1` without rebuilding:

```powershell
.\tools\tab5.ps1 baud -Port COM3
.\tools\tab5.ps1 baud -Port COM3 -Baud 921600
.\tools\tab5.ps1 baud -Port COM3 -DefaultBaud
```

Supported rates are 115200, 230400, 460800, and 921600. Runtime 460800 and
921600 shell round trips are hardware-validated. Persistence is optional and
must be coordinated with the Linux boot-time TTY configuration. See
`docs/login_uart_baud.md`.

## List Ports

```sh
pio device list
python -m serial.tools.list_ports -v
```

## Upload

The local wrapper checks that the selected COM port and build artifacts exist,
then records the complete esptool output under `.logs/`:

```powershell
.\tools\tab5.ps1 flash tab5_min_uart_terminal -Port COM3
```

Do not permanently hard-code `upload_port` in `platformio.ini`. Pick the Tab5
USB debug port from the port list and upload with:

```sh
pio run -e tab5_min_uart_terminal -t upload --upload-port <PORT>
```

For the experimental USB keyboard probe, use the same upload command with
`-e tab5_usb_keyboard_probe`.

For terminal parser validation, use the same upload command with
`-e tab5_terminal_cdc_inject`.

If upload fails, put the Tab5 into download mode and retry. Do not assume upload
succeeded unless PlatformIO reports success.

## Monitor

```sh
pio device monitor --port <PORT> --baud 115200
```

The monitor shows USB debug logs and a copy of received UART bytes/text so you
can confirm that external UART data is arriving.

With the current bridge build, the monitor can also be used as a temporary
keyboard path:

```sh
pio device monitor --port <PORT> --baud 115200
```

After opening the monitor, type a shell command such as `id` and press Enter.
The command is forwarded from USB debug CDC to the Module LLM login UART.

With the `tab5_terminal_cdc_inject` debug build, USB CDC input is instead sent
directly to the Tab5 terminal renderer. This is for replaying exact terminal
byte streams; command text is not executed by the external Linux board in that
mode.

For a fixed parser smoke test against that debug build:

```sh
python tools/send_terminal_test.py --port COM3 --test stage1-smoke
```

For the VT102 screen-model smoke test:

```sh
python tools/send_terminal_test.py --port COM3 --test stage2-screen --chunk-size 16 --chunk-delay 0.08
```

For the color/attribute smoke test:

```sh
python tools/send_terminal_test.py --port COM3 --test stage3-color --chunk-size 16 --chunk-delay 0.08
```

For the xterm/DEC essentials smoke test:

```sh
python tools/send_terminal_test.py --port COM3 --test stage4-xterm --chunk-size 16 --chunk-delay 0.08 --read-response-window 0.8
```

For the Unicode width/cell-integrity smoke test:

```sh
python tools/send_terminal_test.py --port COM3 --test stage7-unicode
```

For the Unicode graphics fallback smoke test:

```sh
python tools/send_terminal_test.py --port COM3 --test stage7-unicode-graphics
```

To capture the exact final framebuffer from `tab5_terminal_regression` or
`tab5_screen_capture`:

```powershell
.\tools\tab5.ps1 screenshot -Port COM3 `
  -OutputPath .logs\screenshots\current.png
```

Add `-Baseline .logs\screenshots\accepted.rgb565` for an automatic pixel
comparison. The normal `tab5_min_uart_terminal` firmware keeps this private
debug command disabled.

The current core uses fixed Unicode 15.0.0 column-width data, stores CJK-wide
glyphs as lead/continuation pairs, and attaches common combining marks without
advancing the cursor. Comprehensive glyph coverage and complex text shaping
remain separate future work.

For a real login-shell SGR demo through the formal UART bridge:

```sh
python tools/send_login_shell_demo.py --port COM3 --demo sgr
```

For the short login-shell connectivity probe:

```powershell
.\tools\tab5.ps1 probe -Port COM3
```

To capture concise startup diagnostics, including official keyboard detection:

```powershell
.\tools\tab5.ps1 boot-log -Port COM3
```

For Stage 4 real app smoke through the formal UART bridge:

```sh
python tools/send_login_shell_app_smoke.py --port COM3 --apps clear,reset,tput,less,nano,vim,htop --rows 32 --cols 64 --chunk-size 32 --chunk-delay 0.08
```

The same test can be run with concise logging:

```powershell
.\tools\tab5.ps1 app-smoke -Port COM3
```

For Stage 10 render/receive latency comparisons through the formal UART bridge:

```powershell
.\tools\tab5.ps1 render-latency -Port COM3 -BurstLines 64 -LineWidth 64
```

The command writes detailed raw and JSON output under `.logs\render-latency-*`.
The wrapper defaults to temporarily disabling the login-UART CDC mirror and
polling `TAB5PIPE` counters, because heavy CDC mirror traffic can perturb the
ESP32-P4 Arduino `HWCDC` path. Pass `-EnableMirror` only when checking the CDC
mirror byte stream itself. Current JSON output includes mirror mode, pipeline
timing, optional exact-line assertions, expected byte counts, `TAB5PIPE`
receive/render/mirror counters, actual UART RX buffer size, and UART driver
error counters. The current Stage 10 P6 `240x64` mirror-disabled metric is
about `1.1s` to pipeline quiescence; this is the accepted Stage 10 close-out
baseline for future comparisons.

The normal `tab5_min_uart_terminal` now uses fixed 18x20 cells to avoid
whole-row redraws for every received character. It keeps the approved
M5GFX `DejaVu18` font face; only character positioning changes. Use
`tab5_terminal_font_prop_preview` when the retained true-proportional renderer
must be inspected. Fixed-cell rows prefill background-color runs before drawing
glyphs, then draw text transparently over that prefilled background. This helps
common shell/TUI output. During batched `terminal::writeBytes()` spans, hardware scroll is
also deferred and the final visible rows are redrawn once at the end of the
batch.
When the login UART software RX ring still has backlog, the formal firmware
keeps a terminal write transaction open and delays physical row flushes until
the backlog clears or a short timeout is reached. This avoids dozens of
redundant scroll-region redraws during large shell bursts.

## Continue On Another Machine

See [HANDOFF.md](HANDOFF.md) first. It is the canonical project memory for
future Codex sessions and machine handoffs, including confirmed design
decisions, known local toolchain traps, tested hardware wiring, UI details, and
next planned work.

When committing from this checkout, note that this project lives inside a larger
git working tree. Scope commits to this directory unless intentionally syncing
the sibling projects too, for example:

```sh
git status -- M5_projs/Tab5_Term
git add M5_projs/Tab5_Term
git commit -m "Add Tab5 UART terminal handoff notes"
git push origin master
```

## Troubleshooting

If the screen shows nothing, first confirm that M5Unified and M5GFX were pulled
from their latest GitHub versions as configured in `platformio.ini`.

If the external board output does not appear on screen, verify:

- External TX is connected to Tab5 `G7` for the tested Module LLM `ttyS1`
  M5-Bus login path.
- Grounds are connected.
- The external UART is 3.3 V TTL, not RS-232 or 5 V TTL.
- The external UART is configured for 115200 8N1.
