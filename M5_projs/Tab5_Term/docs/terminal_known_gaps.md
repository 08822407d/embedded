# Terminal Known Gaps

This file records behavior that is intentionally unsupported, incomplete, or
not yet validated. Do not advertise these items as compatible until their
implementation and regression tests pass.

## Text And Unicode

- Unicode column width is fixed to Unicode 15.0.0. East Asian ambiguous
  characters intentionally remain one column.
- A cell stores at most eight UTF-8 bytes. Common base-plus-combining sequences
  fit, but excess combining marks are dropped and ZWJ emoji sequences are not
  yet kept as one shaped grapheme.
- Combining layout currently has simple drawn fallbacks for grave, acute,
  circumflex, tilde, and diaeresis. Other zero-width marks remain stored and
  width-correct but may not be visible with the current font path.
- The current font path has deterministic fallback glyphs for the Stage 4
  validation characters `é`, `Ω`, and `中`, plus the Stage 7 U3 common
  Unicode box-drawing and block-element set. Other unsupported base code
  points render as `?` even when their one- or two-column layout is correct.
- Unicode line breaking, bidirectional text, and complex shaping are outside
  the current terminal scope.

## xterm And Input

- Mouse tracking modes are recognized so their escape sequences do not leak,
  but touch/mouse reports are not emitted to the host.
- Bracketed-paste mode is tracked, but there is no user-facing paste source in
  the current physical input path.
- The A164 driver tracks one non-modifier HID key at a time. Full simultaneous
  non-modifier rollover is not guaranteed. First-pass semantic capture passed
  printable text, Enter, Tab, Backspace, Escape, arrows, Delete, Ctrl+A,
  Ctrl+E, Alt+x, and application cursor arrows. Home/End/PageUp/PageDown/
  Insert/F1-F4 were not captured. Official A164 documentation only describes
  Sym/Aa/Ctrl/Alt as combo keys, and the local M5Unit-KEYBOARD HID table does
  not map F1-F4 or Home/Page/Insert usages, so treat those as unavailable on
  stock A164 firmware unless future official docs/firmware add a combo.
- USB-A keyboard support is enabled in the default formal firmware and uses the
  shared input mapper alongside the official A164 keyboard. Basic physical
  input through the Tab5 USB-A port, reconnect, Shift/Ctrl-direction cases,
  Backspace repeat, full-screen `less` and `htop` exits, and at least 3-key
  printable groups have passed. Stage 9 automated TUI coverage has passed for
  installed `clear`, `reset`, `tput`, `less`, `vim`, `htop`, `top`, and
  `whiptail`; `tmux`, `screen`, `dialog`, `btop`, and `nano` were skipped
  because they were not installed on the current Module LLM image. USB-A
  keyboard first-pass semantic capture passed printable text, Enter, Tab,
  Backspace, Escape, normal and application arrows, PageUp/PageDown, Home/End,
  Delete, Insert, F1-F4, Ctrl+A, Ctrl+E, and Alt+x. Full NKRO is not claimed,
  and the isolated `tab5_usb_keyboard_probe` environment remains useful for
  USB-only diagnostics. A previous USB coexistence test exposed an M5GFX
  panel-detection failure that could leave the board black-screened;
  `display_boot_guard` now recovers by restarting after a failed display
  autodetect, but the underlying M5GFX/Tab5 race is not root-caused.
- Only terminal queries and private modes covered by the Stage 1-4 corpus and
  real-application smoke are currently claimed.

## Serial Session Integration

- Raw UART has no automatic PTY window-size propagation. The Module LLM login
  profile currently applies the fixed `64x32` size.
- The private USB CDC management and regression protocols are local device
  tooling, not terminal escape-sequence extensions advertised to Linux
  applications.
- PPP/IP with SSH or Telnet remains a future transport option and is not part
  of the current UART terminal.

## Display And Performance

- Formal firmware uses fixed-cell DejaVu18 rendering for interactive
  performance. Proportional rendering remains a preview/debug option.
- Capture-enabled debug firmware can verify final logical framebuffer pixels,
  including font rasterization and status-bar placement. It cannot verify
  backlight brightness, panel color cast, viewing angle, ghosting, bad pixels,
  touch alignment, enclosure mounting, or physical readability.
- Framebuffer capture blocks the main loop while roughly 1.8 MB is streamed
  over USB CDC. It is debug instrumentation for stable screens, not a
  production-time display transport.

## Power Detection Caveat

- Dynamic charge-state detection cannot rely on `CHG_STAT` /
  `M5.Power.isCharging()` because those signals did not reliably follow cable
  insertion and removal on the tested Tab5. The production status-bar lightning
  icon now uses a debounced INA226-current heuristic derived from
  `tab5_power_detect_probe` evidence: strong negative current means charging.
  A debug/programming data cable may not show the lightning icon if the battery
  current is not sufficiently negative. Broader validation across battery
  levels and charger types remains useful.
- This issue was explicitly excluded from Stage 6 acceptance on 2026-06-12,
  then later handled for the tested hardware with the INA226 heuristic. Battery
  percentage/level refresh remains independently accepted.
