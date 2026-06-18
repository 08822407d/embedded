# Developer Architecture

This document is the codebase map for people who want to understand or modify
the Tab5 terminal firmware without reading every source file first. It records
the current architecture; it is not a promise that every future feature must fit
the same shape.

For project status and validation history, read `docs/current_work.md` and
`docs/terminal_validation.md`. For known limitations, read
`docs/terminal_known_gaps.md`.

## Reading Order

Start here:

1. `platformio.ini` for build environments and compile-time feature gates.
2. `include/app_config.h` for default pins, terminal geometry, and feature
   macros.
3. `src/main.cpp` for runtime orchestration.
4. `include/terminal_core.h` and `src/terminal_core.cpp` for terminal parsing,
   screen state, rendering, and diagnostics.
5. `include/input_mapper.h`, `src/input_mapper.cpp`,
   `include/hid_keyboard_mapper.h`, and `src/hid_keyboard_mapper.cpp` for the
   normalized keyboard model.
6. Device-specific input and transport modules only after the shared model is
   clear: `src/tab5_keyboard_input.cpp`, `src/usb_keyboard_probe.cpp`,
   `src/login_uart.cpp`, and `src/usb_management.cpp`.

## Runtime Flow

The formal `tab5_min_uart_terminal` firmware is a real UART terminal, not just
a display demo:

```text
external login UART bytes
  -> login_uart::serial()
  -> main.cpp bounded UART drain
  -> terminal::writeByte()
  -> terminal_core parser/screen model
  -> M5GFX drawing in the terminal viewport

physical keyboard event
  -> A164 or USB-A input driver
  -> input_event_queue
  -> input_mapper, using terminal application-cursor/keypad modes
  -> login_uart::serial()
  -> external Linux login shell
```

`src/main.cpp` keeps each loop bounded: UART bytes, queued key events, USB host
work, status-bar refresh, and `M5.update()` all get regular time. Avoid adding
unbounded loops there. Heavy or blocking work should move into a module with a
clear update function or FreeRTOS task.

## Source Layout

### Orchestration

- `src/main.cpp`: startup order and main loop. It initializes M5Unified,
  display orientation, display boot guard, login UART, terminal viewport,
  optional debug modes, input queue, official A164 keyboard, and USB-A keyboard
  support. It should stay thin.
- `include/app_config.h`: default compile-time configuration shared by all
  environments. Most feature flags can be overridden from `platformio.ini`.
- `src/display_boot_guard.cpp`: detects unusable display dimensions after
  M5GFX startup and restarts a limited number of times.
- `src/display_orientation.cpp`: centralizes the standard vs keyboard-mounted
  180-degree orientation choice.

### Terminal Core

- `include/terminal_core.h`: public terminal API. Other modules should use this
  boundary instead of reaching into terminal internals.
- `src/terminal_core.cpp`: terminal parser, cell buffers, SGR/color state,
  DEC/xterm modes, UTF-8 width handling, fallback glyph drawing, alternate
  screen, replies, and diagnostic snapshots.
- `src/unicode_width.cpp`: generated Unicode width interval tables used by the
  core.

`terminal_core.cpp` is intentionally still one translation unit today. It is
large, but splitting it before the next performance stage would make regression
tracking harder. If it is split later, use these boundaries:

- parser and escape-sequence dispatch;
- screen model and cell mutation;
- Unicode width and fallback glyph rendering;
- M5GFX renderer;
- diagnostics and snapshot export.

Keep these invariants intact:

- All printable output enters through `terminal::writeByte()` or
  `terminal::writeBytes()`.
- A cell is either single-width, a wide-glyph lead, or a wide-glyph
  continuation. Mutations that erase, insert, delete, or scroll cells must
  repair wide-pair state.
- Terminal replies use the configured `ResponseWriter`; they must not write
  directly to a particular UART or CDC transport.
- Raw UART login behavior does not automatically resize the host PTY. The
  current Module LLM profile applies `64x32` on the Linux side.

### Input

- `include/input_mapper.h` defines the transport-neutral key event model:
  `KeyCode`, `Modifier`, `KeyAction`, and `TerminalModes`.
- `src/input_mapper.cpp` turns normalized key events into xterm-compatible byte
  sequences, including application cursor/keypad modes and modifier encoding.
- `include/hid_keyboard_mapper.h` and `src/hid_keyboard_mapper.cpp` translate
  USB HID usage/modifier bytes into the shared input model.
- `src/input_event_queue.cpp` decouples device drivers and the main UART bridge.
- `src/tab5_keyboard_input.cpp` reads the official A164 keyboard through
  M5Unit-KEYBOARD HID mode.
- `src/usb_keyboard_probe.cpp` is the USB-A host keyboard path. The name
  contains historical "probe" wording, but the default formal firmware now
  enables USB-A keyboard input.

New keyboard transports should submit `input::KeyEvent` into the queue. Do not
emit terminal escape sequences directly from a device driver unless the shared
mapper is being deliberately bypassed for a documented reason.

### UI, Power, And Diagnostics

- `src/ui_theme.cpp` owns the color palette used by the terminal and status bar.
- `src/status_bar.cpp` draws the top bar, battery area, and charging icon. It
  also contains the current INA226 charge-current heuristic.
- `src/screen_capture.cpp` implements debug-only framebuffer capture.
- `src/terminal_debug_input.cpp` implements CDC injection and terminal state
  diagnostics for regression builds.
- `src/power_detect_probe.cpp` is an isolated diagnostic firmware path for
  charging-state research.

### Host Tools

- `tools/tab5.ps1`: primary Windows wrapper for build, flash, diagnostics, and
  scripted validation.
- `tools/run_terminal_regression.py` and `tools/run_stage6_regression.ps1`:
  deterministic parser/screen regression entry points.
- `tools/send_login_shell_app_smoke.py`: real UART login-shell app/TUI matrix.
- `tools/capture_keyboard_semantics.py`: `cat -vET` keyboard sequence capture.
- `tools/capture_screen.py`: debug framebuffer capture and optional pixel
  comparison.

Prefer adding repeatable host-side scripts for fixed procedures instead of
reconstructing long commands manually.

## Build Environments

The important PlatformIO environments are:

| Environment | Purpose |
| --- | --- |
| `tab5_min_uart_terminal` | Formal firmware. UART terminal with A164 and USB-A keyboard paths enabled. |
| `tab5_min_uart_terminal_fixed_debug` | Formal-like build with fixed-cell rendering forced for visual/debug checks. |
| `tab5_terminal_cdc_inject` | Debug build. USB CDC bytes feed the terminal core directly and do not reach the login UART. |
| `tab5_terminal_regression` | CDC injection plus terminal state and framebuffer diagnostics. |
| `tab5_screen_capture` | Formal-like build with debug framebuffer capture enabled. |
| `tab5_power_detect_probe` | Isolated power/charging-state research firmware. |
| `tab5_usb_keyboard_probe` | USB-A keyboard isolation build; useful for USB-only debugging. |
| `tab5_terminal_font_prop_preview` | Proportional-font preview path, not the formal performance baseline. |

When adding a feature flag, place the default in `include/app_config.h` and
override it from `platformio.ini` only for the environments that need a
different behavior.

## Extension Guidelines

- Add terminal compatibility in small, testable increments. Update
  `docs/terminal_implementation_plan.md`, `docs/terminal_validation.md`, and
  `docs/terminal_known_gaps.md` when the compatibility claim changes.
- Keep device drivers transport-specific and input semantics shared. A164,
  USB-A, and any future keyboard should converge at `input::KeyEvent`.
- Keep debug CDC injection separate from production keyboard input so debug
  bytes cannot interfere with a real login session.
- Do not advertise a broader `TERM` identity than the firmware and Module LLM
  configuration can actually support.
- Treat current `64x32`, `xterm-256color`, truecolor, and 921600-login-UART
  behavior as the accepted baseline unless the user explicitly changes it.
- If a change requires the user to press USB/A164 keys, announce that before
  starting the capture window.

## Open Refactor Opportunities

These are reasonable future improvements, not required cleanup for normal
feature work:

- Split `terminal_core.cpp` along parser/model/render/diagnostic boundaries
  after adding performance measurements.
- Rename `usb_keyboard_probe` to a production-neutral module name once the
  compatibility aliases and documentation can be updated safely.
- Add a generated source-map or Doxygen pass if external contributors start
  browsing APIs through generated docs.
- Add performance counters before changing the renderer; the current user-visible
  bottleneck is likely redraw cost, so changes should be measured.
