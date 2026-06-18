# Current Work

This file is the compact source of truth for the current implementation stage,
its checkpoint, and its milestones. Update it whenever the active stage or
milestone changes.

## Working Principle

Use repo-local scripts for repeatable deterministic operations when they reduce
conversation output and repeated model work. Builds, flashing, serial replay,
log capture, and result extraction should normally run through scripts such as
`tools/tab5.ps1`, with full logs stored locally and only summaries returned.

Keep direct commands when the operation is short and one-off. Scripts are not a
substitute for source inspection, architecture reasoning, or review.

For display changes, use the debug framebuffer capture before requesting a
camera photograph when exact final pixels are sufficient evidence. Keep manual
or optical inspection for panel brightness, color cast, viewing angle, physical
orientation, mounting, ghosting, and bad-pixel checks. The capture workflow is
documented in `docs/screen_capture.md`.

Long PlatformIO builds must use the detached worker in `tools/tab5.ps1`.
Build incidents, evidence, and confirmed mitigations are maintained in
`docs/build_troubleshooting.md`; extend that record when a new failure pattern
or discriminating test is found.

Groundwork for later advanced features should not consume hardware validation
time by default. For this kind of preparatory work, implement the narrow
internal capability, document what it does and which future feature will use it,
and record the status as implemented but unverified. Validate it later together
with the advanced feature that makes it user-visible. Continue to run live
device checks for current UART-login behavior, formal firmware changes,
user-visible UI/input behavior, and high-regression-risk changes.

## Completed Checkpoint

Completed through Stage 9:

- Stages 1 through 4 of `terminal_implementation_plan.md`.
- Deterministic CDC tests for the implemented parser/screen features.
- Real Module LLM login-shell application smoke for the available tools.
- DejaVu18 font face retained with formal fixed-cell rendering at `64x32`.
- Formal rendering later returned to fixed 18x20 cells for performance while
  retaining the DejaVu18 font face and the proportional preview environment.
- Formal `tab5_min_uart_terminal` image rebuilt, flashed, and verified.
- Login-shell probe returned `shell-path-ok: m5stack-LLM`.
- Stage 5 shared input mapping, official A164 I2C keyboard support,
  keyboard-mounted orientation, and 921600 login-UART integration.
- The user completed the remaining practical A164 and integration checks on
  2026-06-12 and reported no observed problem.
- Stage 6 regression harness, Stage 7 Unicode width/graphics fallback, and
  Stage 8 protocol-readiness work are complete at their documented validation
  levels.
- USB-A keyboard input has been promoted into the default formal
  `tab5_min_uart_terminal` firmware after coexistence, reconnect, `catv`,
  full-screen `less`, and shell-probe validation with the official A164
  keyboard path still enabled.
- Stage 9 TUI and input compatibility hardening is complete at the documented
  validation level. No terminal-core or input-mapper fix was required by the
  Stage 9 matrix.

Do not restart completed font selection, Stage 1-4 deterministic tests, or
Stage 9 keyboard capture unless new work causes a regression or a new keyboard
transport is added.

## Completed Mainline Stage: Stage 9 TUI And Input Compatibility Hardening

Goal: make the accepted `xterm-256color`, `64x32`, 921600 UART terminal more
trustworthy with common Linux text user interfaces and both physical keyboard
paths, without prematurely starting SSH/Telnet/PPP transport work.

Status: completed on 2026-06-18. V1 and V2 are complete at first-pass
validation level. V3 found no real failure requiring a firmware or script fix.
V4 records the validated and unavailable behaviors in this file,
`terminal_validation.md`, `terminal_known_gaps.md`, and `HANDOFF.md`. Any future
validation window that needs physical keyboard input must still be announced
before the command starts.

### Milestone V1: TUI Validation Matrix

- Keep the existing Stage 6 `app-smoke` behavior stable.
- Add a separate Stage 9 TUI matrix command that tests a broader set of
  installed applications and skips missing programs.
- Include known-safe automated candidates first: `clear`, `reset`, `tput`,
  `less`, `vim`, `htop`, `top`, `tmux`, `screen`, `dialog`, `whiptail`,
  `btop`, and `nano`.
- Record pass, skip, fail, and timeout explicitly so missing packages are not
  confused with terminal failures.

Exit condition: one local command runs the Stage 9 TUI matrix against the
current formal firmware and leaves the shell recoverable.

Status: completed on 2026-06-18. `tools/tab5.ps1 tui-matrix -Port COM3`
passed on the default formal firmware. Installed applications `clear`,
`reset`, `tput`, `less`, `vim`, `htop`, `top`, and `whiptail` returned
`ok rc=0`. Missing candidates `tmux`, `screen`, `dialog`, `btop`, and `nano`
were reported as skipped. A final shell probe returned
`shell-path-ok: m5stack-LLM`.

### Milestone V2: Keyboard Semantic Matrix

- Capture important key sequences from both USB-A keyboard and official A164
  keyboard through `cat -v`.
- Cover printable keys, Enter, Backspace, Tab, Escape, arrows, Home/End,
  Page Up/Down, Delete, Insert, F1-F12 where available, Ctrl/Alt/Shift
  combinations, and application cursor mode.
- Keep manual involvement explicit. If a test needs the user to press physical
  keys, tell the user at the start of that work.

Exit condition: key behavior differences are documented, and any mapper fixes
gain deterministic regression coverage.

Status: completed at first-pass level on 2026-06-18. Added
`tools/capture_keyboard_semantics.py` and `tools/tab5.ps1 key-capture` to run a
raw `cat -vET` capture with automatic tty restore. USB-A normal mode captured
printable text, Enter, Tab, Backspace, Escape, arrows, PageUp/PageDown,
Home/End, Delete, Insert, F1-F4, Ctrl+A, Ctrl+E, and Alt+x. USB-A application
cursor mode captured application arrow sequences and Home/End. A164 normal
mode captured printable text, Enter, Tab, Backspace, Escape, arrows, Delete,
Ctrl+A, Ctrl+E, and Alt+x. A164 application cursor mode captured application
arrow sequences. A164 Home/End/PageUp/PageDown/Insert/F1-F4 were not captured.
The official product page and user manual only document Sym/Aa/Ctrl/Alt as
special combination keys, and the local M5Unit-KEYBOARD HID table maps Del and
arrows but does not include F1-F4 or Home/Page/Insert usages. Treat those keys
as unavailable on stock A164 firmware unless later M5Stack documentation or
firmware proves an official combo. No mapper fix was required by these
captures. Follow-up shell probes returned `shell-path-ok: m5stack-LLM`.

### Milestone V3: Fix Only Real Failures

- Do not broaden terminal-core scope speculatively.
- If the Stage 9 matrix exposes raw escape leakage, bad screen restore, wrong
  key sequences, or stuck TTY state, fix that specific behavior and add a
  regression case.

Exit condition: every accepted Stage 9 fix has a replayable script or
diagnostic check.

Status: completed on 2026-06-18 with no accepted fix required. The TUI matrix
and keyboard semantic captures did not expose raw escape leakage, bad screen
restore, wrong mapper output, or stuck TTY state that needed code changes.

### Milestone V4: Stage 9 Exit Record

- Update `terminal_known_gaps.md` with exactly which TUI and keyboard behaviors
  are validated.
- Keep uninstalled applications and untested input combinations explicit.
- Commit and push the checkpoint after the stage is accepted.

Exit condition: Stage 9 has a concise validation record and no hidden manual
requirements.

Status: completed on 2026-06-18. The exit record is this Stage 9 section plus
the synchronized summaries in `terminal_validation.md`, `terminal_known_gaps.md`,
and `HANDOFF.md`. No hidden manual requirement remains; uninstalled Linux
programs and A164-unavailable keys are explicitly recorded.

## Pending Mainline Selection

No active mainline stage is open after Stage 9. Deferred or future work remains
available, including mature-login transport research, touch/mouse reporting,
broader Unicode glyph coverage, and any future official/USB/BLE keyboard
transport expansion. Start the next stage only after selecting scope.

## Completed Mainline Stage: Stage 6 Test And Regression Harness

Goal: make terminal compatibility changes repeatably testable and prevent
already accepted display, input, and Linux integration behavior from
regressing.

Status: completed on 2026-06-12. Automated regression passed, the formal
firmware was restored, and the user reported no problem in the final physical
checks. USB keyboard support was later resumed as a separate follow-up and does
not change the original Stage 5 acceptance result.

Existing groundwork:

- `tools/send_terminal_test.py` replays deterministic Stage 1-4 terminal byte
  streams through the CDC injection build.
- `tools/send_login_shell_demo.py` exercises the real login shell and validates
  selected markers and replies. Its `recover` demo is wrapped by
  `tools/tab5.ps1 recover` for recovering a shell/tty after an interrupted
  interactive test.
- `tools/send_login_shell_app_smoke.py` runs real terminal applications and
  records return markers.
- `tools/tab5.ps1` provides the common build, flash, serial, probe, and test
  entry points.

All R1-R5 milestones are complete. Charging-state detection remains an
explicitly unresolved known issue and was excluded from Stage 6 acceptance.
A dedicated automatic probe now exists for collecting evidence, but no
production charging-state rule has been accepted yet.

### Milestone R1: Baseline And Manifest

- Record the accepted `64x32`, DejaVu18 fixed-cell, 921600,
  `xterm-256color`, A164, and keyboard-mounted-orientation baseline.
- Inventory existing deterministic corpora and real-app smoke tests.
- Define which checks are automatic, diagnostic-only, or require visual
  confirmation.

Exit condition: one manifest identifies every current regression check, its
required firmware environment, and its pass criterion.

Status: completed. `tests/terminal_regression_manifest.json` records the
accepted baseline and Stage 1-4 assertions; `docs/stage6_regression.md`
classifies automatic and manual checks.

### Milestone R2: Machine-Readable Screen State

- Add a diagnostic-only CDC request that cannot enter the production keyboard
  path.
- Report terminal dimensions, cursor position, margins, important modes, and
  stable screen-buffer summaries or hashes.
- Keep diagnostic reporting outside normal rendering and UART timing paths.

Exit condition: a host script can inspect terminal state without relying on a
screen photograph.

Status: completed. The diagnostic-only `tab5_terminal_regression` environment
implements private OSC 777 state, row, and cell queries. Formal firmware leaves
the diagnostic macro disabled. A later Stage 6 infrastructure extension added
diagnostic RGB565 framebuffer capture and host-side PNG/pixel comparison for
checks that require final rendered pixels rather than only the screen model.

### Milestone R3: Automated Assertions

- Replay the existing escape-sequence corpora and compare resulting screen
  state and terminal replies with checked-in expectations.
- Cover wrapping, scrolling, erase/insert/delete, scroll regions, alternate
  screen, SGR colors, UTF-8 fallback, and cursor/application modes.
- Save concise pass/fail output and detailed logs locally.

Exit condition: deterministic parser and screen regressions fail with a useful
location and expected/actual summary.

Status: completed. `tools/run_terminal_regression.py` passed all four existing
Stage 1-4 corpora on physical Tab5 hardware. The first run correctly exposed
one bad tab-stop expectation; after review and correction, the rerun passed
4/4.

### Milestone R4: Real-Application Regression

- Run the available `clear`, `reset`, `tput`, `less`, `vim`, and `htop`
  checks through the Module LLM login shell; include `nano` and `tmux` only
  where installed.
- Confirm the status bar is not damaged and the A164, UART receive, display,
  and battery polling remain responsive.

Exit condition: a single documented command runs the available real-shell
smoke suite and preserves logs.

Status: automated portion completed. The restored formal firmware passed the
login-shell probe, and `clear`, `reset`, `tput`, `less`, `vim`, and `htop`
returned `rc=0`; `nano` was explicitly skipped because it is not installed.
`tools/run_stage6_regression.ps1` now performs the diagnostic run, restores the
formal image, and runs the shell checks. The complete script was exercised with
`-SkipBuild` on physical hardware and passed end to end.

### Milestone R5: Known Gaps And Stage Exit

- Record unsupported behavior instead of silently treating it as compatible.
- Prioritize compatibility additions only when real applications expose them.
- Keep current known gaps explicit, including CJK double-width/combining text
  and any input-device behavior that is not yet validated.

Status: completed. `docs/terminal_known_gaps.md` records the initial gap
inventory. On 2026-06-12 the user reported no problem with status-bar
placement, A164 input, battery-level refresh, or display responsiveness.
Charging-state detection remains deferred because the available status signal
did not reliably follow cable insertion and removal.

Stage 6 completion condition: one local workflow runs deterministic assertions
and real-shell smoke, produces machine-readable results, and documents any
remaining manual checks and known gaps.

Completion result: satisfied on 2026-06-12.

## Completed Mainline Stage: Stage 7 Unicode Width And Cell Integrity

Goal: make UTF-8 text obey mainstream terminal column rules before expanding
font coverage.

Status: U1 and U2 completed on 2026-06-12. U3 completed on 2026-06-15 after the
user asked to continue the plan while skipping deferred items. The diagnostic
firmware passed six deterministic corpora on physical Tab5 hardware, a
framebuffer screenshot confirmed the new Unicode graphics fallback pixels, the
formal firmware was restored, and the login-shell probe passed.

### Milestone U1: Width-Aware Cell Model

- Each cell is explicitly `Single`, `Wide`, or `Continuation`.
- A wide glyph owns a lead cell and one continuation cell.
- Writing, backspace, erase, insert/delete character, redraw, alternate-screen
  storage, and scrolling preserve or repair wide-cell pairs.
- A width-2 glyph at the final column wraps as a unit instead of splitting
  across rows.
- Diagnostic cell snapshots expose width as `0` continuation, `1` single, and
  `2` wide lead.

Status: completed and hardware-regression tested.

### Milestone U2: Fixed Unicode Column Width

- Width classification is fixed to Unicode 15.0.0 using generated `wcwidth`
  zero-width and East Asian wide intervals.
- Combining characters append to the preceding cell without advancing the
  cursor. Each cell stores up to eight UTF-8 bytes, enough for the base plus
  common combining sequences and emoji modifiers.
- UTF-8 validation rejects overlong encodings, surrogates, values above
  `U+10FFFF`, and malformed continuation bytes without corrupting parser state.
- Ambiguous-width characters remain one column, matching the common
  non-CJK-locale terminal policy.
- `tools/generate_unicode_width.py` regenerates the checked-in lookup table;
  the firmware has no runtime Python or `wcwidth` dependency.

Status: completed and hardware-regression tested.

### Milestone U3: Common Unicode Graphics Fallback

- Render common Unicode box-drawing characters used by modern TUIs instead of
  falling back to `?`.
- Cover light, heavy, and rounded single-line box corners, tees, crossings,
  horizontal lines, and vertical lines.
- Render common block elements used for progress bars, meters, and simple
  charts, including fractional left/lower blocks, half blocks, full block, and
  light/medium/dark shade.
- Keep these as deterministic renderer fallbacks; do not claim comprehensive
  Unicode font coverage.
- Add `stage7-unicode-graphics` to the CDC test corpus and regression
  manifest. Use framebuffer capture for final-pixel inspection when the board
  is available.

Status: completed and hardware-regression tested on 2026-06-15. The
`stage7-unicode-graphics` corpus passed together with Stage 1-4 and
`stage7-unicode` at 6/6. The captured framebuffer was 1280x720 RGB565LE with
CRC32 `6D8657DB`; the restored formal firmware then returned
`shell-path-ok: m5stack-LLM`.

Further Stage 7 grapheme-cluster, ZWJ emoji, bidirectional text, complex
shaping, or broad CJK font coverage remains outside the active scope. Keep it
separate from the completed U1/U2 column model and the U3 graphics fallback.

## Completed Mainline Stage: Stage 8 Terminal Protocol Readiness

Goal: add small standards-aligned protocol capabilities that prepare the Tab5
terminal for future mature login transports such as SSH, Telnet, or PTY-backed
serial helpers, without changing the accepted raw UART login behavior.

Raw UART login remains fixed at the Module LLM profile's persistent
`stty rows 32 cols 64`. Stage 8 must not inject visible shell commands or
expect ordinary serial getty/login to query terminal size automatically.

### Milestone T1: xterm Text-Area Size Report

- Implement xterm window-manipulation query `CSI 18 t`.
- Reply with `CSI 8;<rows>;<cols>t`, using the current logical terminal rows
  and columns.
- Validate this through the CDC regression response path.
- Document that this is a standard xterm-style response for future
  SSH/Telnet/PTY integration, not a raw-UART automatic resize mechanism.

Status: completed and hardware-regression tested on 2026-06-15. Both
`tab5_terminal_regression` and `tab5_min_uart_terminal` built successfully.
The regression firmware passed all seven deterministic cases, including
`stage8-protocol`, and the restored formal firmware returned
`shell-path-ok: m5stack-LLM`.

### Milestone T2: Terminal Geometry Snapshot API

- Provide a single read-only terminal geometry snapshot for future transport
  layers.
- Include viewport origin/size, rendered grid size, cell size, and character
  rows/columns.
- Keep the existing raw UART login behavior unchanged and do not inject `stty`
  commands.
- Reuse the snapshot from the existing `CSI 18 t` response path so later
  protocol work has one geometry source.

Status: implemented and locally build-checked on 2026-06-15, not
hardware-validated. Both `tab5_min_uart_terminal` and
`tab5_terminal_regression` built successfully. This is preparatory API work for
future SSH/Telnet/PTY-backed transports and is intentionally left for validation
when one of those advanced features is implemented.

Stage completion result: satisfied on 2026-06-15. Stage 8 is complete because
all defined deliverables are implemented at the documented validation level:
T1 was hardware-regression tested, and T2 is preparatory API work intentionally
left for later validation with its future transport consumer. Stage 9 was later
opened as TUI and input compatibility hardening.

## Completed Mainline Stage: Stage 5 Input And Integration

Goal: make the official Tab5 companion keyboard production-ready while keeping
terminal key semantics transport-independent for future input devices.

Status: completed on 2026-06-12 after user-reported physical keyboard and
integration testing found no observed problem.

The shared input contract and mapper are implemented, and the official I2C
keyboard driver is present in the formal firmware. USB keyboard support was
excluded from the original Stage 5 completion condition, then resumed and
promoted into the default formal firmware after targeted hardening and physical
validation.

Stage 5 display prerequisite:

- Screen orientation is encapsulated by `display_orientation`.
- The formal default is `KeyboardMountedLandscape`, a 180-degree turn from the
  original landscape direction so the display matches the official companion
  keyboard installation direction.
- Both landscape directions retain the same `64x32` terminal geometry. One
  character-row-height blank margin is reserved above and below the terminal
  content, outside its scroll rectangle.
- Any future
  runtime orientation selector must apply the orientation before rebuilding and
  redrawing UI regions.

Terminal-size synchronization:

- A raw UART login has no standard unsolicited equivalent of SSH PTY window
  resizing. The Module LLM ttyS1 login now persistently applies
  `stty rows 32 cols 64`.
- The same ttyS1-only login profile persistently sets
  `TERM=xterm-256color` and `COLORTERM=truecolor`. Module reboot verification
  passed with `tput colors=256`, and the user visually confirmed colored
  `htop` output on Tab5 on 2026-06-12.
- Adding the xterm `CSI 18 t` query and `CSI 8;32;64t` response is small
  terminal-core work, but ordinary serial shells do not issue that query or
  apply its result automatically.
- Automatic dynamic synchronization therefore also needs a Linux-side helper
  or login hook. Do not inject a visible `stty` command into the user's shell
  as a terminal protocol.

Hardware paths:

- USB keyboard: HID boot-keyboard input on the Tab5 USB-A host port is enabled
  in the default formal firmware. The isolated `tab5_usb_keyboard_probe`
  environment remains for diagnostics. Basic input, reconnect, Shift/Ctrl
  cases, repeat, at least 3-key printable groups, and a full-screen `less`
  exit have passed. Full NKRO and broad application coverage are not claimed.
- Official companion keyboard: A164 Tab5 Keyboard on Ext.Port1, I2C address
  `0x6D`, `SDA=G0`, `SCL=G1`, active-low interrupt `G50`. The formal firmware
  uses official `M5Unit-KEYBOARD` 0.1.0 in HID mode.

Architecture constraints:

- Device drivers emit normalized key events; they do not directly construct
  shell byte sequences.
- A shared input mapper converts normalized events into UTF-8/control bytes and
  VT/xterm key sequences.
- The mapper must observe terminal modes such as application cursor keys and
  application keypad mode.
- USB CDC debug injection remains isolated from production keyboard input.
- Each hardware path remains compile-time gated until its runtime validation
  passes.
- Keep device tasks nonblocking and queue-based so rendering and UART service
  are not starved.

### Milestone I1: Shared Input Contract

- Define normalized key, modifier, press/release, and repeat events.
- Define the transport-independent mapper API and output queue.
- Move existing USB scancode-to-sequence behavior out of
  `usb_keyboard_probe.cpp`.
- Add host-testable mapping cases for printable keys, Ctrl, Alt, Escape,
  navigation keys, function keys, and terminal application modes.

Exit condition: mapping behavior can be tested without either physical
keyboard.

Status: implemented. `input_mapper` covers printable/control keys, Alt prefix,
navigation keys, F1-F12, keypad keys, and application cursor/keypad modes.
Compile-time assertions validate representative byte sequences.

### Milestone I2: USB Keyboard

- Validate enumeration, connect/disconnect, modifier handling, rollover, and
  repeat behavior on a physical USB keyboard.
- Route normalized USB events through the shared mapper.
- Verify login, shell editing, `cat -v`, and at least one full-screen
  application.

Exit condition: USB keyboard works through reconnects and no longer depends on
probe-only direct byte translation.

Status: resumed by user request on 2026-06-15 and promoted into the default
formal firmware on 2026-06-16. The HID modifier/usage mapping is shared by the
official A164 keyboard and USB boot-keyboard driver. The USB path emits
normalized `input::KeyEvent` values, including software repeat for held keys,
and the main loop routes those events through the same xterm-aware
`input_mapper` path as the official keyboard. The isolated
`tab5_usb_keyboard_probe` environment remains available for diagnostics, but
the formal `tab5_min_uart_terminal` image now enables USB-A keyboard input by
default.

USB input hardening status: in progress. The first hardening change moves USB
software repeat into the USB client task timer so held keys can repeat even
when a boot keyboard only sends HID reports on state changes. Disconnect now
clears the active repeat state and emits release events before resetting the
probe state. Runtime reconnect validation passed through extended boot-log
capture, and a `cat -v` capture showed Shift printable input, left/right arrow
escape sequences, Backspace repeat, and Ctrl+C through the USB keyboard path.
Rollover, full-screen app behavior, and formal-firmware enablement remain open.

Formal integration status: completed. The previous opt-in
`tab5_min_uart_terminal_usb_keyboard` environment is retained as a compatibility
alias for the default formal build; `tab5_min_uart_terminal` now enables the USB
keyboard path without disabling the official A164 keyboard. The default image
built, flashed, booted with A164 ready, started the USB host task, passed the
login shell probe, captured physical USB keyboard input through `catv`, exited
full-screen `less` with USB-keyboard `q`, and passed a final shell probe. A
previous black-screen incident in the opt-in coexistence build was mitigated by
`display_boot_guard`, which checks for a usable M5GFX panel immediately after
`M5.begin()` and restarts up to two times if display autodetect fails. This is
a recovery guard, not proof of the M5GFX/Tab5 display-autodetect root cause.

### Milestone I3: Official Companion Keyboard

- Verify the exact official hardware and protocol from primary documentation.
- Implement a separately gated driver that emits the same normalized events.
- Verify printable keys, modifiers, navigation/function keys available on the
  device, disconnect/fault behavior, and coexistence with status polling.

Exit condition: the official keyboard can operate the login shell through the
shared mapper. The paused USB path can adopt that mapper later without changing
official-keyboard behavior.

Status: completed using official `M5Unit-KEYBOARD` 0.1.0 in HID mode.
The formal firmware detected address `0x6D`, firmware `0x01`, and configured the
G50 falling-edge interrupt. Physical key-to-shell validation passed for
printable text and Enter: typing `printf 'tab5-keyboard-ok\n'` on the A164
produced `tab5-keyboard-ok` on its own output line. On 2026-06-12 the user
reported that the remaining practical keyboard tests showed no problem. The
current HID driver still tracks one non-modifier key at a time through
`last_hid_keycode`; full simultaneous non-modifier rollover is not guaranteed
and remains a documented limitation for future input work.

### Milestone I4: Integration

- Confirm the official keyboard path does not block UART receive, display
  refresh, or battery status updates.
- Run the input section of `terminal_validation.md` and record known hardware
  limitations.

Status: login-UART integration now supports runtime
115200/230400/460800/921600 selection over a private USB CDC management path.
The helper coordinates the current Module LLM `/dev/ttyS1` shell, and 460800
plus 921600 real shell probes passed. Optional Tab5 NVS persistence and reset
restoration were validated first at 460800. The Module LLM boot-time M5Bus
login UART was later permanently set to 921600 8N1 and reboot-validated. Tab5
was therefore saved at 921600, hard-reset, and strict shell-probed
successfully. The installed hardware baseline is now 921600 on both endpoints.
The user reported no problem after the remaining practical integration tests
on 2026-06-12, completing I4.

Stage 5 completion condition: the shared mapping is stable, the official A164
keyboard has reached its device-specific exit condition, and its integration
validation has been recorded. The keyboard-mounted display direction must also
be visually confirmed with the physical installation, including status-bar
position, battery-area position, terminal geometry, and absence of mirroring.
USB keyboard support was explicitly excluded from this condition. The resumed
USB work is a follow-up path and must pass the I2 exit condition before it is
enabled in the formal firmware by default.

## Progress Log

- 2026-06-18: Stage 9 opened as TUI and input compatibility hardening. Added a
  Stage 9 TUI matrix profile to `tools/send_login_shell_app_smoke.py` and a
  `tools/tab5.ps1 tui-matrix` wrapper. The command requires no physical
  keyboard input and skips missing programs. On the default formal firmware it
  passed `clear`, `reset`, `tput`, `less`, `vim`, `htop`, `top`, and
  `whiptail`; skipped missing `tmux`, `screen`, `dialog`, `btop`, and `nano`;
  and the follow-up shell probe returned `shell-path-ok: m5stack-LLM`. Next
  Stage 9 work is the V2 physical keyboard semantic matrix; any run that needs
  the user to press USB/A164 keys must announce that before the capture window.
- 2026-06-18: Stage 9 V2 physical keyboard semantic capture completed at
  first-pass level. Added `tools/capture_keyboard_semantics.py` and
  `tools/tab5.ps1 key-capture`. USB-A normal mode captured `abc123`, Enter
  `^M`, Tab `^I`, Backspace `^?`, Escape `^[`, normal arrows
  `^[[A/B/D/C`, PageUp/PageDown `^[[5~/^[[6~`, Home/End `^[[H/^[[F`,
  Delete/Insert `^[[3~/^[[2~`, F1-F4 `^[OP/OQ/OR/OS`, Ctrl+A `^A`,
  Ctrl+E `^E`, and Alt+x `^[x`. USB-A application cursor mode captured
  `^[OA/OB/OD/OC` and Home/End `^[OH/OF`. A164 normal mode captured
  printable text, Enter, Tab, Backspace, Escape, normal arrows, Delete
  `^[[3~`, Ctrl+A, Ctrl+E, and Alt+x. A164 application cursor mode captured
  application arrows. A164 Home/End/PageUp/PageDown/Insert/F1-F4 were not
  captured. Official documentation and the local M5Unit-KEYBOARD HID table do
  not expose those keys or official combos, so they are a stock A164 limitation
  rather than an untested user-layout issue. Final shell probe returned
  `shell-path-ok: m5stack-LLM`.
- 2026-06-16: USB-A keyboard was promoted into the default formal firmware.
  `platformio.ini` now separates common, formal, and debug build flags:
  formal builds enable `ENABLE_USB_KEYBOARD_PROBE=1`, while CDC injection and
  power-detect debug builds keep USB keyboard disabled. The previous
  `tab5_min_uart_terminal_usb_keyboard` env remains as a compatibility alias.
  `M5.Power.setExtOutput(true, m5::ext_USB)` is skipped for CDC injection and
  power-detect debug builds. Default `tab5_min_uart_terminal` rebuilt in
  32.5s after the final disconnect-path hardening, flashed to COM3, booted
  with USB host and A164 enabled, returned `shell-path-ok: m5stack-LLM`,
  captured `usb` plus control-key input through `catv`, exited `less` via
  USB-keyboard `q` with `rc=0`, and passed a final shell probe. A normal USB
  disconnect `ESP_ERR_INVALID_STATE` report-submit path now closes the keyboard
  without retry logging from the driver. The compatibility alias
  `tab5_min_uart_terminal_usb_keyboard` also built successfully in 381.1s.
  A later standard probe captured zero bytes because a previous interactive
  shell command was still pending; the new `tools/tab5.ps1 recover` command
  restored the tty with `recover-ok: m5stack-LLM`, and the following standard
  probe again returned `shell-path-ok: m5stack-LLM`. The next USB TUI coverage
  check also passed: after one missed-`q` attempt was recovered, `htop-usb`
  exited `htop` through USB-keyboard `q` with `rc=0`, followed by a successful
  shell probe.
- 2026-06-15: USB-A keyboard support was resumed. Added shared
  HID-keyboard mapping (`hid_keyboard_mapper`) so the official A164 keyboard
  and USB boot-keyboard probe use the same modifier/usage-to-`KeyCode`
  translation. Reworked `usb_keyboard_probe` to submit normalized key events
  instead of direct bytes, added basic software repeat for held USB keys, and
  changed the main input bridge so either official keyboard or USB probe events
  pass through the common `input_mapper`. `tab5_usb_keyboard_probe` built
  successfully in 505.3s and `tab5_min_uart_terminal` in 457.9s. The USB probe
  firmware was flashed to COM3, boot logging confirmed the host task is waiting
  for a HID boot keyboard, and the login shell probe still reached
  `m5stack-LLM`. The user then connected a USB keyboard to the USB-A port and
  confirmed successful input. At that point this was first-pass physical
  validation only; reconnect, modifiers, repeat, rollover, full-screen
  applications, and default-firmware enablement moved to the next USB input
  hardening stage. Current USB-A keyboard probe first-pass stage is closed.
- 2026-06-16: USB input hardening started. USB software repeat no longer
  depends on repeated identical HID reports; the USB client task now services
  repeat timing while polling host events. Disconnect also clears repeat state
  and emits release events for active keys. `tools/tab5.ps1 boot-log` can now
  use `-DurationSeconds` for longer USB attach/detach log captures.
- 2026-06-16: USB-A keyboard hardening validation advanced. A 45s boot-log
  capture recorded `keyboard connected`, `keyboard disconnected`, and
  reconnected keyboard events for VID `0x046a` PID `0x00b7`. The new
  `send_login_shell_demo.py --demo catv` mode captured real USB-keyboard input
  through the login shell: `Aa!`, `^[[D`, `^[[C`, Backspace repeat echo, and
  `^C`. Remaining USB work is rollover characterization, at least one
  full-screen application driven from the USB keyboard, and the formal
  firmware enablement/coexistence decision.
- 2026-06-16: USB keyboard full-screen and basic rollover checks advanced.
  `less-usb` started `less /etc/os-release`; the user exited it with USB-keyboard
  `q`, and the shell marker returned `rc=0`. A `catv` capture for simultaneous
  key groups showed `asd...jkl`, so at least 3-key printable groups reach the
  host through the boot-keyboard report path; full NKRO is not claimed. Added
  `tab5_min_uart_terminal_usb_keyboard` as an opt-in formal-terminal build that
  keeps A164 enabled while also enabling USB keyboard input.
- 2026-06-16: built and flashed `tab5_min_uart_terminal_usb_keyboard`. Boot log
  confirmed the USB host task and official A164 driver both started
  (`addr=0x6D fw=0x01 mode=HID irq=G50`), a replugged USB keyboard enumerated
  as VID `0x046a` PID `0x00b7`, and the login shell probe still reached
  `m5stack-LLM`. A `catv` run in this coexistence build captured physical USB
  keyboard input `usb` and `^C`. Leave default `tab5_min_uart_terminal` unchanged
  until the user accepts USB keyboard as a default production input.
- 2026-06-16: black-screen incident after the opt-in USB coexistence firmware.
  The user reported that the Tab5 showed nothing. The immediately prior
  coexistence boot log had contained an M5GFX panel-detection failure
  (`M5Tab5 display panel was not detected`). A direct flash attempt for the
  normal firmware first failed because the local build artifacts were missing,
  so `tab5_min_uart_terminal` was rebuilt in 360.2s, flashed to COM3, and
  boot-logged for 10s. The restored formal firmware showed normal
  `board_M5Tab5` autodetect, no panel-detection error, A164 ready, login UART
  `921600 persisted=saved`, and a successful strict shell probe returning
  `shell-path-ok: m5stack-LLM`. Do not flash the coexistence environment again
  during routine work until the display-init risk is investigated.
- 2026-06-16: added `display_boot_guard`, an application-level startup guard
  that runs immediately after `M5.begin()`. It checks that M5GFX exposes a
  non-null panel and plausible display dimensions, then clears its RTC retry
  counter on success. On failure it prints `[display]` diagnostics and
  `ESP.restart()`s up to two times. `tab5_min_uart_terminal` rebuilt
  successfully in 345.6s, `tab5_min_uart_terminal_usb_keyboard` rebuilt
  successfully in 390s, and the guarded normal firmware was flashed to COM3.
  A 10s boot log for the normal firmware showed no `[display]` retry and the
  strict shell probe again returned `shell-path-ok: m5stack-LLM`. The guarded
  USB coexistence firmware initially had only been compile-verified; later in
  the same session it passed targeted boot-log and input tests.
- 2026-06-16: guarded `tab5_min_uart_terminal_usb_keyboard` was flashed and
  tested. A 30s boot log reproduced the M5GFX panel-detection failure on the
  first boot, then `display_boot_guard` logged
  `[display] unusable after M5.begin: ... size=0x0 attempt=0/2`, restarted the
  board, and the second boot detected `ST7123 display`. A164 initialized,
  USB host started, and the shell probe returned `shell-path-ok: m5stack-LLM`.
  USB keyboard input in the guarded coexistence firmware then passed:
  `catv` captured `usb^C`; `less-usb` exited through USB-keyboard `q` and
  returned `rc=0`; a final shell probe succeeded. During deliberate USB-A
  unplug/replug, the host logged one `ESP_ERR_INVALID_STATE` submit failure and
  recovered by disconnecting/re-enumerating the keyboard. The USB report path
  was then hardened so callbacks request report resubmission and the client
  task performs it with short retry handling. `tab5_min_uart_terminal_usb_keyboard`
  rebuilt in 26.2s, was reflashed, passed a 45s boot/replug log, `catv`
  `usb^C`, `less-usb rc=0`, and shell probe. `tab5_min_uart_terminal` also
  rebuilt successfully in 20.9s. Remaining policy decision: whether/when to
  enable USB keyboard in the default formal firmware.
- 2026-06-15: dynamic charge-state detection was reopened only as a diagnostic
  effort. Added `tab5_power_detect_probe`, a RAM ring-buffer power logger,
  `tools/power_detect_probe.py`, and `tools/tab5.ps1 power-detect` so the user
  can plug/unplug cables while the device records samples locally and exports
  them after CDC reconnect. This does not yet solve charging-state detection;
  it produces evidence for choosing or rejecting a production heuristic.
- 2026-06-15: first automated power-detect run captured 361 samples over 180s
  with no drops. `api`/raw `CHG_STAT` changed only for 14 early samples and did
  not track repeated cable cycles. INA226 current split cleanly into two
  clusters: about `-979mA` and `+199mA`.
- 2026-06-15: after the first production heuristic, user validation showed the
  sign interpretation was backwards: the lightning disappeared when the charge
  cable was plugged in and appeared when unplugged. The status bar now treats
  strong negative INA226 current as charging, with hysteresis thresholds of
  `<-600mA` to enter and `>-300mA` to exit, requiring two consecutive 500ms
  samples before switching. `CHG_STAT`/`M5.Power.isCharging()` remain diagnostic
  signals only.
- 2026-06-15: Stage 8 was closed after T1 and T2 were completed at their
  documented validation levels. T1 was hardware-regression tested. T2 remains
  intentionally not hardware-validated because it is preparatory geometry API
  work for future mature transports. No Stage 9 scope is active yet.
- 2026-06-15: the user instructed Codex to save updated requirements,
  discovered issues, and new automation command notes before continuing the
  plan, while skipping explicitly deferred work. Stage 7 U3 was selected as
  the next non-deferred increment: common Unicode graphics fallback for
  box-drawing and block-element glyphs. Implemented the fallback renderer,
  added `stage7-unicode-graphics` to `tools/send_terminal_test.py` and the
  regression manifest, built both `tab5_terminal_regression` and
  `tab5_min_uart_terminal`, flashed the regression firmware, passed 6/6
  hardware regression, captured `stage7-unicode-graphics.png` with CRC32
  `6D8657DB`, restored formal firmware, and verified
  `shell-path-ok: m5stack-LLM`. USB-A keyboard support and dynamic charge-state
  detection remain skipped.
- 2026-06-15: after confirming that raw UART login does not automatically use
  xterm size queries, the user approved implementing `CSI 18 t` only as
  future protocol readiness for mature transports such as SSH/Telnet/PTY-backed
  helpers. It must not be used as a raw serial login resize mechanism.
  Implemented `CSI 18 t` -> `CSI 8;32;64t`, added `stage8-protocol`, built
  regression and formal firmware, passed hardware regression at 7/7, restored
  formal firmware, and verified `shell-path-ok: m5stack-LLM`.
- 2026-06-15: the first seven-case regression run dropped one read-only OSC 777
  diagnostic cell reply in the existing `stage7-unicode` case while
  `stage8-protocol` passed. The runner now retries read-only diagnostic
  queries before failing; after reflashing the regression firmware the complete
  run passed 7/7.
- 2026-06-12: Stage 7 U1/U2 completed. Added the explicit single/wide/
  continuation cell model, Unicode 15.0.0 width tables, zero-width combining
  attachment, strict UTF-8 validation, and wide-cell-safe editing. The new
  `stage7-unicode` corpus and all Stage 1-4 corpora passed 5/5 on hardware.
  Formal firmware was restored; shell probing and the installed full-screen
  app smoke suite passed.
- 2026-06-12: the user completed the final Stage 6 physical checks and reported
  no observed problem. Stage 6 was closed. Battery-level refresh was accepted;
  charging-state detection remains intentionally deferred and does not block
  completion. Stage 7 scope remains undecided.
- 2026-06-12: Stage 6 R1-R3 implemented. Added the
  `tab5_terminal_regression` environment, read-only terminal snapshots, OSC 777
  diagnostic queries, a structured regression manifest, and host assertions.
  Physical deterministic replay passed Stage 1-4 at 4/4 after correcting one
  reviewed tab-stop expectation.
- 2026-06-12: Stage 6 R4 automated checks passed after restoring formal
  firmware. The login shell probe returned `shell-path-ok: m5stack-LLM`;
  `clear`, `reset`, `tput`, `less`, `vim`, and `htop` returned `rc=0`, while
  missing `nano` was skipped. A one-command full regression/restoration script
  and the initial known-gap inventory were added. The one-command script was
  then run with cached artifacts and successfully flashed diagnostics, passed
  4/4 deterministic cases, restored formal firmware, passed the shell probe,
  and repeated the app smoke.
- 2026-06-12: the user reported no problem after the remaining physical A164
  and integration tests. Stage 5 was closed and the active mainline moved to
  Stage 6, Test And Regression Harness. Full simultaneous non-modifier rollover
  remains documented rather than claimed as supported.
- 2026-06-12: Module LLM ttyS1 terminal settings were made persistent and
  reboot-verified: `TERM=xterm-256color`, `COLORTERM=truecolor`, `32x64`, and
  `tput colors=256`. The user confirmed that `htop` displays in color on Tab5.
  This closed the host terminal-environment mismatch.
- 2026-06-12: USB keyboard support was deferred indefinitely by user request.
  Its existing experimental probe remains available, but it no longer blocks
  Stage 5 completion or the transition to Stage 6.
- 2026-06-08: formal terminal geometry changed from `64x34` to explicit
  `64x32`. The 640-pixel terminal content area is vertically centered below
  the 32-pixel status bar, leaving 24 pixels above and below on the current
  display. The formal image built, flashed to COM3 with verified hashes, and
  passed the `shell-path-ok: m5stack-LLM` probe at 921600.
- 2026-06-08: font work completed and formal UART firmware restored.
- 2026-06-08: Stage 5 input and integration opened at Milestone I1.
- 2026-06-08: display orientation encapsulated and defaulted to the
  keyboard-mounted 180-degree direction.
- 2026-06-08: rotated formal firmware built and flashed to COM3 with verified
  hashes; the Module LLM shell probe still returned
  `shell-path-ok: m5stack-LLM`. Physical orientation remains a visual check.
- 2026-06-08: official A164 keyboard driver added with G0/G1 I2C and G50 INT.
  Boot log confirmed `addr=0x6D fw=0x01 mode=HID irq=G50`; the UART shell probe
  continued to pass. USB keyboard work was paused by explicit user request.
- 2026-06-08: physical A164 input path validated through the Module LLM login
  shell. Printable command input and Enter worked; the command
  `printf 'tab5-keyboard-ok\n'` printed `tab5-keyboard-ok` on a separate line.
- 2026-06-08: runtime login-UART baud management added. Coordinated shell
  round trips passed at 460800 and 921600; NVS restore passed after a Tab5-only
  reset at 460800.
- 2026-06-08: Module LLM M5Bus login UART was permanently configured as
  921600 8N1 and retained it across a Linux reboot. Tab5 was then set to
  `active=921600 persisted=921600`, hard-reset, and strict shell-probed
  successfully. This is the current installed baseline.
- 2026-06-08: measured proportional rendering at about 64 printable bytes per
  second despite the 921600 UART. A normal printable byte caused three
  whole-row redraws through cursor erase, cell update, and cursor draw. The
  formal renderer was returned to fixed 18x20 cells without changing the
  DejaVu18 font face; proportional rendering remains an explicit preview.
- 2026-06-08: the fixed-cell formal firmware was built, flashed, and retained
  `active=921600 persisted=921600`; strict shell probing passed. The same
  64-character single-`printf` measurement improved from 1.000 second to
  0.016 second, about 62.5x, while retaining DejaVu18.
- 2026-06-08: Windows builds moved to a detached worker with persistent
  status/result files. The first recorded full build took 518.64 seconds; the
  cached validation build took 15.5 seconds. Incident method and evidence are
  maintained in `build_troubleshooting.md`.
