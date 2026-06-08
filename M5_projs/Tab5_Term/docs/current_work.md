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

Long PlatformIO builds must use the detached worker in `tools/tab5.ps1`.
Build incidents, evidence, and confirmed mitigations are maintained in
`docs/build_troubleshooting.md`; extend that record when a new failure pattern
or discriminating test is found.

## Completed Checkpoint

Completed before Stage 5:

- Stages 1 through 4 of `terminal_implementation_plan.md`.
- Deterministic CDC tests for the implemented parser/screen features.
- Real Module LLM login-shell application smoke for the available tools.
- DejaVu18 font face retained with formal fixed-cell rendering at `64x32`.
- Formal rendering later returned to fixed 18x20 cells for performance while
  retaining the DejaVu18 font face and the proportional preview environment.
- Formal `tab5_min_uart_terminal` image rebuilt, flashed, and verified.
- Login-shell probe returned `shell-path-ok: m5stack-LLM`.

Do not restart completed font selection or Stage 1-4 deterministic tests unless
Stage 5 causes a regression.

## Active Mainline Stage: Stage 5 Input And Integration

Goal: support both the official Tab5 companion keyboard and ordinary USB HID
keyboards without coupling terminal key semantics to either transport.

Status: active. Stage 5 planning checkpoint created on 2026-06-08.

Current milestone: official Tab5 Keyboard validation. The shared input contract
and mapper are implemented, and the official I2C keyboard driver is now present
in the formal firmware. USB keyboard work is explicitly paused to conserve the
current development budget.

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
  resizing. The Linux TTY currently needs `stty rows 32 cols 64`.
- Adding the xterm `CSI 18 t` query and `CSI 8;32;64t` response is small
  terminal-core work, but ordinary serial shells do not issue that query or
  apply its result automatically.
- Automatic dynamic synchronization therefore also needs a Linux-side helper
  or login hook. Do not inject a visible `stty` command into the user's shell
  as a terminal protocol.

Hardware paths:

- USB keyboard: use the Tab5 USB-A host port. A low-level HID boot-keyboard
  probe already builds, but runtime validation with a physical keyboard remains
  pending and is not part of the current work.
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

Status: paused by user request. Do not modify or validate the USB keyboard path
until the official I2C keyboard support is complete.

### Milestone I3: Official Companion Keyboard

- Verify the exact official hardware and protocol from primary documentation.
- Implement a separately gated driver that emits the same normalized events.
- Verify printable keys, modifiers, navigation/function keys available on the
  device, disconnect/fault behavior, and coexistence with status polling.

Exit condition: the official keyboard can operate the login shell through the
shared mapper. The paused USB path can adopt that mapper later without changing
official-keyboard behavior.

Status: driver implemented using official `M5Unit-KEYBOARD` 0.1.0 in HID mode.
The formal firmware detected address `0x6D`, firmware `0x01`, and configured the
G50 falling-edge interrupt. Physical key-to-shell validation passed for
printable text and Enter: typing `printf 'tab5-keyboard-ok\n'` on the A164
produced `tab5-keyboard-ok` on its own output line. Modifier, navigation,
function-key, and terminal application-mode checks remain pending.

### Milestone I4: Integration

- Define behavior when both keyboards are present.
- Confirm neither input path blocks UART receive, display refresh, or battery
  status updates.
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
This work does not complete I4 because the remaining keyboard checks are still
pending.

Stage 5 completion condition: shared mapping is stable, both requested keyboard
paths have reached their device-specific exit conditions, and integration
validation has been recorded. The keyboard-mounted display direction must also
be visually confirmed with the physical installation, including status-bar
position, battery-area position, terminal geometry, and absence of mirroring.
If unavailable hardware blocks physical validation, keep the affected
milestone explicitly pending rather than silently declaring Stage 5 complete.

## Next Mainline Stage

After Stage 5 reaches its completion condition:

1. Confirm the formal UART build still passes the concise shell probe.
2. Start Stage 6 regression-harness work.
3. Audit remaining Stage 4/xterm compatibility gaps and prioritize only those
   exposed by real applications.

## Progress Log

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
