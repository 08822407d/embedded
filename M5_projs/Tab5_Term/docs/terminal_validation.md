# Terminal Validation Checklist

Use this file after each implementation stage. The tests are intentionally
manual-first because the Tab5 display and external Linux board are part of the
system being validated.

Tests can be run through either the default USB monitor bridge or the dedicated
CDC injection debug firmware. The bridge path executes shell commands on the
external Linux board and then renders the board's output. The CDC injection path
renders host-sent bytes directly and is better for exact parser tests.

## Final-Pixel Validation

When a test requires the final rendered image rather than only terminal cell
state, use a capture-enabled debug environment:

```powershell
.\tools\tab5.ps1 build tab5_terminal_regression
.\tools\tab5.ps1 flash tab5_terminal_regression -Port COM3
.\tools\tab5.ps1 screenshot -Port COM3 `
  -OutputPath .logs\screenshots\current.png
```

Use `tab5_terminal_regression` for deterministic CDC-injected screens. Use
`tab5_screen_capture` when the displayed content must come from the real Linux
login UART. The host writes a PNG, exact RGB565 bytes, and JSON metadata. It
can also compare against a prior raw capture:

```powershell
.\tools\tab5.ps1 screenshot -Port COM3 `
  -OutputPath .logs\screenshots\current.png `
  -Baseline .logs\screenshots\accepted.rgb565
```

This replaces a camera photograph for glyph pixels, colors, clipping, overlap,
layout, cursor, and stale-pixel checks. It does not replace physical inspection
of brightness, panel color, viewing angle, orientation, mounting, ghosting, or
bad pixels. See `screen_capture.md` for the full protocol and restore steps.

## Validation Input Paths

Preferred low-friction path:

- Use the existing USB CDC/JTAG serial port to inject validation byte streams
  into the firmware. Build and flash `tab5_terminal_cdc_inject`; in that mode
  USB CDC bytes route directly to `terminal::writeByte()` instead of being
  forwarded to the external login UART. This requires no keyboard and no extra
  hardware, and lets Codex or a host script replay exact escape-sequence tests.
- In CDC injection mode, text such as `printf ...` is displayed literally
  unless the host sends the actual bytes produced by that command. Use the
  default bridge build when a real shell command must run on the external Linux
  board.

Example host-side byte injection:

```sh
python -c "import serial,time; s=serial.Serial('COM3',115200); time.sleep(.2); s.write(b'\x1b[2J\x1b[Htop-left\r\n'); s.close()"
```

Project helper:

```sh
python tools/send_terminal_test.py --port COM3 --test stage1-smoke
```

The helper waits after opening the port because opening the ESP32-P4 USB CDC
port can reset the board. If only the cursor block is visible after a test, the
bytes were probably sent before firmware startup completed; rerun with a longer
`--open-delay`.

The helper sends in small chunks by default. If the first half of the test is
visible but later lines are missing or truncated, keep the current firmware and
rerun with slower transport settings, for example:

```sh
python tools/send_terminal_test.py --port COM3 --test stage1-smoke --chunk-size 16 --chunk-delay 0.08
```

Expected `stage1-smoke` screen highlights:

- `hello` and `world` render on separate rows.
- `CR+EL: XYZ` replaces the earlier text on that row and clears the tail.
- `Backspace: 13`, `Tabs: A B`, and `OSC skip: beforeafter` are visible.
- The color sample shows red, green, and 256-color yellow foreground text.
- The DEC graphics block shows a small box/line sample instead of raw `lqkx...`
  characters.
- `Row 16 via CSI H`, `cleared tail`, and `End of CDC injection test` appear
  below the graphics block. `cleared tail` appears at the right side of its row
  because the test uses `CSI 2K` to erase the line without moving the cursor
  back to column 1 before printing those bytes.

VT102 screen-model helper:

```sh
python tools/send_terminal_test.py --port COM3 --test stage2-screen --chunk-size 16 --chunk-delay 0.08
```

Expected `stage2-screen` screen highlights:

- `ICH: abXYcd`, `DCH: abef`, and `ECH: ab   f` appear near the top.
- `IL inserted` appears between `IL base 1` and `IL base 2`.
- The delete-line block keeps `DL keep 1` and shifts `DL keep 3` upward.
- `Origin mode row 14` appears at the top of the configured scroll region,
  proving `DECSTBM` plus `DECOM` row addressing.
- `Tab stop:` shows `A` followed by a tab jump to `B`.
- `End of VT102 screen test` appears below the query row.

Expected host-side response from that helper includes:

```text
\x1b[0n\x1b[20;10R\x1b[?6c
```

This means DSR reports OK, CPR reports row 20 column 10 at the query point, and
primary DA reports a VT102-style terminal.

External UART path:

- If a separate USB-to-UART adapter is desired, use an auxiliary UART on exposed
  Tab5 GPIO rather than the USB-A port. The USB-A port is USB Host and would
  require a USB serial host class/driver stack; it is not a simple TTL UART.
- The current login UART uses `G7` as Tab5 RX and `G6` as Tab5 TX for the
  Module LLM path. Do not reuse that pair for validation unless the login UART
  is intentionally disconnected.
- Candidate auxiliary pairs from the Tab5 PinMap are `G17/G52` on M-Bus
  PB_IN/PB_OUT or `G53/G54` on the HY2.0-4P Port.A connector. Wire USB-UART TX
  to the configured Tab5 RX, USB-UART RX to the configured Tab5 TX only if
  firmware will transmit responses, and always connect GND. Use 3.3 V TTL
  levels; do not feed RS-232 or 5 V TTL into Tab5 GPIO.

Real login-shell visual demo:

```sh
python tools/send_login_shell_demo.py --port COM3 --demo vt102
```

This uses the formal UART terminal build, not the CDC injection debug build.
The host sends a temporary here-document to the external LLM module login shell;
the shell runs `printf` commands and emits terminal control sequences back to
Tab5. It does not write a file on the LLM module.

Expected screen highlights:

- The first row says `LLM shell -> Tab5 VT102 demo`.
- Host and kernel lines come from the real LLM module shell.
- Color, inverse, DEC graphics, insert/delete/erase character, and scroll
  region effects appear below.
- A final `End of real shell VT102 demo` line appears before the shell prompt.

Avoid terminal query sequences such as `CSI 6 n` in this real-shell demo unless
the shell input side is prepared for the response, because terminal replies go
back into the LLM module's shell input stream.

Real login-shell SGR demo:

```sh
python tools/send_login_shell_demo.py --port COM3 --demo sgr
```

Expected screen highlights:

- `bold` is visibly heavier than surrounding text.
- `dim` is darker than normal text.
- `underline` has a line under the word.
- `inverse` swaps foreground and background.
- `SECRET` in `hidden:[SECRET]` is not visible, but the brackets are visible.
- `red`, `bright-green`, `yellow256`, and `true-orange` differ visibly.
- The `BCE:` row keeps a blue background from `blue-to-eol` through the erased
  tail of the row.

## Stage 1: Core Screen And Parser

Basic shell output:

```sh
printf 'hello\r\nworld\r\n'
```

Expected:

- `hello` and `world` appear on separate terminal rows below the status bar.
- The status bar is not overwritten.
- Cursor advances after the last line.

Carriage return:

```sh
printf 'abc\rXYZ\r\n'
```

Expected:

- The visible line starts with `XYZ`.
- There is no stray `[` or escape-sequence text.

Tabs and backspace:

```sh
printf 'A\tB\r\n12\b3\r\n'
```

Expected:

- `B` lands at the next 8-column tab stop or the last column if no tab stop
  remains.
- Backspace moves the cursor left without erasing by itself, so `13` is visible
  for the second line.

Clear screen and cursor position:

```sh
printf '\033[2J\033[Htop-left\r\n'
```

Expected:

- Terminal content area clears.
- `top-left` appears at the first cell below the status bar.
- Status bar remains visible.

Cursor movement and erase line:

```sh
printf '\033[2J\033[Habcde\033[1D!\033[K\r\n'
```

Expected:

- The first visible line is `abcd!`.
- Characters to the right of the cursor were erased.

Unsupported sequence recovery:

```sh
printf 'before\033]0;ignored title\007after\r\n'
```

Expected:

- The line shows `beforeafter`.
- The OSC body text does not appear on screen.

Scroll:

```sh
for i in $(seq 1 120); do printf 'line %03d\r\n' "$i"; done
```

From the development PC, the repeatable bridge test is:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" `
  tools\send_login_shell_demo.py `
  --port COM3 --baud 921600 --demo scroll --open-delay 3
```

The script sends 50 paced lines through the real Linux login shell and fails
unless `SCROLL-01` through `SCROLL-50` return in order.

Expected:

- Content scrolls within the terminal area only.
- The green status bar and black battery area remain intact.
- The latest lines are visible at the bottom.

Smoke commands:

```sh
clear
printf 'TERM=%s\r\n' "$TERM"
tput clear
```

Expected:

- `clear` and `tput clear` do not leave raw escape text on screen.
- If `$TERM` is broader than current firmware support, note it before testing
  richer apps.

## Stage 2: Screen Model Completeness

Run after insert/delete operations and scroll region support are implemented:

```sh
printf '\033[2J\033[H1\r\n2\r\n3\033[2;1H\033[LX\r\n'
printf '\033[2J\033[Habcdef\033[3D\033[P\r\n'
printf '\033[2J\033[3;10r\033[10;1Hbottom\033Dtop?\r\n\033[r'
```

Expected results must be documented with the implementation because these
features have edge cases around cursor and scroll-region state.

## Stage 3: Attributes And Color

CDC injection helper:

```sh
python tools/send_terminal_test.py --port COM3 --test stage3-color --chunk-size 16 --chunk-delay 0.08
```

Expected:

- Attribute row shows heavier `bold`, darker `dim`, underlined `underline`,
  reversed `inverse`, and invisible `SECRET` in the hidden sample.
- 16-color, bright-color, 256-color, and truecolor rows show visibly distinct
  foreground colors.
- `BCE EL` keeps a blue background across the erased tail of the row.
- `BCE ECH` shows erased character cells using the active magenta background.
- `Reset` returns to the terminal theme colors after `0m`.

Color table:

```sh
for n in 30 31 32 33 34 35 36 37 90 91 92 93 94 95 96 97; do
  printf '\033[%sm%3s\033[0m ' "$n" "$n"
done
printf '\r\n'
```

Expected:

- Foreground colors visibly differ.
- `0m` returns to the theme default.

256-color sample:

```sh
for n in 16 21 46 82 196 202 226 231; do
  printf '\033[38;5;%sm%3s\033[0m ' "$n" "$n"
done
printf '\r\n'
```

Expected:

- Values map to distinct RGB565 colors.

## Stage 4: xterm/DEC Essentials

CDC injection helper:

```sh
python tools/send_terminal_test.py --port COM3 --test stage4-xterm --chunk-size 16 --chunk-delay 0.08 --read-response-window 0.8
```

Expected screen highlights:

- The title line says `Stage4 xterm/DEC essentials test`.
- `Primary buffer survives alternate screen.` is still visible after the
  alternate-screen section exits.
- No `ALTERNATE SCREEN` or `Temporary alternate red text` lines remain visible
  on the final screen; those should disappear when `?1049l` restores primary
  screen content.
- `Alt screen restored: primary line above should remain` is visible.
- Private mode toggles do not leak raw escape text.
- Bracketed paste markers do not leak; the visible text is `before PASTE after`.
- UTF-8 sample line shows a coherent single-cell path for `é`, `Ω`, and `中`.
  ASCII cells use the fixed terminal font grid. These validation glyphs use
  deterministic built-in single-cell bitmap fallbacks because the available
  built-in M5GFX fonts did not cover the mixed Latin-1/Greek/CJK sample
  reliably.
- The final line says `End of Stage4 xterm/DEC essentials test`.

Expected host-side response from that helper includes:

```text
\x1b[?6c\x1b[>0;0;0c\x1b[7;31R\x1b[?7;39R\x1b[?6c
```

This means primary DA, secondary DA, CPR, DEC private CPR, and ESC `Z` DECID
all replied through the terminal response callback.

Stage 8 T1 adds the xterm text-area size query. The dedicated
`stage8-protocol` regression case expects:

```text
\x1b[8;32;64t
```

This response prepares future SSH/Telnet/PTY-backed integrations. It is not
used to auto-resize the current raw UART login shell; that path still uses the
Module LLM profile's persistent `stty rows 32 cols 64`.

Stage 8 T2 adds the shared terminal geometry snapshot API used by the T1
response path and by future transport layers. It has no separate hardware
validation requirement yet because it is preparatory and not user-visible. On
2026-06-15 both formal and regression environments built successfully; hardware
validation is deferred until an advanced login transport or PTY-backed
integration consumes it.

Validation record:

- 2026-06-15: `stage8-protocol` passed as part of the full seven-case hardware
  regression run. The formal firmware was restored afterward, and the
  login-shell probe returned `shell-path-ok: m5stack-LLM`.

## Font Preview

CDC injection helper:

```sh
python tools/send_terminal_test.py --port COM3 --test font-preview --chunk-size 16 --chunk-delay 0.08 --read-response-window 0.2
```

Expected:

- The first line says `Font debug: DejaVu18 glyphs in fixed 18x20 cells`.
- Cell geometry is `64x32`; narrow glyphs retain equal-width cells in this
  debug mode.
- DEC graphics, SGR attributes/colors, cursor, and UTF fallback samples remain
  coherent.

Proportional 20px-line-height preview:

```sh
python tools/send_terminal_test.py --port COM3 --test font-prop-preview --chunk-size 8 --chunk-delay 0.12 --read-response-window 0.2
```

Expected:

- The firmware must be `tab5_terminal_font_prop_preview`.
- The first line says
  `Font preview: DejaVu18 true proportional in 18x20 rows`.
- Debug fixed-cell mode is off. The `Narrow:` sample should show `i`, `l`, `!`,
  and `|` taking visibly less horizontal space than `M` and `W` in the `Wide:`
  sample.
- This is the visual/release-style preview path. For terminal parser and TUI
  correctness work, use fixed-cell rendering.

Fixed-cell debug comparison with the same proportional font:

```sh
pio run -e tab5_terminal_font_prop_fixed_debug
python tools/send_terminal_test.py --port COM3 --test font-prop-preview --chunk-size 16 --chunk-delay 0.08 --read-response-window 0.2
```

Expected:

- The firmware must be `tab5_terminal_font_prop_fixed_debug`.
- The same DejaVu18 glyphs are drawn, but every logical terminal cell has the
  same pixel width. Narrow characters will again show more surrounding blank
  space. Use this mode for feature development and compatibility tests.

Run common applications:

```sh
less /etc/os-release
nano /tmp/tab5-terminal-test.txt
vim /tmp/tab5-terminal-test.txt
htop
```

Expected:

- Alternate-screen apps restore the shell screen on exit.
- Cursor movement, colors, and status lines remain coherent.
- Exiting the app does not leave raw control sequences visible.

Automated real-shell smoke helper:

```sh
python tools/send_login_shell_app_smoke.py --port COM3 --apps clear,reset,tput,less,nano,vim,htop --rows 32 --cols 64 --chunk-size 32 --chunk-delay 0.08
```

This uses the formal UART build and launches the apps on the external LLM
module. It sets `TERM=xterm-256color` and `stty clocal`; the `clocal` flag is
needed because GNU less otherwise blocks while opening `/dev/ttyS1` on this
serial login environment. On the current LLM image, `nano` is not installed and
is expected to be skipped.

For correctness testing, flash `tab5_min_uart_terminal_fixed_debug`. Use
`tab5_min_uart_terminal` only when checking the approved proportional
release-style appearance.

## Stage 5: Input

Use `current_work.md` to identify the currently active input milestone. Run the
same semantic checks for every supported keyboard transport so transport
differences do not hide mapper defects.

Before keyboard testing, mount the Tab5 in the official companion keyboard
orientation and reboot the formal firmware.

Expected:

- The status bar is at the physical top edge when viewed from the keyboard.
- Status text is upright, and the battery area remains at the upper-right.
- Terminal content starts below the status bar and retains the expected
  landscape geometry.
- The image is rotated, not mirrored.

With the verified official companion keyboard attached through its documented
interface:

```sh
cat -v
```

Expected:

- Printable keys appear.
- Enter, Backspace, Tab, Escape, arrows, Home/End, Page Up/Down, Delete, and
  function keys emit the expected terminal input sequences for the current
  mode.
- Shift, Ctrl, and Alt combinations are consistent between transports.
- Application cursor mode changes arrow-key output as expected.
- Disconnecting and reconnecting one device does not stop UART receive or the
  other input path.
- Holding and releasing a key does not leave a stuck modifier or an unbounded
  repeat stream.
- Multiple simultaneous non-modifier keys are either represented correctly or
  recorded as a documented hardware/firmware limitation. The current A164 HID
  driver stores only one `last_hid_keycode`, so this check is mandatory before
  declaring official-keyboard support complete.

Record device model, firmware environment, connection method, and unavailable
keys for every physical validation pass.

USB keyboard validation is deferred and is not part of the current Stage 5
exit criteria.

Official Tab5 Keyboard first-pass check:

1. Boot the formal firmware and run `.\tools\tab5.ps1 boot-log -Port COM3`.
   Confirm the summary contains `addr=0x6D`, `mode=HID`, and `irq=G50`.
2. At the Module LLM shell prompt, type
   `printf 'tab5-keyboard-ok\n'` using only the physical keyboard and press
   Enter. The command and `tab5-keyboard-ok` must appear on screen.
3. Run `cat -v`, then test Tab, Escape, Backspace, arrows, Home/End,
   Page Up/Down, Delete, Ctrl combinations, Alt combinations, and available
   function keys. Exit with Ctrl+C.
4. Emit `printf '\033[?1h'`, run `cat -v`, and confirm arrow keys change from
   `^[[A/B/C/D` to `^[OA/B/C/D`. Exit and restore normal cursor mode with
   `printf '\033[?1l'`.

Validation record:

- 2026-06-08: step 2 passed on the installed A164 keyboard. The expected
  `tab5-keyboard-ok` output appeared on a separate line.
- 2026-06-12: the user completed the remaining practical keyboard and
  integration tests and reported no observed problem. Stage 5 was accepted as
  complete.
- Full simultaneous non-modifier rollover is not guaranteed by the current
  single-key HID state tracker and remains a documented limitation.
- 2026-06-12: the Module LLM ttyS1 login environment was persistently set to
  `TERM=xterm-256color`, `COLORTERM=truecolor`, and `32x64`; reboot verification
  passed and colored `htop` output was visually confirmed on Tab5.

## Stage 6: Regression Harness

Stage 6 converts the existing visual and marker-based checks into a repeatable
regression workflow. Its primary new requirement is diagnostic-only,
machine-readable terminal state; it must not alter the production input path.

The regression manifest must include:

- deterministic Stage 1-4 corpus replay;
- terminal query/reply checks;
- screen dimensions, cursor, margins, mode, and buffer-summary assertions;
- real login-shell application smoke for every installed test application;
- explicit visual-only checks and known unsupported behavior.

Keep detailed output under the repository log directories and print only a
concise pass/fail summary in the normal command path.

Validation record:

- 2026-06-12: `tab5_terminal_regression` built and flashed successfully.
- The first deterministic run passed Stage 1, 3, and 4. Stage 2 exposed an
  incorrect manifest expectation for a default tab stop; review confirmed the
  implementation result and the expectation was corrected.
- The second deterministic run passed all four cases.
- Formal `tab5_min_uart_terminal` was rebuilt and restored with verified flash
  hash.
- The restored firmware passed `shell-path-ok: m5stack-LLM`.
- Real-app smoke passed `clear`, `reset`, `tput`, `less`, `vim`, and `htop`;
  `nano` was skipped because it is not installed.
- `tools/run_stage6_regression.ps1 -Port COM3 -SkipBuild` then passed the
  complete workflow and left `tab5_min_uart_terminal` installed.
- 2026-06-12: the user reported no problem in the final manual check of
  status-bar placement, A164 input, battery-level refresh, and display
  responsiveness. Stage 6 was accepted as complete.
- Charging-state detection was not accepted as working. It remains explicitly
  deferred because the observed charger-status source did not reliably change
  with cable insertion and removal.

Runtime login-UART baud check:

```powershell
.\tools\tab5.ps1 baud -Port COM3
.\tools\tab5.ps1 baud -Port COM3 -Baud 460800
.\tools\tab5.ps1 probe -Port COM3
.\tools\tab5.ps1 baud -Port COM3 -Baud 921600
.\tools\tab5.ps1 probe -Port COM3
.\tools\tab5.ps1 baud -Port COM3 -DefaultBaud
.\tools\tab5.ps1 probe -Port COM3
```

Expected:

- Each coordinated change reports the requested active rate.
- Every probe captures an executed `shell-path-ok: <hostname>` marker.
- The status title changes to the active rate.
- `-DefaultBaud` leaves `persisted=default`.

Validation record:

- 2026-06-08: 460800 and 921600 coordinated shell probes passed against
  Module LLM `/dev/ttyS1`.
- NVS was saved at 460800, Tab5 alone was hard-reset, boot reported
  `baud=460800 persisted=saved`, and the shell probe passed.
- Final state was restored to
  `active=115200 persisted=default pending=none`.
- Later on 2026-06-08, the Module LLM boot-time M5Bus login UART was
  permanently changed to 921600 8N1 and retained the rate after a Linux reboot.
  Tab5 was then saved at 921600, hard-reset, and reported
  `baud=921600 persisted=saved`; the strict shell probe passed. The current
  installed baseline is 921600 on both endpoints.
- The formal renderer was then returned to fixed 18x20 cells while keeping the
  DejaVu18 font. A single Linux `printf` of 64 printable characters took about
  0.016 second at the post-render USB observation point, compared with
  1.000 second in proportional mode. The 921600 state and strict shell probe
  remained valid.

## Regression Notes

Whenever a bug is found, add:

- Firmware environment and commit.
- Exact byte stream or command.
- Expected screen behavior.
- Observed screen behavior.
- Whether the problem is parser, screen model, rendering, input, or transport.

## Stage 7: Unicode Width And Cell Integrity

Automated U1/U2 check:

```powershell
.\tools\tab5.ps1 build tab5_terminal_regression
.\tools\tab5.ps1 flash tab5_terminal_regression -Port COM3
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" `
  tools\run_terminal_regression.py --port COM3 --case stage7-unicode
```

Expected: `PASS stage7-unicode` and `1 passed, 0 failed`. The assertions cover
wide lead/continuation state, right-margin wrapping, combining attachment,
erase/insert/delete repair, scrolling, and malformed UTF-8.

Visual check while the regression firmware is installed:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" `
  tools\send_terminal_test.py --port COM3 --test stage7-unicode
```

Expected:

- `ASCII: A中B` has `中` occupying two logical columns with `B` aligned after
  it.
- `Combining: éX` keeps the acute mark on `e`; `X` immediately follows in the
  next cell.
- The boundary sample places `中Z` on the next row as a unit.
- Erase, insert, delete, and scroll samples contain no half glyph or displaced
  continuation cell.
- `Invalid: ?OK` shows one deterministic replacement and parser recovery.

Stage 7 U3 Unicode graphics fallback check:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" `
  tools\run_terminal_regression.py --port COM3 --case stage7-unicode-graphics
```

Expected: `PASS stage7-unicode-graphics` and `1 passed, 0 failed`. The
assertions confirm that common box-drawing and block-element codepoints are
stored as single-width UTF-8 cells rather than falling back to ASCII `?`.

Visual/final-pixel check:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" `
  tools\send_terminal_test.py --port COM3 --test stage7-unicode-graphics
.\tools\tab5.ps1 screenshot -Port COM3 `
  -OutputPath .logs\screenshots\stage7-unicode-graphics.png
```

Expected: the light, heavy, and rounded box samples render as connected line
graphics, and the block row shows fractional blocks, full block, half blocks,
right half block, and light/medium/dark shade. This remains a deterministic
fallback set, not a complete Unicode font.

Restore and verify the formal firmware after diagnostic testing:

```powershell
.\tools\tab5.ps1 build tab5_min_uart_terminal
.\tools\tab5.ps1 flash tab5_min_uart_terminal -Port COM3
.\tools\tab5.ps1 probe -Port COM3
.\tools\tab5.ps1 app-smoke -Port COM3
```

Validation record:

- 2026-06-15: Stage 1-4 plus `stage7-unicode` and
  `stage7-unicode-graphics` passed 6/6 on physical Tab5. The framebuffer
  screenshot for `stage7-unicode-graphics` captured 1280x720 RGB565LE with
  CRC32 `6D8657DB`. Formal firmware was restored afterward, and the login-shell
  probe returned `shell-path-ok: m5stack-LLM`.
- 2026-06-12: Stage 1-4 plus `stage7-unicode` passed 5/5 on physical Tab5.
- Formal firmware was restored with verified flash hashes.
- Login-shell probe returned `shell-path-ok: m5stack-LLM`.
- `clear`, `reset`, `tput`, `less`, `vim`, and `htop` returned `rc=0`;
  `nano` was skipped because it is not installed.
