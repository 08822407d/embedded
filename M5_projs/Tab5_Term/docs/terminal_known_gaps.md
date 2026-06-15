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
  non-modifier rollover is not guaranteed.
- USB-A keyboard support is explicitly deferred.
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

## Deferred Non-Terminal Issue

- Dynamic charge-state detection was intentionally shelved after the observed
  hardware/API indicators did not reliably follow cable insertion and removal.
  Resume only with an automated capture method or new hardware evidence.
- This deferred issue was explicitly excluded from Stage 6 acceptance on
  2026-06-12. Battery percentage/level refresh remains independently accepted.
