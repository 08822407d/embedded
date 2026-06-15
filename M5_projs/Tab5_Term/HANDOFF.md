# Tab5 UART Terminal Handoff

This is the canonical project memory for future Codex sessions and machine
handoffs. Read this file before rereading large parts of the source tree.

Keep this document current whenever a hardware fact, architecture decision,
toolchain workaround, tested command, or UI behavior is confirmed. The goal is
to preserve decisions and traps even if a long Codex session is compacted.

## Codex Working Rules

- Prefer repo-local scripts for repeatable, deterministic work that would
  otherwise produce long commands or large tool output in the conversation.
  Typical candidates are builds, flashing, serial tests, log filtering, and
  regression replays. Store full output locally and report only the useful
  summary. Do not create a script for a one-off task that is clearer as a short
  command, and do not replace architecture analysis or code review with opaque
  automation.
- As the project grows, Codex must extend the project memory and handoff format
  when the existing structure no longer captures enough context. This includes
  adding sections such as decision logs, module boundaries, validation matrices,
  hardware notes, or troubleshooting records when they become useful.
- Do not wait for a separate documentation request when a task establishes a
  stable design decision, discovers a hardware fact, or exposes a repeatable
  trap. Update `HANDOFF.md` as part of that work.
- Groundwork that only prepares future advanced features should normally avoid
  hardware flashing or live-device validation. Implement the narrow internal
  piece, document its behavior and intended future consumer, and mark the item
  as implemented but unverified until the advanced feature that uses it is
  developed and validated. Keep live hardware validation for user-visible
  behavior, formal firmware changes, regression-risky changes, and work that
  directly affects the current UART login path.
- If Codex cannot confidently judge whether the documentation structure needs
  expansion, the user may periodically ask for a documentation-structure
  analysis. Record the resulting changes in this file.

## Project Goal

Turn the M5Stack Tab5 into a standalone physical terminal for an external Linux
board, roughly in the VT100 family of behavior. The current firmware is still a
minimal UART viewer/bridge with early UI and input plumbing; it is not yet a
full terminal emulator.

Near-term priorities:

- Keep the verified login UART path stable.
- Grow terminal behavior deliberately: parsing, screen model, keyboard input,
  and UI/status rendering should become separate modules instead of expanding
  `src/main.cpp`.
- Preserve the USB debug bridge while standalone keyboard input is still
  experimental.
- Treat visual/status-bar details as user-tuned behavior unless intentionally
  changed.

## Repository Scope

This project directory is:

```text
M5_projs/Tab5_Term
```

The git repository root on the first development machine was:

```text
/home/cheyh/projs/embedded
```

There were unrelated dirty files in sibling directories. When committing or
pushing this work, scope the commit to `M5_projs/Tab5_Term` unless those sibling
changes are intentionally part of the same sync.

## Source Map

- `include/app_config.h`: compile-time hardware and feature settings. Current
  status bar height and terminal text size live here.
- `src/main.cpp`: startup, UART read/display loop, USB monitor bridge, optional
  USB keyboard queue drain, and periodic `M5.update()`. Keep this file as the
  coordinator, not the terminal emulator implementation.
- `include/ui_theme.h` / `src/ui_theme.cpp`: terminal theme and color palette.
  Add future color schemes here.
- `include/display_orientation.h` / `src/display_orientation.cpp`: maps
  semantic screen orientations to M5GFX rotation values. The configured
  keyboard-mounted orientation is a 180-degree turn from the original
  landscape direction.
- `include/status_bar.h` / `src/status_bar.cpp`: status bar drawing, battery
  polling, battery icon, and battery area layout.
- `include/terminal_core.h` / `src/terminal_core.cpp`: first-stage
  character-cell terminal core, parser state machine, cursor/screen model,
  scrolling, erase operations, and basic SGR colors.
- `include/terminal_debug_input.h` / `src/terminal_debug_input.cpp`: debug-only
  USB CDC direct-injection input path for terminal parser validation.
- `include/screen_capture.h` / `src/screen_capture.cpp`: debug-only logical
  framebuffer readback. It streams the existing MIPI DSI RGB565 framebuffer
  one row at a time and does not allocate a second full-screen buffer.
- `include/input_mapper.h` / `src/input_mapper.cpp`: transport-independent key
  events and VT/xterm sequence mapping, including application cursor/keypad
  modes. Representative mappings are compile-time tested.
- `include/input_event_queue.h` / `src/input_event_queue.cpp`: shared
  nonblocking event queue for production input devices.
- `include/login_uart.h` / `src/login_uart.cpp`: owns the external login UART,
  supported runtime baud rates, delayed switching, and optional NVS state.
- `include/usb_management.h` / `src/usb_management.cpp`: intercepts private
  USB CDC management frames before normal CDC bytes reach the Linux shell.
- `include/tab5_keyboard_input.h` / `src/tab5_keyboard_input.cpp`: official
  A164 Tab5 Keyboard driver using M5Unit-KEYBOARD HID mode on G0/G1 with G50
  interrupt.
- `include/usb_keyboard_probe.h` / `src/usb_keyboard_probe.cpp`: experimental
  USB-A host keyboard probe. It is compile-time gated and should not be assumed
  production-ready until runtime-tested with a physical keyboard.
- `docs/terminal_implementation_plan.md`: staged compatibility plan from
  core ANSI/VT102 behavior toward a practical `xterm-256color` subset.
- `docs/terminal_validation.md`: manual validation checklist for each terminal
  compatibility stage.
- `docs/terminal_font_strategy.md`: terminal font decisions, fixed-grid
  constraints, built-in preview environments, and external font options.
- `docs/current_work.md`: active mainline stage, completed checkpoint, and
  current milestones. Read this before continuing implementation.
- `docs/build_troubleshooting.md`: persistent Windows build incident log,
  stall criteria, evidence checklist, and recovery procedure.
- `docs/screen_capture.md`: final-pixel capture protocol, host commands,
  comparison outputs, limitations, and validation record.
- `docs/login_uart_baud.md`: runtime login-UART management commands, endpoint
  coordination, persistence safety, recovery, and validated rates.
- `tools/send_terminal_test.py`: host-side helper that sends deterministic test
  byte streams to a `tab5_terminal_cdc_inject` firmware over USB CDC. It sends
  in small chunks by default because Tab5 renders while receiving and can lose
  late bytes if a long test stream is written all at once.

Expected future split:

- An ECMA-48/VT102/xterm-style parser should be a separate module.
- A screen buffer/model should be separate from raw display writes.
- Input mapping should stay separate from transport so USB monitor, USB-A
  keyboard, and any future BLE/network input can share terminal semantics.

## Mainline Stage

Stages 1 through 6 are complete. Stage 6, Test And Regression Harness, was
accepted on 2026-06-12 after its automated workflow passed and the user
reported no problem in the final physical checks. Stage 7 U1/U2 Unicode width
and cell integrity are complete. On 2026-06-15 the user approved continuing the
plan while skipping deferred work, so Stage 7 U3 added common Unicode
box-drawing and block-element renderer fallbacks. U3 passed hardware
regression and framebuffer capture, and the formal firmware was restored. The
canonical stage state is in `docs/current_work.md`.

Stage 5 completed on 2026-06-12 after the user reported no problem in the
remaining physical A164 and integration tests. Its accepted baseline includes
the 180-degree keyboard-mounted display orientation, `64x32` geometry,
official A164 input, and 921600 login UART.

USB keyboard support was
deferred indefinitely by explicit user request on 2026-06-12; its existing
probe should remain untouched unless the user explicitly resumes that work.
USB support is not a Stage 6 requirement.

Module LLM ttyS1 terminal-environment integration was completed on 2026-06-12.
The ttyS1-only login profile now survives reboot with
`TERM=xterm-256color`, `COLORTERM=truecolor`, and `stty rows 32 cols 64`;
`tput colors` reports 256 and the user visually confirmed colored `htop` on
Tab5. This is no longer a Stage 5 blocker.

Official Tab5 Keyboard facts confirmed on 2026-06-08:

- Product: A164 Tab5 Keyboard, connected through Tab5 Ext.Port1.
- Bus: I2C address `0x6D`, `SDA=G0`, `SCL=G1`, active-low `INT=G50`.
- Driver: official `M5Unit-KEYBOARD` release `0.1.0`.
- Selected mode: HID, because terminal navigation/function keys must retain HID
  identities before the shared mapper emits VT/xterm sequences.
- Official tutorial:
  `https://docs.m5stack.com/en/arduino/projects/tab5/tab5_keyboard`
- Official library:
  `https://github.com/m5stack/M5Unit-KEYBOARD`
- Verified device instance: firmware `0x01`; G50 falling-edge ISR configured.
- Physical input validation: the user typed
  `printf 'tab5-keyboard-ok\n'` on the A164, and the Module LLM shell displayed
  `tab5-keyboard-ok` on a separate line. This verifies printable characters,
  Enter, I2C event delivery, mapping, UART transmission, and shell execution.
  The user later reported no problem in the remaining practical input tests.
- The current A164 HID path tracks one non-modifier key through
  `last_hid_keycode`. Since the keyboard advertises simultaneous key presses,
  full non-modifier rollover remains a documented limitation and may need a
  pressed-key-state refactor in future input work.

The completed checkpoint before Stage 5 includes:

- terminal compatibility Stages 1 through 4;
- Stage 4 deterministic and real-login-shell app validation;
- acceptance of the DejaVu18 true-proportional release renderer;
- restoration and verification of the formal UART firmware on the board.

Stage 6 must reuse the existing deterministic corpora and real-shell smoke
scripts. Its main missing capability is a diagnostic-only machine-readable
screen-state summary plus host assertions, followed by a unified local
regression command and explicit known-gap record.

Stage 6 R1-R4 automated work was completed on 2026-06-12:

- `tab5_terminal_regression` provides diagnostic-only OSC 777 state queries.
- `tools/run_terminal_regression.py` and
  `tests/terminal_regression_manifest.json` passed all Stage 1-4 corpora on the
  physical Tab5.
- The formal image was restored and passed the Module LLM shell probe.
- Real-app smoke passed every installed target; `nano` remains absent.
- `tools/run_stage6_regression.ps1` repeats the complete diagnostic, restore,
  probe, and app-smoke workflow. Its cached-artifact path was exercised
  successfully on physical hardware and left the formal image installed.
- The final restored-firmware integration check passed by user observation.
- Charging-state detection remains explicitly deferred and was not a Stage 6
  completion requirement.
- A later Stage 6 test-infrastructure extension added private OSC 777
  framebuffer capture. `tab5_terminal_regression` captures deterministic
  injected screens, while `tab5_screen_capture` preserves the real login-UART
  path. The host generates PNG, raw RGB565, metadata, and optional pixel diffs.
  The ordinary formal firmware keeps the feature disabled.
- The first host implementation opened the USB port before disabling DTR/RTS,
  which reset Tab5 and captured a startup screen. Configure DTR/RTS on a closed
  pyserial object before opening it, as `tools/capture_screen.py` now does.

## Terminal Compatibility Target

Recommendation: aim for a pragmatic `xterm-256color`-compatible subset, built
in layers. Do not target plain VT100 as the final Linux terminal identity; it
is historically important but too small for modern Linux TUIs. Also do not
claim full xterm compatibility until the implemented sequence set and input
behavior are tested.

Rationale:

- Modern Linux console behavior is broadly described as a VT102 plus
  ECMA-48/ISO 6429/ANSI-control subset, with private extensions.
- Most terminal emulators and many remote shells use xterm-family behavior as
  the compatibility center. `xterm-256color` is a practical target because it
  gives common applications cursor addressing, alternate screen, attributes,
  and 256-color SGR without needing a custom terminfo entry on ordinary Linux
  systems.
- Use terminfo names as contracts. If the firmware cannot yet satisfy
  `xterm-256color`, advertise something narrower during testing, such as
  `vt100`, `vt102`, `ansi`, or a project-specific terminfo entry.

Implementation layers:

1. Core screen model: fixed rows/columns, cursor, scroll region, wrap mode,
   clear-to-end, clear-screen, insert/delete line/character, save/restore
   cursor, tabs, CR/LF/BS/BEL.
2. Parser: byte-oriented state machine for C0 controls, ESC, CSI, OSC/DCS
   skipping, parameters, private markers such as `?`, and recovery on malformed
   sequences.
3. SGR attributes: reset, bold/dim, underline, inverse, foreground/background
   8-color, bright 16-color, then `38;5;n` / `48;5;n` 256-color.
4. DEC/xterm essentials: origin mode, cursor visibility, application cursor
   keys, bracketed paste ignored or tracked, alternate screen
   `?1049h/l`, and device/status replies only where useful.
5. Input: printable UTF-8 bytes, Enter/Backspace/Tab/Escape, arrows,
   Home/End/Page keys, function keys, Ctrl/Alt combinations, and optional
   application-keypad/application-cursor variants.
6. Validation: use terminfo-driven apps (`clear`, `reset`, `tput`, `nano`,
   `vim`, `less`, `htop`) plus a dedicated escape-sequence test corpus before
   raising the advertised terminal identity.

Terminal validation input strategy:

- Prefer USB CDC direct-injection for escape-sequence validation. PlatformIO
  env `tab5_terminal_cdc_inject` enables `ENABLE_TERMINAL_CDC_INJECTION=1`.
  In that mode, host-side bytes from the existing Tab5 USB serial port route
  directly to `terminal::writeByte()` instead of being forwarded to the
  external login UART. This is the lowest-complexity way for Codex or a host
  script to send deterministic test streams while the user only watches the
  Tab5 screen.
- CDC injection is a debug feature only. Keep it disabled in formal/standalone
  firmware so USB CDC bytes cannot interfere with physical keyboard input or
  accidentally send local test streams into terminal rendering.
- CDC injection is not a shell execution path. It replays bytes into the
  renderer. Use the default USB-to-login-UART bridge when the external Linux
  board must execute a real shell command and produce output.
- In CDC injection mode, login UART display/input is intentionally disabled so
  external Linux output cannot pollute deterministic parser tests.
- If a CDC injection smoke test shows the first rows but loses later rows near
  the DEC graphics sample, first lower the host-side send rate with
  `tools/send_terminal_test.py --chunk-size 16 --chunk-delay 0.08`. This is a
  CDC/rendering transport pressure symptom, not proof that the parser rejected
  those sequences.
- USB-A should not be treated as a simple UART. It is a USB Host port for USB
  devices and would require a USB CDC-ACM/vendor serial host driver stack to
  use a USB-to-serial dongle from the Tab5 side. Keep that as a later, higher
  complexity option.
- If a physical UART adapter is still wanted, add an auxiliary UART on exposed
  GPIO. Current login UART uses `G7/G6`. Good candidate validation pairs are
  M-Bus `G17/G52` or HY2.0-4P Port.A `G53/G54`; wire USB-UART TX to the
  configured Tab5 RX and share GND. Use only 3.3V TTL levels.

## Current Firmware State

- PlatformIO environment: `tab5_min_uart_terminal`
- Terminal validation environment: `tab5_terminal_cdc_inject`
- Experimental keyboard environment: `tab5_usb_keyboard_probe`
- Platform: `pioarduino/platform-espressif32` at tag `54.03.21`
- Framework: Arduino
- Board setting: `esp32-p4-evboard`
- Display stack: M5Unified / M5GFX
- Login UART: `HardwareSerial(1)` owned by `login_uart`
- Login UART pins currently configured in `include/app_config.h`:
  - RX: `G7`
  - TX: `G6`
  - Default baud: `115200`
  - Runtime rates: `115200`, `230400`, `460800`, `921600`
  - Format: `8N1`
- USB debug CDC remains separate from the external login UART.
- USB-to-login-UART bridge is currently enabled:
  - `ENABLE_USB_LOGIN_UART_BRIDGE=1`
  - `ENABLE_USB_BRIDGE_DEBUG_LOG=0`
- USB CDC direct terminal injection is compile-time gated:
  - default env keeps `ENABLE_TERMINAL_CDC_INJECTION=0`
  - `tab5_terminal_cdc_inject` builds with `ENABLE_TERMINAL_CDC_INJECTION=1`
  - when enabled, USB CDC input is not forwarded to the login UART
- USB-A keyboard probe is compile-time gated:
  - default env keeps `ENABLE_USB_KEYBOARD_PROBE=0`
  - `tab5_usb_keyboard_probe` builds with `ENABLE_USB_KEYBOARD_PROBE=1`
- The status bar renders a themed battery area at the right edge. For Tab5,
  `M5.Power.getBatteryLevel()` uses the board's INA226 reading and estimates
  level as a 2S Li-Po pack. `M5.Power.isCharging()` reads the IP2326 charge
  status through IO expander pin `E2.P6/CHG_STAT`; the battery icon shows a
  lightning mark while charging.
- UI colors and status bar rendering have been split out of `src/main.cpp` into
  `src/ui_theme.cpp` and `src/status_bar.cpp` so future terminal UI work does
  not accumulate in the main loop.
- Terminal rendering now flows through `terminal_core`, not direct M5GFX print
  calls in `src/main.cpp`. The core owns a fixed character grid below the
  status bar and implements C0 handling, ESC/CSI cursor/erase, OSC/DCS/APC/PM
  skipping, DEC Special Graphics line drawing, SO/SI G0/G1 selection, 8-bit C1
  CSI/OSC/DCS entry points, VT102-style insert/delete character and line,
  erase character, scroll regions, origin mode, delayed autowrap, cursor
  visibility, tab-stop set/clear, full terminal-area redraw, and SGR
  reset/basic/256/truecolor mapping to RGB565. It also has a response callback
  for basic terminal replies: DSR OK, CPR, and VT102 primary DA. It is still a
  pragmatic VT102-style subset, not full `xterm-256color`.
- Stage 3 SGR/color support now includes visible bold rendering, dim,
  underline, inverse, hidden, 8/16-color foreground/background, 256-color
  foreground/background, truecolor foreground/background mapped to RGB565, and
  background-color erase behavior. Erased cells keep the current foreground and
  background colors but clear text attributes such as underline, inverse, and
  hidden, so colored TUI backgrounds survive `EL/ED/ECH` without making erased
  spaces hidden or underlined.
- Stage 4 first-pass xterm/DEC essentials support now includes alternate screen
  buffer switching for `?47`, `?1047`, and `?1049`, cursor save/restore mode
  `?1048`, application cursor mode tracking `?1`, application keypad mode
  tracking via `ESC =` / `ESC >` and `?66`, bracketed paste mode tracking
  `?2004`, mouse/focus mode tracking or safe ignore for common private modes,
  primary DA, secondary DA, CPR, DEC private CPR, ESC `Z` DECID, and a UTF-8
  printable-cell path. UTF-8 is currently one codepoint per cell; CJK
  double-width layout is implemented for the Unicode 15.0.0 width table. The
  formal build renders ASCII with M5GFX `DejaVu18` in fixed 18x20 cells. The
  retained proportional preview uses actual glyph advances. The Stage 4 UTF-8
  validation glyphs (`é`, `Ω`, `中`) render through deterministic built-in 6x8
  bitmap fallbacks because the default GLCD font grouped UTF-8 bytes correctly
  but did not cover the sample glyphs reliably, and `efontCN_12` still did not
  provide stable mixed Latin-1/Greek/CJK coverage on the Tab5 screen. Stage 7
  U3 adds deterministic drawn fallbacks for common Unicode box-drawing and
  block-element glyphs. Other unsupported non-ASCII characters currently render
  as `?`.

## Status Bar And Battery UI Decisions

The status bar is part of the user-visible terminal chrome, not remote terminal
content.

Current fixed settings:

- `STATUS_BAR_HEIGHT=32`
- default terminal font: `DejaVu18`, `TERMINAL_TEXT_SIZE=1`,
  `TERMINAL_CELL_WIDTH=18`, `TERMINAL_CELL_HEIGHT=20`
- formal renderer: fixed-cell; logical terminal geometry is `64x32`, with one
  blank character row above and below the terminal area. On the current
  720-pixel display this is a centered 640-pixel content area with 24-pixel
  top and bottom margins below the 32-pixel status bar.
- Raw UART has no standard unsolicited window-size negotiation. Keep the
  Module LLM TTY at `stty rows 32 cols 64`; optional xterm `CSI 18 t` reporting
  alone does not make a normal serial shell update its TTY size.
- On 2026-06-15 the user explicitly approved implementing `CSI 18 t` only as
  future protocol readiness for mature login transports such as SSH, Telnet, or
  PTY-backed helpers. Do not use it as a raw UART login resize mechanism, and
  do not inject visible `stty` commands from firmware.
- T1 validation on 2026-06-15: `CSI 18 t` replies with `CSI 8;32;64t` in the
  `stage8-protocol` regression case. The complete regression run passed 7/7,
  then `tab5_min_uart_terminal` was restored and the shell probe returned
  `shell-path-ok: m5stack-LLM`.
- T2 on 2026-06-15 added `terminal::TerminalGeometry` and
  `terminal::getGeometry()` as the shared geometry source for future transport
  layers. The snapshot includes viewport origin/size, rendered grid size, cell
  size, and logical rows/columns. This is preparatory work for SSH/Telnet/PTY
  integration, not a raw UART resize mechanism. It is locally build-checked but
  not hardware-validated.
- Stage 8 is complete as of 2026-06-15. There is no active Stage 9 scope yet;
  define the next stage explicitly before starting new compatibility work.
- proportional renderer: retained in `tab5_terminal_font_prop_preview`
- Screen orientation: `SCREEN_ORIENTATION_KEYBOARD_MOUNTED`; this maps the
  original landscape rotation `1` to opposite landscape rotation `3`, matching
  the official companion keyboard installation direction.
- `display_orientation` owns this mapping. Do not scatter raw M5GFX rotation
  values through UI modules. Apply orientation before calculating display
  dimensions and drawing UI regions.
- Title text: white on green, left aligned.
- Battery area: black panel, right edge of the title bar, same height as the
  title bar.

Battery layout is intentionally grouped in `BatteryAreaLayout kBatteryArea` in
`src/status_bar.cpp`. Adjust that group instead of scattering constants.

Current battery area behavior:

- Area width: `124`
- Right inset: `0` so the black area reaches the screen's right edge.
- Percent text size: `2`
- Percent/icon gap: `6`
- Battery body: `42x20`
- Battery cap: `4x8`
- The percent text and full battery icon are centered as one content group
  inside the battery area.

Battery icon visual notes:

- The battery outline and fill use `fillSmoothRoundRect`.
- The outer body radius is `5`; inner body and fill track radii are `3`.
  Earlier attempts were visibly too angular, then too capsule-shaped. Preserve
  this balance unless deliberately retuning the icon.
- The fill color bands intentionally mimic iOS-style battery colors:
  - `<=20%`: red
  - `<=50%`: yellow
  - `>50%`: green
  - charging: green fill plus a lightning mark
- The lightning mark is now a filled polygon, not a rounded stroke path. It has
  a screen-background outline and a pale yellow body
  `rgb565(255, 226, 120)`. Earlier rounded-stroke versions looked too soft.

Tab5 battery facts confirmed from M5Unified/M5Stack sources:

- `M5.Power.getBatteryLevel()` is backed by INA226 voltage/current readings and
  estimates a 2S Li-Po battery percentage.
- `M5.Power.isCharging()` reads the IP2326 charger status through IO expander
  pin `E2.P6/CHG_STAT`.
- Use these APIs instead of guessing from raw ADC voltage unless a later board
  revision proves they are wrong.
- `PI4IOE5V6408` has IRQ support and M5GFX configures an interrupt mask for
  `E2.P6/CHG_STAT`, but current Tab5 public PinMap and local M5Unified/M5GFX
  sources do not expose a usable PI4IOE `nINT` line to ESP32-P4 GPIO. Do not
  build a fake interrupt path without confirming the schematic or a board
  revision.
- Current runtime strategy: poll only `isCharging()` every `500ms`, poll
  `getBatteryLevel()` every `10000ms`, and redraw the battery area only when
  the displayed battery level or charging state changes. This keeps start/stop
  charging latency below human-noticeable levels while avoiding high-frequency
  INA226 reads.
- User validation on 2026-06-04 showed `E2.P6/CHG_STAT` and
  `M5.Power.isCharging()` both stayed high through repeated USB data cable and
  charge cable insert/remove cycles: `p6=1 api=1 ev=0`, while `n` kept
  increasing. This confirms the firmware was polling and the board stayed
  powered, but this `CHG_STAT` path did not distinguish the user's external
  power transitions on the tested setup. Do not treat the stuck-high value as a
  verified plug/unplug signal.
- M5Unified configures the Tab5 INA226 in continuous shunt+bus mode during
  `M5.Power.begin()`. Reading `getBatteryCurrent()` / `getBatteryVoltage()` at
  the status bar cadence only fetches already sampled values over I2C; it is
  acceptable for diagnostics and likely acceptable for a future debounced
  external-power heuristic once the current sign and thresholds are measured on
  hardware.

Power status validation build:

- PlatformIO env `tab5_power_status_probe` enables
  `ENABLE_POWER_STATUS_DIAGNOSTICS=1`.
- The diagnostic build currently draws
  two compact lines in the green status bar. Line 1 is
  `p=<raw> a=<state> i=<mA> v=<mV> e=<changes> n=<samples>`. Line 2 is
  `w=<count> ai=<mA> r=<mA>..<mA> av=<mV> all=<mA>..<mA>`. `p` is raw
  `E2.P6/CHG_STAT`, `a` is the M5Unified `isCharging()` result, `i` is the
  current INA226 battery current, `v` is INA226 battery voltage, `e` increments
  on raw/API state changes after boot, and `n` increments on each 500ms
  charging-status sample. `w/ai/r/av` summarize the latest 10 samples
  (approximately 5 seconds); `all` is the observed current range since boot.
- This screen-side diagnostic is needed because unplugging the USB power/data
  cable removes the serial port. If `n` keeps incrementing after unplugging,
  the firmware is still alive and polling. If `n` resets after replugging, the
  board rebooted during the cable transition.
- If external power insertion does not change `p/a`, inspect the rolling
  `ai/r/av` values after holding each cable state for at least 5 seconds. A
  stable current sign/magnitude difference between cable-in and
  cable-out can be turned into a debounced lightning-icon heuristic. Do not
  choose the sign or threshold until the tested board reports real values in
  both states.
- User feedback on 2026-06-04: do not keep adding screen variables for manual
  observation. If power detection is revisited, implement a dedicated automatic
  debug recorder first. It should let the user only plug/unplug cables while
  firmware records timestamped state changes, current/voltage windows,
  extrema, and reboot/sample continuity; then present a compact summary on
  screen or over serial when available.
- On 2026-06-15 that automatic recorder was implemented as
  `tab5_power_detect_probe`. It samples every 500ms into a 4096-entry RAM ring
  buffer, logs API charging, raw `E2.P6/CHG_STAT`, USB CDC connection state,
  INA226 current/voltage, and battery percentage, then exports CSV over
  `PWRLOG DUMP`. `tools/power_detect_probe.py` and
  `tools/tab5.ps1 power-detect` automate COM-port disappear/reappear handling,
  dump collection, and threshold analysis. This is still diagnostic evidence
  gathering; do not change the production lightning icon until a captured log
  shows a stable rule.

Power detect probe workflow:

```powershell
.\tools\tab5.ps1 build tab5_power_detect_probe
.\tools\tab5.ps1 flash tab5_power_detect_probe -Port COM3
.\tools\tab5.ps1 power-detect -Port COM3 -DurationSeconds 180
```

See `docs/power_detect_probe.md` for output files and restore commands.

## Verified Hardware Path

The original generic assumption `G38/G37` did not match the tested Module LLM
M5-Bus login path. Cross-checking with Module LLM context and M5Unified pin
tables showed that Module LLM `ttyS1` / M5-Bus `TRM_TXD/TRM_RXD` maps to Tab5
M-Bus pin 15/16:

```text
Module LLM TX -> Tab5 G7  (Tab5 RX)
Module LLM RX -> Tab5 G6  (Tab5 TX, needed for bidirectional shell)
GND           -> GND
```

The Tab5 firmware was verified through the USB monitor bridge. Sending:

```sh
id
```

through the Tab5 USB monitor returned a root shell response from the Module LLM
login UART.

## New Machine Bootstrap

From a fresh checkout, run:

```sh
pio --version
pio device list
python -m serial.tools.list_ports -v
pio run -e tab5_min_uart_terminal
pio run -e tab5_usb_keyboard_probe
```

Do not permanently set `upload_port` in `platformio.ini`. Identify the Tab5 USB
debug/programming port on the new machine. On the first machine it appeared as:

```text
/dev/ttyACM0
VID:PID 303A:1001
Espressif USB JTAG/serial
```

That port name is machine-local and may differ after replugging or on another
host.

Upload and monitor:

```sh
pio run -e tab5_min_uart_terminal -t upload --upload-port <PORT>
pio device monitor --port <PORT> --baud 115200
```

For USB-A keyboard testing, upload the probe env instead:

```sh
pio run -e tab5_usb_keyboard_probe -t upload --upload-port <PORT>
pio device monitor --port <PORT> --baud 115200
```

If upload fails, put Tab5 into download mode and retry. Do not assume the board
was flashed unless PlatformIO reports a successful upload.

## Local Toolchain Notes

The first development machine needed local PlatformIO package repair before the
ESP32-P4 Arduino build succeeded:

- The RISC-V toolchain package was empty/broken and was repaired by installing
  `riscv32-esp-elf` through Espressif IDF tools and linking it into
  `~/.platformio/packages/toolchain-riscv32-esp`.
- Missing Python packages were installed locally for PlatformIO tooling:
  `rich-click`, `PyYAML`, `intelhex`, `rich`, `esp-idf-size`.

A new machine may not need this if PlatformIO downloads clean packages. If the
same build failures recur, check the PlatformIO package install before changing
project code or switching frameworks.

A later Windows checkout used:

```text
C:\Users\cheyh\.platformio\penv\Scripts\pio.exe
```

On that machine, `pio` was not on `PATH`. Builds needed this temporary PATH
prefix so `riscv32-esp-elf-g++` could be found:

```powershell
$env:PATH = "$env:USERPROFILE\.platformio\packages\toolchain-riscv32-esp\riscv32-esp-elf\bin;$env:PATH"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e tab5_min_uart_terminal
```

The same Windows checkout also needed:

- `toolchain-riscv32-esp` repaired by copying the populated
  `riscv32-esp-elf` directory into
  `%USERPROFILE%\.platformio\packages\toolchain-riscv32-esp`.
- PlatformIO virtualenv `click` pinned below 8.2, because `click 8.3.1`
  caused an `esptool.py` `ParamType.get_metavar()` error.
- For the first `tab5_usb_keyboard_probe` build, local `.pio/libdeps` from
  `tab5_min_uart_terminal` were reused after a transient GitHub TLS failure.
- PlatformIO upload may fail before flashing because it tries to download
  `mklittlefs` and the local proxy/TLS path can fail. In that case, build with
  PlatformIO and flash the generated images directly with `esptool.py`.

Direct flash command verified on the Windows checkout:

```powershell
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"
$python = "$env:USERPROFILE\.platformio\penv\Scripts\python.exe"
$esptool = "$env:USERPROFILE\.platformio\packages\tool-esptoolpy\esptool.py"
$bootApp0 = "$env:USERPROFILE\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
& $python $esptool --chip esp32p4 --port COM3 --baud 1500000 --before default-reset --after hard-reset write-flash -z 0x2000 .pio\build\tab5_min_uart_terminal\bootloader.bin 0x8000 .pio\build\tab5_min_uart_terminal\partitions.bin 0xe000 $bootApp0 0x10000 .pio\build\tab5_min_uart_terminal\firmware.bin
```

Routine Windows operations are now wrapped by `tools/tab5.ps1`. Full command
output is stored under ignored `.logs/`; successful commands only print a
compact summary. Prefer these commands in future Codex sessions:

```powershell
.\tools\tab5.ps1 build tab5_min_uart_terminal
.\tools\tab5.ps1 flash tab5_min_uart_terminal -Port COM3
.\tools\tab5.ps1 probe -Port COM3
.\tools\tab5.ps1 app-smoke -Port COM3
```

Builds now run in a detached worker whose stdout/stderr go directly to files.
This prevents a Codex/tool-call timeout from invalidating the compiler output
pipe. A timed-out caller can inspect or rejoin the same build:

```powershell
.\tools\tab5.ps1 build-status tab5_min_uart_terminal
.\tools\tab5.ps1 build-wait tab5_min_uart_terminal
```

The detailed triage method and incident history are maintained in
`docs/build_troubleshooting.md`; update that file rather than rediscovering the
same host-side symptoms in each session.

Runtime login-UART management:

```powershell
.\tools\tab5.ps1 baud -Port COM3
.\tools\tab5.ps1 baud -Port COM3 -Baud 921600
.\tools\tab5.ps1 baud -Port COM3 -DefaultBaud
```

The normal command coordinates Tab5 and `/dev/ttyS1`; `-Tab5Only` is a
recovery operation. `-Persist` saves only the Tab5 side, so it must not be used
as a permanent setting unless the Linux boot-time TTY rate is also changed.
See `docs/login_uart_baud.md`.

Current Windows board observation:

```text
Port: COM3
VID:PID: 303A:1001
MAC: 30:ed:a0:e2:e2:48
```

Treat COM3 as local state. Recheck ports after replugging or on another
machine.

Nonfatal build warning seen repeatedly:

```text
periph_ctrl.h: warning: invalid suffix on literal
```

This warning comes from ESP-IDF headers in the Arduino package and has not
blocked successful builds.

Windows build behavior:

- Each new PlatformIO environment can trigger a full rebuild of M5GFX,
  M5Unified, and Arduino framework objects. On the Windows checkout this took
  roughly 4 minutes for `tab5_terminal_cdc_inject` and
  `tab5_power_status_probe`, even with `-j 2`.
- Do not chain several first-time environment builds. Use the detached wrapper
  one environment at a time.
- A 2026-06-08 incident confirmed that foreground caller timeouts could leave
  compiler children blocked on an inherited output path. Direct detached log
  files allowed the same build to pass the repeatedly stuck source.
- The detached full build completed successfully in 518.64 seconds. Its next
  cached build completed through the new wrapper in 15.5 seconds, including a
  persistent state/result file and a working `build-status` query.
- Prefer `-j 2` normally. Use evidence from
  `docs/build_troubleshooting.md` before changing job count, deleting `.pio/`,
  or modifying source in response to a silent stall.

## Runtime Login UART Baud

Implemented and hardware-validated on 2026-06-08:

- `HardwareSerial::updateBaudRate()` changes UART1 without reinitializing the
  display, keyboard, or terminal core.
- The active rate is shown in the status title.
- A private USB CDC management frame is consumed locally and never forwarded
  to the Linux shell.
- The host tool coordinates both endpoints and uses `exec bash -l` after
  `stty`; an earlier background-`stty` approach failed because the interactive
  shell restored its saved 115200 termios state.
- Real `shell-path-ok: m5stack-LLM` round trips passed at 460800 and 921600.
- NVS persistence was tested at 460800: after a Tab5-only hard reset, boot log
  reported `baud=460800 persisted=saved` and the shell probe passed.
- Initial test cleanup coordinated both endpoints back to 115200 and cleared
  the NVS key.
- The Module LLM was subsequently configured to retain `921600 8N1` for its
  M5Bus login UART across reboot. After that Linux reboot, Tab5 was changed
  through USB CDC to `active=921600 persisted=921600`.
- A Tab5-only hard reset then reported
  `Login UART: baud=921600 persisted=saved`, and the strict shell probe
  returned `shell-path-ok: m5stack-LLM`.
- Current installed baseline: both Module LLM and Tab5 use persistent
  `921600 8N1`.
- A failed high-speed attempt can leave corrupted bytes in Bash's current
  input line. Recover endpoint rates, allow the prompt to return, then rerun
  the probe. The probe now requires an executed marker rather than trusting
  process exit alone.

This removes UART line rate as a fixed build-time setting. It does not prove
that visible per-character refresh is UART-bound; the true-proportional
renderer remains a likely bottleneck if the symptom persists at 921600.

That rendering bottleneck was measured and mitigated later on 2026-06-08:

- With true-proportional rendering, a single Linux `printf` of 64 printable
  characters took 1.000 second at the post-render USB observation point,
  approximately 64 characters per second.
- A printable byte caused three whole-row redraws: erase the old cursor, render
  the changed cell, and draw the new cursor.
- The formal default was changed to fixed 18x20 cells while retaining the
  DejaVu18 font face.
- The identical test then took about 0.016 second, approximately
  4000 characters per second and a 62.5x improvement.
- The flashed firmware retained `active=921600 persisted=921600`, and the
  strict Module LLM shell probe passed.
- True-proportional rendering remains available only in
  `tab5_terminal_font_prop_preview` until a dirty-row/batched renderer is
  implemented.

## Expected Runtime Behavior

After boot:

- The Tab5 screen shows the status line with UART pins and baud rate.
- The right side of the status line shows a black battery panel with enlarged
  percentage text and an iOS-style battery icon. A lightning mark inside the
  icon means M5Unified reports that the battery is charging.
- Bytes from `LoginSerial` are passed through `terminal_core` and rendered
  below the status bar. The formal build uses fixed 18x20 cells with the
  DejaVu18 font face. The proportional renderer is preview-only.
- The current core implements a pragmatic VT102/xterm-style subset: C0
  controls, common ESC/CSI cursor and erase operations, scroll regions,
  insert/delete character and line, delayed autowrap, DEC Special Graphics,
  basic terminal replies, SGR attributes/colors, alternate screen switching,
  bracketed-paste/mouse-mode tracking or safe ignore, and a one-codepoint-per
  cell UTF-8 display path.
- Precise full xterm compatibility, app-level validation, CJK double-width
  layout, mouse event generation, and keyboard/input integration remain future
  work.
- Bytes from `LoginSerial` are echoed to USB debug Serial.
- Bytes typed into USB monitor are forwarded to `LoginSerial` while the bridge
  flag remains enabled.

## Last Verified State

Verified on 2026-06-04 in the Windows checkout at:

```text
C:\Users\cheyh\Desktop\embedded\M5_projs\Tab5_Term
```

Commands completed successfully:

```powershell
$env:PATH = "$env:USERPROFILE\.platformio\packages\toolchain-riscv32-esp\riscv32-esp-elf\bin;$env:PATH"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e tab5_min_uart_terminal
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e tab5_usb_keyboard_probe
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e tab5_power_status_probe
```

The default `tab5_min_uart_terminal` firmware with the first-stage terminal
core was then flashed to COM3 with the direct `esptool.py` command above, and
all flash hashes were verified.

Later, `tab5_terminal_cdc_inject` was flashed to COM3 with the direct
`esptool.py` command and all flash hashes were verified. This is the current
board image after the CDC-injection work. `tools/send_terminal_test.py --port
COM3 --test stage1-smoke` was run successfully and sent 280 bytes to the Tab5.
The user still needs to visually confirm the rendered screen output.

After that flash, terminal special-character rendering was improved in source:
DEC Special Graphics box drawing, SO/SI G0/G1 switching, and common 8-bit C1
controls were added; the CDC test helper now waits longer after opening the
port because opening USB CDC may reset ESP32-P4. These source changes were
compiled successfully for `tab5_min_uart_terminal` and
`tab5_terminal_cdc_inject`, but were intentionally not flashed at the user's
request. The board may therefore still be running the older CDC injection image
until an explicit flash is requested.

Rechecked on 2026-06-05: `tab5_min_uart_terminal` and
`tab5_terminal_cdc_inject` both still compile successfully after the special
character rendering changes. No firmware was flashed during this recheck.

Later on 2026-06-05, `tab5_terminal_cdc_inject` was compiled, flashed to COM3
with the direct `esptool.py` command, and all flash hashes were verified. Then
`tools/send_terminal_test.py --port COM3 --test stage1-smoke` was run and sent
416 bytes in one write. The user observed the early test lines, including
hello/world, backspace, tabs, OSC skipping, cursor-left overwrite, SGR colors,
and the start of the DEC graphics section; the trailing part of the test did
not fully appear. The visible `XYZoverwrite: abc` line was expected CR behavior
from the older test payload, not a terminal parser bug: CR moves to column 0
without clearing the rest of the row.

After that observation, `tools/send_terminal_test.py` was updated to use a
clearer `CR+EL` case and chunked writes by default. The updated helper was run
against the already-flashed `tab5_terminal_cdc_inject` image and sent 419 bytes
with `chunk_size=32` and `chunk_delay=0.050s`, then resent more conservatively
with `chunk_size=16` and `chunk_delay=0.080s`. The user photo-confirmed that
the full updated smoke test reached the screen: hello/world, `CR+EL: XYZ`,
backspace, tab, OSC skipping, cursor-left overwrite, color samples, DEC
graphics box drawing, G1 graphics switching back to ASCII, CSI H positioning,
8-bit CSI erase-line, final line, and the cursor were visible. `cleared tail`
appeared at the right side of its row because the test sends `CSI 2K` after
printing text; erase-line does not move the cursor before the next printable
bytes. This confirms the CDC overrun suspicion and validates the current
stage-1 parser/rendering smoke path at the photographed level.

Later on 2026-06-05, the VT102-style screen model work was implemented in
`terminal_core`: insert/delete character and line, erase character, scroll
regions, origin mode, delayed autowrap, tab-stop set/clear, basic DSR/CPR/DA
responses, and `terminal::redraw()`. Both `tab5_min_uart_terminal` and
`tab5_terminal_cdc_inject` compiled successfully after these changes. The CDC
injection image was flashed temporarily and this command sent 380 bytes:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_terminal_test.py --port COM3 --test stage2-screen --chunk-size 16 --chunk-delay 0.08
```

The host read back 17 response bytes:
`\x1b[0n\x1b[20;10R\x1b[?6c`. This verifies DSR OK, cursor-position report,
and VT102 primary device attributes over the terminal response callback.

After the CDC validation, the formal `tab5_min_uart_terminal` image was flashed
back to COM3 with the direct `esptool.py` command and all flash hashes were
verified. A final USB-to-login-UART bridge check sent
`printf 'Tab5 formal core final build\r\n'` to the external Module LLM shell
and read back the command echo, `Tab5 formal core final build`, bracketed-paste
mode toggles, and the `root@m5stack-LLM:~#` prompt. The current board image is
therefore the formal UART terminal build, not the CDC injection debug build.

For real end-to-end visual validation through the external LLM shell, use:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_login_shell_demo.py --port COM3 --demo vt102
```

This sends a temporary here-document to the LLM login shell; the shell emits
the terminal control sequences back to Tab5, so the display exercises the full
USB CDC -> Tab5 bridge -> LoginSerial -> LLM shell -> LoginSerial -> Tab5
terminal path. It does not create a file on the LLM module. This was run on
2026-06-05 and captured output from `host: m5stack-LLM` with kernel
`4.19.125`, plus color, inverse, DEC graphics, character edit operations, and
scroll-region demo sequences. A later `--demo probe` run sent 70 bytes and
captured `shell-path-ok: m5stack-LLM`.

Do not include terminal query sequences such as `CSI 6 n` in real shell demos
unless the LLM shell input side is prepared for the reply. In the formal build,
terminal replies are correctly sent back to `LoginSerial`, which means a query
from shell output becomes input to the shell; use CDC injection tests for DA,
DSR, and CPR validation instead.

Later on 2026-06-05, Stage 3 SGR/color work was implemented in
`terminal_core`. Both `tab5_min_uart_terminal` and `tab5_terminal_cdc_inject`
compiled successfully. The CDC injection image was flashed temporarily and this
command sent 673 bytes:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_terminal_test.py --port COM3 --test stage3-color --chunk-size 16 --chunk-delay 0.08
```

After that, the formal `tab5_min_uart_terminal` image was flashed back to COM3
with the direct `esptool.py` command and all flash hashes were verified. A
real LLM shell SGR check then sent 540 bytes through the full login-shell path
and captured 1243 bytes, including the expected `LLM shell -> Stage 3 SGR
check`, attribute samples, color samples, BCE row, and shell prompt output.
After removing an unused helper from `terminal_core`, both environments were
rebuilt again, the formal `tab5_min_uart_terminal` image was flashed to COM3
again with verified hashes, and `tools/send_login_shell_demo.py --demo probe`
captured `shell-path-ok: m5stack-LLM`. The `--demo sgr` real-shell check was
then replayed again against that final formal image, sent 540 bytes, and
captured 1243 bytes with the expected Stage 3 SGR output. To replay the
real-shell SGR check, use:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_login_shell_demo.py --port COM3 --demo sgr
```

Later on 2026-06-05, Stage 4 xterm/DEC essentials were implemented in
`terminal_core`. Both `tab5_min_uart_terminal` and `tab5_terminal_cdc_inject`
compiled successfully, and `tools/send_terminal_test.py` passed Python syntax
checking. The CDC injection image was flashed to COM3 with verified hashes, and
the one-time photo-validation command was run:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_terminal_test.py --port COM3 --test stage4-xterm --chunk-size 16 --chunk-delay 0.08 --read-response-window 0.8
```

It sent 646 bytes and read back 34 response bytes:
`\x1b[?6c\x1b[>0;0;0c\x1b[7;31R\x1b[?7;39R\x1b[?6c`.
This confirms primary DA, secondary DA, CPR, DEC private CPR, and ESC `Z`
DECID through the response callback. The user's photo confirmed the Stage 4
main path but showed poor UTF-8 glyph coverage with the default terminal font.
The terminal renderer then tried to keep ASCII cells on the existing font path
while non-ASCII UTF-8 cells used M5GFX `efontCN_12`. The user's second photo
showed that this still did not render `é`, `Ω`, and `中` reliably. The renderer
now uses strict UTF-8 cell decoding plus deterministic 6x8 bitmap fallback
glyphs for those Stage 4 validation characters, with other unsupported
non-ASCII falling back to `?`. A later font pass changed the default ASCII font
from scaled `Font0` to native `AsciiFont8x16`.

After that change, both `tab5_terminal_cdc_inject` and `tab5_min_uart_terminal`
compiled successfully. A transient PlatformIO host-side `checkprogsize`
failure occurred once for `tab5_min_uart_terminal`:
`pioarduino-build.py.esp32p4: 系统找不到指定的文件。`; immediately rerunning the
same build succeeded, confirming it was not a source error. The CDC injection
image was flashed to COM3 again with verified hashes, and the same command above
was replayed. It again sent 646 bytes and read back the same 34 response bytes:
`\x1b[?6c\x1b[>0;0;0c\x1b[7;31R\x1b[?7;39R\x1b[?6c`.
The board is intentionally left running the `tab5_terminal_cdc_inject` image
with the revised Stage 4 test screen visible so the user can send another photo
for UTF-8 glyph validation. After photo validation, flash
`tab5_min_uart_terminal` back before treating the board as the formal UART
terminal.

The user's follow-up photo confirmed that the deterministic fallback glyphs for
`é`, `Ω`, and `中` are visible as stable single-cell shapes instead of blank or
corrupt cells. Stage 4's deterministic CDC smoke test is therefore considered
passed at the current scope. The board was then flashed back to
`tab5_min_uart_terminal` for formal UART-terminal use.

Stage 4 real app smoke was then added in
`tools/send_login_shell_app_smoke.py` and run through the formal UART bridge:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_login_shell_app_smoke.py --port COM3 --apps clear,reset,tput,less,nano,vim,htop --rows 43 --cols 96 --chunk-size 32 --chunk-delay 0.08
```

This launches actual programs on the external LLM module through the path
USB CDC -> Tab5 bridge -> LoginSerial -> LLM shell -> LoginSerial -> Tab5
terminal. The validation helper must chunk shell input; sending long commands
in one write caused dropped characters once and left the shell in an unfinished
single-quote continuation prompt. Recovery was to send a closing `'`, after
which bash reported `-bash: rintf: command not found` and returned to the normal
prompt.

The full app smoke result on 2026-06-05 was:

- `clear`: ok `rc=0`
- `reset`: ok `rc=0`
- `tput`: ok `rc=0`
- `less`: ok `rc=0`
- `nano`: skipped because the current LLM module image does not have `nano`
  installed
- `vim`: ok `rc=0`
- `htop`: ok `rc=0`

Important trap: GNU less 590 on the LLM module blocks in
`open("/dev/ttyS1", O_RDONLY)` unless the serial TTY has `clocal` set. This was
confirmed with `strace` after `timeout 3s less -F -X /etc/os-release` returned
`124`; the trace showed the block at `/dev/ttyS1`. The smoke helper therefore
sets `stty echo clocal rows 43 cols 96` during setup and also refreshes
`stty clocal` before each app, because running `reset` can clear the termios
flag.

Earlier, the `tab5_power_status_probe` firmware with INA226 current/voltage
rolling-window diagnostics was flashed while investigating charging detection.
It was useful for confirming that `p=1`, `a=1`, and `e=0` stayed unchanged
while `n` kept increasing and `i/v` fluctuated during repeated cable
insert/remove cycles. That diagnostic firmware is no longer the current board
image; the board has since been flashed back to the default terminal firmware.

## USB-A Keyboard Probe

Goal: make Tab5 standalone by reading a USB keyboard from the Tab5 USB-A host
port and forwarding key input to `LoginSerial`.

Research summary:

- Tab5 has a USB-A host port.
- M5Unified exposes external output power control; `M5.begin()` defaults
  `cfg.output_power=true`, and Tab5 external USB power can also be explicitly
  enabled with `M5.Power.setExtOutput(true, m5::ext_USB)`.
- Arduino-ESP32 `USBHIDKeyboard` is device-side and is not suitable for reading
  an external keyboard.
- ESP-IDF USB host APIs are present in the local ESP32-P4 Arduino package
  (`usb/usb_host.h`, `libusb.a`, `libesp_hid.a`).
- ESP-IDF's HID host example uses the `usb_host_hid` component, but that
  component header set was not present in this Arduino ESP32-P4 package.

Implemented probe:

1. `platformio.ini` now has `tab5_usb_keyboard_probe`, extending the default
   env and defining `ENABLE_USB_KEYBOARD_PROBE=1`.
2. `setup()` explicitly enables Tab5 USB-A power with
   `M5.Power.setExtOutput(true, m5::ext_USB)` when the probe flag is enabled.
3. `src/usb_keyboard_probe.cpp` uses low-level `usb/usb_host.h` directly.
4. The probe enumerates USB devices, claims HID boot keyboard interfaces, sends
   `SET_PROTOCOL boot` and `SET_IDLE`, then polls interrupt-IN boot reports.
5. Basic US keyboard scancodes are translated to bytes and common VT-style
   navigation sequences, then queued to the main loop.
6. The main loop drains the keyboard queue and writes bytes to `LoginSerial`.

Build verification completed on the Windows checkout:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e tab5_min_uart_terminal
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e tab5_usb_keyboard_probe
```

Runtime validation with an actual USB keyboard on Tab5 USB-A is deferred and
does not block the current mainline stage.
Test the probe before treating it as the default terminal input path.

This probe is now part of Stage 5 described in `docs/current_work.md`. It must
be refactored toward a transport-independent key event/input mapping layer
before USB and the official keyboard are both treated as supported production
inputs.

## Terminal Font Decision

On 2026-06-05, the user reported that the terminal font looked rough, like a
minimal pixel font scaled up. Local M5GFX font inventory showed:

- Previous terminal ASCII font: `fonts::Font0` at text size 2. This is a 6x8
  GLCD font scaled to 12x16, so it looks blocky.
- Best low-risk built-in candidate: `fonts::AsciiFont8x16`. This is a native
  fixed 8x16 ASCII bitmap font, so it should look cleaner at the current
  height.
- `DejaVu*` and `FreeMono*` are smoother but GFX proportional/baseline fonts.
  The project now supports both fixed-cell rendering and a true proportional
  visual renderer. Fixed-cell rendering remains the correctness/debug baseline;
  proportional rendering is for release-style visual inspection.

The earlier candidate used `AsciiFont8x16` at text size 1 in fixed 12x16 cells,
preserving a `96x43` validation baseline. It was cleaner than scaled `Font0`,
but the user preferred the later DejaVu18 true-proportional result.

Font selection is compile-time configurable:

- `TERMINAL_FONT_FACE_ASCII8X16=0`
- `TERMINAL_FONT_FACE_DEJAVU18=1`
- default `TERMINAL_FONT_FACE=TERMINAL_FONT_FACE_DEJAVU18`
- `TERMINAL_RENDER_MODE_FIXED_CELL=0`
- `TERMINAL_RENDER_MODE_PROPORTIONAL=1`
- default `TERMINAL_RENDER_MODE=TERMINAL_RENDER_MODE_PROPORTIONAL`
- default `TERMINAL_FORCE_FIXED_CELL_RENDERING=0`

The independent proportional preview env is
`tab5_terminal_font_prop_preview`. It extends CDC injection and uses
`fonts::DejaVu18` in `18x20` rows with
`TERMINAL_RENDER_MODE_PROPORTIONAL=1` and
`TERMINAL_FORCE_FIXED_CELL_RENDERING=0`. This is the closest built-in M5GFX
option to a 20px line-height, non-monospaced preview without using a destructive
`setTextSize(20)` scale factor. In this mode, rows are redrawn by accumulating
actual glyph advances, so narrow glyphs such as `i`, `l`, and `!` should
visibly occupy less horizontal space than `M` and `W`.

The debug comparison env is `tab5_terminal_font_prop_fixed_debug`. It uses the
same `DejaVu18` font and `18x20` geometry but sets
`TERMINAL_FORCE_FIXED_CELL_RENDERING=1`, forcing equal-width logical cells.
Use this mode when developing or testing terminal functionality with the
proportional-looking font, so cursor addressing and TUI layout stay easy to
inspect.

For real login-UART compatibility testing, use
`tab5_min_uart_terminal_fixed_debug`. CDC injection, USB keyboard probe, and
power-status probe environments also explicitly force fixed-cell rendering.

For visual evaluation, `tools/send_terminal_test.py` gained a `font-preview`
test. The CDC injection image was built and flashed to COM3 with verified
hashes, then this command was run:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_terminal_test.py --port COM3 --test font-preview --chunk-size 16 --chunk-delay 0.08 --read-response-window 0.2
```

It sent 686 bytes and left the Tab5 screen on a font preview page containing
uppercase/lowercase/digits/punctuation, dense ambiguous glyphs, attributes,
colors, DEC graphics, and UTF fallback samples. This remained pending user photo
approval at that moment.

For the first proportional-font 20px-line-height preview,
`tab5_terminal_font_prop_preview` was built successfully and the default
`tab5_min_uart_terminal` was also rebuilt successfully after the font-selector
changes. That preview still rendered each glyph centered inside a fixed cell,
so the user correctly observed that `i`, `l`, and `!` still occupied the same
logical width as other letters.

The renderer was then extended with true proportional display mode. In
proportional mode, terminal data remains a logical cell grid, but rendering
computes each row's x positions from actual glyph advances. A single changed
cell can shift later text on that row, so the proportional renderer repaints
the affected row instead of relying on single-cell pixel updates. This is
expected to be slower than fixed-cell debug rendering.

The following environments were built successfully after this change:

- `tab5_terminal_font_prop_preview`
- `tab5_terminal_font_prop_fixed_debug`
- `tab5_terminal_cdc_inject`
- `tab5_min_uart_terminal`

The true proportional preview image was flashed to COM3 with verified hashes,
then this command was run:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" tools\send_terminal_test.py --port COM3 --test font-prop-preview --chunk-size 8 --chunk-delay 0.12 --read-response-window 0.2
```

It sent 769 bytes and left the Tab5 screen on
`Font preview: DejaVu18 true proportional in 18x20 rows`. At that point the
board was running the CDC-injection preview image; the formal UART image was
restored during the final validation below.

External font strategy is documented in `docs/terminal_font_strategy.md`.
Confirmed M5GFX routes are compile-time `lgfx::GFXfont` data with
`M5.Display.setFont(&font)` and runtime `loadFont(...)` for VLW/BFF-style font
sources from arrays, paths, filesystem objects, or `DataWrapper` objects.

On 2026-06-08, the user approved the DejaVu18 true-proportional appearance.
The font face remains accepted, but the formal renderer later returned to
fixed cells for throughput:

- Formal `tab5_min_uart_terminal`: DejaVu18, fixed 18x20 cells, `64x32`, with
  one blank character row above and below the terminal area.
- Formal debug `tab5_min_uart_terminal_fixed_debug`: same font and geometry,
  fixed-cell display.
- `tab5_terminal_font_prop_preview`: DejaVu18 true-proportional display.
- CDC and hardware probe environments: fixed-cell display for deterministic
  feature testing.
- `tools/send_login_shell_app_smoke.py` defaults to `rows=32`, `cols=64`.
- External-font support remains a documented future option, not required for
  the accepted current style.

Final font-stage validation on 2026-06-08:

- `tab5_min_uart_terminal`, `tab5_min_uart_terminal_fixed_debug`,
  `tab5_terminal_cdc_inject`, and `tab5_terminal_font_prop_preview` all built
  successfully.
- The formal `tab5_min_uart_terminal` image was flashed to COM3; esptool
  verified all written data and reported MAC `30:ed:a0:e2:e2:48`.
- `.\tools\tab5.ps1 probe -Port COM3` sent a command through
  `USB CDC -> Tab5 -> Module LLM login UART` and captured
  `shell-path-ok: m5stack-LLM`.
- At that checkpoint the board used the accepted DejaVu18 true-proportional
  renderer. The later performance rollback to fixed cells supersedes this
  runtime state without changing the font face.

Later on 2026-06-08, the formal default was rotated 180 degrees for the
official companion keyboard installation direction. The rotated formal build
compiled and flashed to COM3 with verified hashes, and the login-shell probe
again returned `shell-path-ok: m5stack-LLM`. The board's current image includes
this orientation change.

## Quick Validation Checklist

On the new Windows machine:

```powershell
.\tools\tab5.ps1 build tab5_min_uart_terminal
.\tools\tab5.ps1 flash tab5_min_uart_terminal -Port <PORT>
.\tools\tab5.ps1 probe -Port <PORT>
```

Then type:

```sh
id
```

Expected result: the Module LLM shell responds, and the response is visible in
both the USB monitor and on the Tab5 display.

## Stage 7 Unicode Checkpoint

U1/U2 were completed on 2026-06-12. The terminal cell model now distinguishes
single cells, wide leads, and wide continuations. Unicode width follows fixed
Unicode 15.0.0 tables; combining marks attach without advancing the cursor,
and malformed UTF-8 is validated before entering the screen model.

The physical `stage7-unicode` regression passed together with Stage 1-4 at
5/5. Formal firmware was then restored and passed the login-shell and
full-screen application smoke checks. See `docs/current_work.md` for the
milestone contract and `docs/terminal_validation.md` for exact reproduction
commands. Font coverage remains narrower than the now-correct column model.

On 2026-06-15 the user asked to save current requirements, discovered issues,
and automation-command notes before continuing planned work while skipping
deferred items. U3 was selected as the next practical increment because modern
terminal TUIs often use Unicode box drawing and block elements. The new local
test name is `stage7-unicode-graphics`; use the framebuffer screenshot command
for final-pixel review when the board is available.

U3 validation on 2026-06-15:

- `tab5_terminal_regression` built and flashed to COM3.
- `tools/tab5.ps1 terminal-regression -Port COM3` passed 6/6, including
  `stage7-unicode-graphics`.
- `tools/tab5.ps1 screenshot -Port COM3 -OutputPath
  .logs/screenshots/stage7-unicode-graphics.png` captured a 1280x720 RGB565LE
  framebuffer with CRC32 `6D8657DB`.
- The screenshot showed connected light/heavy/rounded box samples and visible
  block/shade elements.
- `tab5_min_uart_terminal` was flashed back and
  `tools/tab5.ps1 probe -Port COM3` returned `shell-path-ok: m5stack-LLM`.
