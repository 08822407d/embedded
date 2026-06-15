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
