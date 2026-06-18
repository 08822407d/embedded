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
  -> HardwareSerial onReceive callback in main.cpp
  -> main.cpp bounded UART drain into the software RX ring
  -> main.cpp bounded RX-ring pop
  -> terminal::writeBytes()
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

For the formal UART path, `src/main.cpp` deliberately separates UART draining
from rendering. Arduino `HardwareSerial.onReceive(..., false)` drains available
hardware-UART bytes into a software RX ring, and the main loop later pops a
smaller render batch for `terminal::writeBytes()`. This keeps the UART FIFO
serviced while long screen updates are still slow. The render batch size is
`LOGIN_UART_RENDER_BYTES_PER_LOOP`; the current default is `320` bytes after
Stage 10 P6. Do not raise it blindly: larger batches reduce flush/cursor
overhead but also delay status/input/M5 update work while a batch is rendered,
and `384`/`512`/`1024` byte experiments exposed UART FIFO overflow in at least
one validation path.
The formal UART bridge also uses terminal write transactions while the software
RX ring still has backlog (`LOGIN_UART_DEFER_RENDER_WHILE_BACKLOG=1`). Bytes
are parsed into the logical terminal model promptly, but physical row flushes
are delayed until the backlog clears or `LOGIN_UART_RENDER_DEFER_MAX_MS`
expires. Keep this distinction clear when interpreting `TAB5PIPE rendered`
counters: they mean bytes have been parsed and accounted for, while the final
batch path flushes the screen before the quiescent validation point.
The fixed-cell renderer also uses
`TERMINAL_FIXED_ROW_BACKGROUND_PREFILL` by default, filling same-background row
runs before glyph drawing so common shell/TUI rows avoid per-cell background
fills. `TERMINAL_TRANSPARENT_TEXT_AFTER_ROW_PREFILL` then draws text over that
prefilled row without asking M5GFX to fill each glyph box, while setting the
M5GFX base color for correct anti-aliased edge blending.
`TERMINAL_DEFER_SCROLL_DURING_WRITE_BYTES` defers physical framebuffer scrolls
inside `terminal::writeBytes()` spans; it updates the logical screen model and
redraws the final dirty scroll region at the end of the batch. Row text-style
caching avoids repeating identical font/color setup while a row is being
redrawn. Keep the single-byte path immediate for interactive output.

## Source Layout

### Orchestration

- `src/main.cpp`: startup order and main loop. It initializes M5Unified,
  display orientation, display boot guard, login UART, terminal viewport,
  optional debug modes, input queue, official A164 keyboard, USB-A keyboard
  support, and the formal login-UART software RX ring. It should stay thin.
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
- `include/render_pipeline_diagnostics.h` exposes diagnostic snapshots for the
  formal login-UART receive/render/mirror pipeline, plus the runtime CDC
  mirror enable switch used by Stage 10 tests. The implementation lives in
  `src/main.cpp` because it reports main-loop software RX state.
- `src/login_uart.cpp` owns the external login UART, supported runtime baud
  rates, delayed switching, optional NVS state, actual RX buffer reporting, and
  UART driver error counters.
- `src/power_detect_probe.cpp` is an isolated diagnostic firmware path for
  charging-state research.

### Host Tools

- `tools/tab5.ps1`: primary Windows wrapper for build, flash, diagnostics, and
  scripted validation.
- `tools/run_terminal_regression.py` and `tools/run_stage6_regression.ps1`:
  deterministic parser/screen regression entry points.
- `tools/send_login_shell_app_smoke.py`: real UART login-shell app/TUI matrix.
- `tools/measure_render_latency.py`: Stage 10 real login-shell burst workload;
  records mirror-disabled pipeline timing by default, optional CDC mirror
  exact-line integrity, expected byte counts, and `TAB5PIPE`
  receive/render/mirror counters for performance comparisons.
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
- Use the Stage 10 render-latency workload before changing the renderer. The
  default wrapper path disables CDC mirroring and relies on `TAB5PIPE`
  counters; use `-EnableMirror` only for short CDC mirror integrity checks.
  Current P6 evidence shows deferred hardware scroll is valuable, but future
  renderer changes still need clean `TAB5PIPE` receive counters and regression
  runs before they are treated as accepted.
