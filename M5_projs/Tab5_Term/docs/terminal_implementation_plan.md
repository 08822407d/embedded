# Terminal Implementation Plan

This project will grow terminal compatibility in layers. The goal is to keep
each step small enough to validate on the Tab5 while still moving toward a
practical `xterm-256color` subset.

Do not advertise a broader `TERM` value than the firmware can actually satisfy.
During early work, test with simple shell output first. Move toward
`xterm-256color` only after the parser, screen model, color attributes, and
input behavior have been validated.

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

Status on 2026-06-08: active mainline stage. Stage 4 and the accepted font
stage form the completed checkpoint. Detailed milestones and architecture
constraints are maintained in `current_work.md`.

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
- USB-A keyboard runtime validation.
- Shared normalized key-event and terminal-input mapping layer used by both
  keyboard transports.
- Input mapping for arrows, Home/End, Page Up/Down, Insert/Delete, function
  keys, Ctrl, Alt, and Escape.
- Application cursor/keypad mode affects emitted input sequences.
- Serial bridge and keyboard input share the same terminal input semantics.

## Stage 6: Test And Regression Harness

Goal: keep compatibility from regressing as features are added.

Status: next mainline stage after Stage 5 reaches its recorded completion
condition.

Deliverables:

- Small escape-sequence corpus that can be replayed over USB bridge.
- Manual app checklist: `clear`, `reset`, `tput`, `less`, `nano`, `vim`,
  `htop`, and `tmux` where available.
- Optional host-side script to send known byte streams and compare screen state
  summaries from diagnostic firmware.
- Documented known gaps.
