# Terminal Implementation Plan

This project will grow terminal compatibility in layers. The goal is to keep
each step small enough to validate on the Tab5 while still moving toward a
practical `xterm-256color` subset.

Do not advertise a broader `TERM` value than the firmware can actually satisfy.
During early work, test with simple shell output first. Move toward
`xterm-256color` only after the parser, screen model, color attributes, and
input behavior have been validated.

Preparatory work for future advanced transports or features does not require
immediate Tab5 hardware validation unless it changes current user-visible
behavior or the active UART login path. Record those items as implemented but
unverified, with enough detail that they can be validated when the advanced
feature is built.

## Stage 1: Core Screen And Parser

Goal: replace direct display writes with a character-cell terminal core.

Status on 2026-06-05: implemented in `terminal_core` and photo-validated via
`tools/send_terminal_test.py --test stage1-smoke` on the Tab5 CDC injection
build. The same core is compiled into the formal UART terminal build.

Deliverables:

- Fixed character grid below the status bar.
- Cursor position tracked independently from M5GFX's print cursor.
- Basic C0 controls: NUL, BEL, BS, HT, LF, VT, FF, CR, ESC, DEL, CAN, SUB.
- Scroll up/down for the full terminal area.
- ESC skeleton for reset, save/restore cursor, index, next line, reverse index,
  and unsupported sequence recovery.
- CSI skeleton for cursor movement, cursor position, erase display, erase line,
  save/restore cursor, SGR reset/basic colors, and unsupported sequence
  recovery.
- Unsupported OSC/DCS/APC/PM strings are consumed safely instead of leaking
  printable bytes to the screen.

Recommended temporary identity: `ansi` or `vt102` behavior, not full
`xterm-256color`.

## Stage 2: Screen Model Completeness

Goal: support the operations used by line editors, pagers, and simple TUIs.

Status on 2026-06-05: implemented in `terminal_core` for the listed core
operations. `stage2-screen` was replayed through the CDC injection build, and
host-side terminal responses confirmed DSR OK, cursor-position report, and
VT102 primary device attributes. Full app-level validation is still a later
step.

Deliverables:

- Clear screen/line modes audited against ECMA-48 behavior.
- Insert/delete line and character.
- Erase character.
- Scroll region (`DECSTBM`) and origin mode.
- Autowrap behavior closer to DEC terminals, including last-column wrap
  pending state.
- Cursor visibility mode.
- Full redraw path for recovery and display rotation changes.

Recommended temporary identity: `vt102`-style behavior if validation passes.

Current recommendation after this stage: treat the firmware as a pragmatic
VT102-style subset for simple shell use. Do not advertise `xterm-256color`
until Stage 3 attributes and Stage 4 xterm/DEC essentials have app-level
validation.

## Stage 3: Attributes And Color

Goal: make common colored shell and TUI output legible.

Status on 2026-06-05: implemented in `terminal_core` for the listed SGR
features. `stage3-color` was replayed through the CDC injection build, and a
real LLM login-shell SGR demo was replayed through the formal UART build.
Human visual confirmation of exact color/attribute appearance is still useful
because this stage is display-quality dependent.

Deliverables:

- SGR reset, bold/dim, underline, inverse, hidden if useful.
- 8-color and bright 16-color foreground/background.
- 256-color `38;5;n` and `48;5;n`.
- Optional truecolor `38;2;r;g;b` and `48;2;r;g;b`, mapped to RGB565.
- Background color erase behavior decided and tested.

Recommended temporary identity: project-specific or `xterm-16color`/narrow
terminfo until 256-color behavior is validated.

Current background-color erase decision: erased cells use the current foreground
and current background colors, but clear text attributes such as underline,
inverse, and hidden. This preserves colored backgrounds for common TUI output
without copying hidden/underline behavior into erased space.

## Stage 4: xterm/DEC Essentials

Goal: run mainstream terminal applications well enough for real use.

Status on 2026-06-05: first implementation pass completed in `terminal_core`.
The deterministic `stage4-xterm` CDC injection test has been replayed on the
Tab5. Host-side responses confirmed primary DA, secondary DA, CPR, DEC private
CPR, and DECID. Photo passes showed that UTF-8 bytes were grouped into one cell,
but the default GLCD font did not cover the sample glyphs reliably and M5GFX
`efontCN_12` still did not provide stable coverage for the mixed
Latin-1/Greek/CJK sample. The terminal uses a small deterministic single-cell
bitmap fallback for the Stage 4 UTF-8 validation characters (`é`, `Ω`, `中`);
other unsupported non-ASCII characters currently fall back to `?`. A follow-up
photo confirmed the revised UTF-8 fallback no longer leaves those validation
cells blank or visibly corrupt. The approved release-style renderer now uses
DejaVu18 with true proportional advances in 18x20 rows, while compatibility
testing uses the explicit fixed-cell debug builds. Full login-shell app smoke
validation was then run through the formal UART build.
Installed terminfo/full-screen tools `clear`, `reset`, `tput`, `less`, `vim`,
and `htop` all returned `rc=0`; `nano` was skipped because it is not installed
on the current LLM module image. `less` requires `stty clocal` on the LLM
module's serial TTY, otherwise GNU less blocks while opening `/dev/ttyS1`.

Deliverables:

- Alternate screen buffer (`?1049h/l`) with cursor save/restore.
- Application cursor keys and keypad mode.
- Bracketed paste tracking or safe ignore.
- Device/status replies needed by common applications.
- Mouse tracking explicitly ignored or implemented; do not leak sequences.
- UTF-8 printable text path. Current scope is one codepoint per cell; CJK
  double-width layout remains future work.

Recommended identity after validation: `xterm-256color` subset.

## Stage 5: Input And Integration

Goal: make Tab5 standalone rather than only a display/bridge, with display and
input orientation matching the physical keyboard installation.

Status: completed on 2026-06-12. The shared mapper, official A164 keyboard,
keyboard-mounted orientation, and login-UART integration passed the recorded
physical acceptance checks. Full simultaneous non-modifier rollover remains a
documented limitation rather than a supported guarantee.

Deliverables:

- Encapsulated display-orientation control instead of raw rotation values in UI
  modules.
- Default 180-degree keyboard-mounted landscape orientation for the official
  companion keyboard, while retaining a configurable standard landscape
  orientation.
- Status bar, battery area, terminal geometry, and future touch/input
  coordinates remain coherent after orientation changes.
- Official Tab5 companion keyboard support after its exact hardware protocol is
  verified from official documentation.
- Shared normalized key-event and terminal-input mapping layer that keeps the
  official keyboard independent of transport details.
- Input mapping for arrows, Home/End, Page Up/Down, Insert/Delete, function
  keys, Ctrl, Alt, and Escape.
- Application cursor/keypad mode affects emitted input sequences.
- Serial bridge and keyboard input share the same terminal input semantics.

Deferred outside Stage 5:

- USB-A keyboard implementation and runtime validation. The existing
  experimental probe is retained, but work resumes only after an explicit user
  request.

## Stage 6: Test And Regression Harness

Goal: keep compatibility from regressing as features are added.

Status: completed on 2026-06-12. R1-R4 automated infrastructure and hardware
runs passed, and the user reported no problem in the final physical check of
status-bar placement, A164 input, battery-level refresh, and display
responsiveness. Charging-state detection remains an explicitly deferred issue
outside the Stage 6 completion condition.

Deliverables:

- Consolidated regression manifest for the existing Stage 1-4 escape-sequence
  corpora and real-login-shell checks.
- Diagnostic-only, machine-readable terminal state summaries covering cursor,
  modes, margins, dimensions, and screen-buffer content or hashes.
- Host-side assertions that replay known byte streams and compare terminal
  replies and screen state with checked-in expectations.
- Manual app checklist: `clear`, `reset`, `tput`, `less`, `nano`, `vim`,
  `htop`, and `tmux` where available.
- One documented local workflow that runs deterministic checks and the
  available real-shell smoke tests while preserving detailed logs.
- Documented known gaps.

## Stage 7: Unicode Width And Cell Integrity

Goal: implement mainstream terminal column semantics before increasing glyph
coverage.

Status: U1 and U2 completed and hardware-regression tested on 2026-06-12.
U3 completed and hardware-regression tested on 2026-06-15.

U1 deliverables:

- Explicit single-width, wide-lead, and wide-continuation cells.
- Width-aware cursor rendering and row serialization.
- Wide-pair repair for erase, insert/delete character, redraw, and overwrite.
- Whole-glyph wrapping at the right margin.
- Whole-row scrolling that preserves wide-pair state.

U2 deliverables:

- Fixed Unicode 15.0.0 `wcwidth`-style zero/wide interval tables.
- East Asian wide/fullwidth characters consume two columns.
- Combining characters attach to the previous cell without cursor advance.
- Strict UTF-8 scalar validation and deterministic replacement for malformed
  sequences.
- Ambiguous-width characters use one column.

U3 deliverables:

- Deterministic fallback rendering for common Unicode box-drawing characters:
  light, heavy, and rounded single-line corners, lines, tees, and crossings.
- Deterministic fallback rendering for common block elements used by progress
  bars and text charts: fractional left/lower blocks, half blocks, full block,
  and light/medium/dark shade.
- A `stage7-unicode-graphics` corpus with state/cell assertions and an
  optional framebuffer screenshot path for pixel-level review.

Current scope limit: this stage establishes layout and a small practical
graphics fallback set, not comprehensive glyph coverage, bidirectional text,
complex shaping, or unlimited grapheme clusters.

## Stage 8: Terminal Protocol Readiness

Goal: add standards-aligned protocol responses that help future mature login
transports without changing the accepted raw UART login behavior.

T1 deliverables:

- xterm text-area size query `CSI 18 t`.
- Response `CSI 8;<rows>;<cols>t` generated from the current terminal cell
  geometry.
- Regression coverage for the response bytes.
- Documentation that raw UART login does not automatically consume this query;
  current serial use continues to rely on the Module LLM profile's persistent
  `stty rows 32 cols 64`.

Status on 2026-06-15: T1 completed and hardware-regression tested. The
`stage8-protocol` corpus checks `CSI 18 t` response bytes.

T2 deliverables:

- Read-only terminal geometry snapshot API for future transport layers.
- Snapshot includes terminal viewport origin/size, rendered grid size, cell
  size, and logical rows/columns.
- Existing `CSI 18 t` response path uses the shared geometry snapshot.
- No change to raw UART login behavior and no firmware-side `stty` injection.

Status on 2026-06-15: T2 implemented and locally build-checked, not
hardware-validated. Validation is deferred until a mature login transport or
PTY-backed integration consumes this API.

Stage 8 completion status on 2026-06-15: complete. T1 is hardware-regression
tested. T2 is preparatory API work, so its hardware validation is intentionally
deferred under the project rule for advanced-feature groundwork.

## Stage 9: TUI And Input Compatibility Hardening

Goal: make the accepted `xterm-256color`, `64x32`, 921600 UART terminal more
trustworthy with common Linux text user interfaces and both physical keyboard
paths, without prematurely starting SSH/Telnet/PPP transport work.

Deliverables:

- A replayable TUI matrix command for installed shell and full-screen tools,
  with missing programs reported as skipped rather than terminal failures.
- Physical USB-A and A164 keyboard semantic captures through `cat -vET`.
- Explicit records for transport-specific differences, unavailable keys, and
  skipped applications.
- No speculative terminal-core broadening: only real failures found by the
  matrix should create fixes.

Status on 2026-06-18: complete. `tools/tab5.ps1 tui-matrix` passed installed
`clear`, `reset`, `tput`, `less`, `vim`, `htop`, `top`, and `whiptail`; `tmux`,
`screen`, `dialog`, `btop`, and `nano` were skipped because they were not
installed on the current Module LLM image. `tools/tab5.ps1 key-capture`
captured USB-A printable/control/navigation/function-key behavior and A164
printable/control/arrow/Delete/Ctrl/Alt behavior. No accepted real failure
required a firmware or script fix. A164 Home/End/PageUp/PageDown/Insert/F1-F4
are recorded as unavailable on stock A164 firmware unless future official
documentation or firmware adds a combo.

## Stage 10: Render Performance And Interaction Latency

Goal: improve human-visible responsiveness of the accepted raw UART terminal
before opening larger network/PTY transport work.

Initial deliverables:

- Batch host-output spans through the terminal core without repainting the
  cursor around every byte.
- Keep UART draining bounded so USB, keyboard input, status refresh, and
  `M5.update()` continue to run.
- Establish repeatable output workloads for later before/after comparison.
- Plan dirty-row or dirty-rectangle rendering only if the first batching change
  is insufficient.

Status on 2026-06-18: completed. P1 implemented bounded byte-span batching through
`terminal::writeBytes()` and the login-UART drain path. P2 added
`tools/tab5.ps1 render-latency`, a repeatable real login-shell burst workload
that records timing and line-integrity JSON logs. The first P2 measurements
showed that a 64-line burst is complete but slow, while 120-line and 240-line
bursts expose output loss/corruption.

P3 first implementation added deferred dirty-row rendering inside
`terminal::writeBytes()`, increased the login-UART hardware RX buffer to 32
KiB, and added a 32 KiB software RX ring so the main loop can drain UART input
faster than it renders. Hardware measurements recovered the 120-line workload
to `120/120` unique lines and improved the 240-line workload to `239/240`, but
render throughput remains slow and a later raw CDC mirror capture showed a
corrupted early line. The next Stage 10 work should separate CDC mirror
integrity from final framebuffer/render evidence, then continue throughput
optimization from that cleaner measurement boundary.

P4 isolated the measurement path and fixed a real UART receive failure. The
firmware now exposes `TAB5PIPE` counters, exact-line host assertions, expected
marker-bounded byte counts, actual HardwareSerial RX buffer size, and UART
driver error counters. A failing run showed `uart_fifo_ovf=1`, so the login
UART is now drained from `HardwareSerial.onReceive(..., false)` into the 32
KiB software RX ring. Final `64x64`, `120x64`, and `240x64` workloads passed
exact-line checks with expected-byte delta `0`, no UART errors, and no software
drops.

P5 separated optional CDC debug mirroring from firmware render-pipeline timing.
The firmware now supports runtime `render-mirror?` and `render-mirror=0/1`;
`tools/tab5.ps1 render-latency` defaults to mirror-disabled counter polling and
uses `-EnableMirror` only for explicit CDC mirror integrity checks. Disabling
the mirror did not materially improve the `240x64` metric, which still became
pipeline-quiescent in about `17.9s`. Heavy mirror-enabled workloads exposed a
separate `HWCDC` diagnostic risk, so the next milestone should improve row
rendering/LCD update cost rather than CDC throughput.

The planned row-rendering throughput milestone became P6 and is now complete.
The accepted `240x64` burst remains correct and now reaches pipeline
quiescence in about `1.1s` with clean counters.

P6 first increment made the formal render batch configurable as
`LOGIN_UART_RENDER_BYTES_PER_LOOP` and raised the default to `256` bytes. The
second increment added fixed-cell row background prefill through
`TERMINAL_FIXED_ROW_BACKGROUND_PREFILL`, so common same-background rows are
filled in contiguous runs before glyph drawing instead of filling every cell
individually. The third increment added
`TERMINAL_TRANSPARENT_TEXT_AFTER_ROW_PREFILL`, avoiding opaque text-background
fills after row prefill while preserving anti-aliased edge blending through
M5GFX base color. A mirror-disabled `240x64` run now reaches pipeline
quiescence in `16.250s` with no UART errors or software drops. The fourth
increment added `TERMINAL_DEFER_SCROLL_DURING_WRITE_BYTES`, so batched scrolls
update the logical cell buffer and dirty the final visible region instead of
moving the physical framebuffer for every line. A mirror-disabled `240x64` run
reached pipeline quiescence in `8.078s` with no UART errors or software drops.
The fifth increment retuned the batch size to a conservative `320` bytes:
`384`, `512`, and `1024` were faster in some runs but each exposed UART FIFO
overflow in at least one validation path after stricter pipeline-error
checking. With `320`, `120x64` completed twice in `3.359s` and `3.453s`, and
`240x64` completed in `6.782s` with clean counters; the regression corpus still
passes. Further work should be more selective and should preserve the clean
receive counters and regression results.

The sixth increment added terminal write transactions and formal-login
backlog-aware render deferral. The UART bridge can now parse multiple software
RX batches into the logical screen while delaying physical row flushes until
the backlog clears or `LOGIN_UART_RENDER_DEFER_MAX_MS` expires. This avoids
repainting the same scrolled region for every 320-byte batch. With this change,
`120x64` reached pipeline quiescence in `0.640s`, `240x64` in `1.094s`, and
short mirror-enabled `64x64` passed exact integrity in `0.156s`; the regression
corpus still passes.

Stage 10 is closed after the sixth increment. No Stage 10 P7 is currently
planned; further renderer or transport work should be opened under a new stage
unless a regression or fresh performance target explicitly reopens this one.
