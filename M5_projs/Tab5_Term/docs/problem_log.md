# Problem Log

## 2026-07-18: Title-Bar Dots Appeared Uneven Against Rounded Edge

Symptom: although the three colored title-bar dots used the same diameter and
vertical coordinate, the red dot appeared to have a different gap from the
window's top edge.

Root cause: the first dot crossed the 16 px rounded-corner transition while the
other two were beneath the horizontal top segment. Equal circle coordinates
did not imply equal local boundary geometry.

Action taken: shifted the group 5 px right and derived the first visible dot
pixel from the corner's horizontal tangent (`x=1146`). All three dots are 9x9
with top coordinate `y=172` and now lie completely under the same straight
edge.

Status: resolved and hardware-verified in
`.logs/screenshots/tab5-launcher-terminal-window-v12.png` (CRC32 `D19CF65E`).
The complete 51,642-byte terminal resource rectangle matches the baked asset
with zero differing bytes.

## 2026-07-18: Terminal Tile Looked Too Square

Symptom: after the obsolete sleep labels were removed, the baked terminal tile
still occupied the full 146x144 touch region and looked too square.

Action taken: cleared the old button art to the launcher background color and
redrew only the visible tile at 144x108, an exact 4:3 ratio. The original `>_`
glyph pixels were repainted unchanged. The transparent touch target remains
146x144.

Status: resolved and hardware-verified. Screenshot
`.logs/screenshots/tab5-launcher-terminal-4x3.png` has CRC32 `775442C5` and
shows the 4:3 tile with clean margins. The edited screenshot region exactly
matches all 44,998 bytes of the asset, and the new glyph crop exactly matches
the prior glyph crop.

## 2026-07-18: Old Sleep Labels Visible Through Terminal Overlay

Symptom: the terminal tile still showed faint traces of `SLEEP & SHAKE TO
WAKEUP` and `SLEEP 10 SECONDS` on the physical display.

Root cause: those labels are pixels in `app/assets/images/launcher_bg.c`. The
replacement LVGL container used background opacity `245/255`, so it darkened
but did not fully replace the underlying pixels.

Action taken: preserved the official background and generated the independent
`launcher_bg_terminal.c` copy with the terminal tile written directly into its
RGB565 data. `LauncherView` now uses that copy, while `PanelPower` exposes a
fully transparent 146x144 hit region with the existing callback.

Status: resolved and hardware-verified. The final 4:3 revision is captured in
`.logs/screenshots/tab5-launcher-terminal-4x3.png` with CRC32 `775442C5`.
Visual inspection shows no obsolete text. Direct touch input was not repeated
in this checkpoint.

## 2026-07-11: Runtime Fan Policy And NVS Validation

The runtime policy path was built, flashed, and exercised on the connected Tab5
with a real Module Fan v1.1. Verified behavior includes immediate RAM-only
application, dirty/source reporting, explicit NVS save, recovery after the
USB-open reset, restoration of compiled defaults, dynamic 5000/6000 ms control
intervals, and whole-curve rejection on invalid ordering.

One sample immediately after applying 0% still reported `4830 RPM` while the
target and applied duty were already 0. The next sample reported 0 RPM. This is
expected fan inertia plus the timing of the RPM register, not a failed duty
write.

The first build command in this session sourced ESP-IDF without setting the
project-local `IDF_TOOLS_PATH`, so it looked for a nonexistent Python virtual
environment under `~/.espressif` and never entered compilation. Re-running from
`platforms/tab5` with the documented project-local `IDF_TOOLS_PATH` completed
normally. No source change was required for that host-environment error.

NVS fallback was verified for the missing-record case. Deliberate blob
corruption, version mismatch, NVS-full recovery, and power loss during
`nvs_commit` were not destructively tested. The centralized initializer retains
the official firmware's existing behavior of erasing NVS when initialization
reports no free pages or a newer incompatible NVS version.

Status: runtime update and normal persistence are hardware-verified. Corrupt
storage recovery remains source- and build-verified only.

## 2026-07-11: Module Fan Runtime Validation Coverage

The native driver, automatic service, and controlled telemetry were flashed to
the connected Tab5 and exercised with a real Module Fan v1.1. Verified behavior
includes identity/firmware reads, 5-second temperature snapshots, opt-in
start/stop telemetry, 0% stop control, 29% automatic control, RPM reads around
`5040-5190`, and CRC-clean screenshot capture after telemetry.

The following behavior remains unverified on hardware:

- Detach/re-attach behavior when the module is hot-plugged or the official GUI
  toggles EXT 5V.
- The 49%, 69%, and 98% levels under real thermal load.
- Temperature-read failure and three-consecutive-error recovery paths.

Known access boundary: screenshot commands and fan telemetry are serialized by
one firmware task, but two host processes opening the same serial device can
still divide input bytes or fail on exclusive access. Use one of
`tools/module_fan_telemetry.py` or `tools/capture_screen.py` at a time. A stop
command sent during a screenshot is handled after the synchronous image stream
finishes.

Status: core source/build/flash/runtime path verified; destructive or
high-temperature edge cases remain pending.

## 2026-07-11: USB Serial Open Resets The Board Before Debug Commands

Symptom: a status command sent after only a 1-second serial-open delay timed
out. A direct boot capture showed `rst:0x17 (CHIP_USB_UART_RESET)` whenever this
Ubuntu host opened the USB Serial/JTAG TTY. The debug command task starts about
7 seconds later.

Related screenshot symptom: a CRC-valid capture taken after 8 seconds contained
large black areas because the launcher had not completed asynchronous drawing.
A 15-second retry produced the complete launcher with CRC32 `90CEBF21`.

Action taken: `tools/module_fan_telemetry.py` now defaults to an 8-second open
delay, and `tools/capture_screen.py` defaults to 15 seconds. Both remain
overridable for hosts whose serial-open behavior differs.

Status: mitigated in the host tools. Opening the TTY still resets this board, so
stream mode is preferred for telemetry start/read/stop in one connection.

## 2026-07-11: Existing CMake Glob Needed Reconfiguration For New Sources

Symptom: the first incremental build compiled the modified callers but failed
at link time with undefined references to `cpu_temperature::read`,
`module_fan::start`, and `module_fan::getSnapshot`.

Cause: `platforms/tab5/main/CMakeLists.txt` uses `GLOB_RECURSE` without
`CONFIGURE_DEPENDS`. Its cached Ninja source list was created before the new
component files existed.

Action taken: ran `idf.py reconfigure build`. The new sources were then compiled
and the complete firmware linked successfully. Fresh builds that configure
after applying patch `0003` naturally include them.

Status: resolved; no CMake source change was needed.

## 2026-07-09: Parent Git Root Can Make Commits Too Broad

Symptom: running normal git commands from `M5_projs/Tab5_Term` can be
misleading because the actual git root is `/home/cheyh/projs/embedded`.
Unscoped `git add --all` stages changes relative to that larger repository.

Evidence:

- `git rev-parse --show-toplevel` returns `/home/cheyh/projs/embedded`.
- The parent repo remote is `git@github.com:08822407d/embedded.git`.
- There is no remote named `master`; a literal `git push -u master` fails with
  `No such remote 'master'`.
- The official firmware clone has its own M5Stack remote, but it is under the
  ignored `worktrees/` directory and is not the parent repo's push target.

Risk: a broad parent-repo commit could accidentally include unrelated sibling
project changes. It should not, however, become a push or PR to the M5Stack
official firmware repo or the libvterm upstream unless commands are run inside
those separate repositories and a PR is created explicitly.

Action taken: recorded the safe path-scoped workflow in
`docs/official_firmware_build.md` and the current handoff in
`docs/current_work.md`.

Status: mitigated for the current checkpoint. Before committing, run
`git status --short --branch` from the parent repo and stage with
`git add --all M5_projs/Tab5_Term`.

## 2026-07-09: Startup White Screen After Screenshot Debug Integration

Symptom: after the first screenshot-debug integration build was flashed, the
physical Tab5 stayed on a white screen from startup. A first screenshot capture
also returned an all-white `1280x720` image.

Evidence:

- Raw serial boot logs after USB Serial/JTAG reset reached
  `AppStartupAnim on open` but did not reach `AppStartupAnim on close` or
  `AppLauncher on open` within a longer capture window.
- The white screen matched the startup animation's initial white background.
- Moving the screenshot task later showed that the screenshot task was not the
  direct blocker; when delayed, it never started because startup did not reach
  the launcher loop.

Likely cause: app-level UI startup was racing the freshly initialized
display/LVGL port. The first startup animation LVGL lock could block before the
animation created visible objects or closed.

Action taken:

- Added a 250 ms post-HAL settle delay before opening `AppStartupAnim`.
- Kept screenshot debug lazy and non-invasive: the task starts after the main
  app loop has run briefly, and the USB Serial/JTAG driver is installed only
  when serving a screenshot command.

Status: resolved in the current Ubuntu worktree. A post-fix boot log reached
`AppStartupAnim on close`, `AppLauncher on create`, `AppLauncher on open`,
`launcher-view init`, and `screen-capture debug command task started`. A
post-fix screenshot captured the launcher with CRC32 `2B8CB402`.

## 2026-07-09: Initial Screenshot Stream CRC Mismatch

Symptom: the first official-firmware screenshot capture emitted
`TAB5SHOT BEGIN` and `TAB5SHOT END`, but the host calculated a different CRC
than the device.

Evidence:

- Device CRC: `9C0A682E`.
- Host CRC: `5A6E0432`.
- The transfer used plain POSIX `write()` to the console stream.

Likely cause: the USB Serial/JTAG console VFS writes bytes through a small
buffer path with short flush behavior intended for logs, not large raw binary
frames. Logs or dropped bytes can corrupt a full-screen RGB565 stream.

Action taken: switched screenshot transfer to the ESP-IDF `usb_serial_jtag`
driver, used 128-byte chunks, periodic `usb_serial_jtag_wait_tx_done()` calls,
and temporary log-level reduction during the binary frame.

Status: resolved. The verified capture
`.logs/screenshots/tab5-launcher-terminal-entry-fixed.png` was received as
`1280x720 rgb565le` with CRC32 `2B8CB402`.

## 2026-07-09: Launcher Sleep Labels Are Baked Into Background Art

Symptom: searches for `sleep & shake to wakeup`, `sleep 10 seconds`, and related
strings did not find launcher button label text in C++ source. The power panel
uses transparent `Container` hit regions over the launcher background, so the
visible labels are most likely baked into `app/assets/images/launcher_bg.c`.

Action taken: removed the two old transparent click regions from `PanelPower`
and drew a visible terminal button overlay over their combined region. The patch
is recorded at
`port/official_firmware_patches/0001-launcher-terminal-button.patch` for replay
against fresh official-firmware worktrees.

Status: the original diagnosis remains correct. The temporary overlay was
replaced on 2026-07-18 by a dedicated background asset, and the visual result is
now hardware-verified without label bleed. Direct touch-hit validation is still
pending.

## 2026-07-07: Official Firmware Dependency Fetch Can Look Stalled

Symptom: running the official `fetch_repos.py` on Windows produced a long
period with no useful progress output while fetching/checking out the `lvgl`
dependency. It was interrupted during investigation.

Action taken: switched local helper scripts to shallow clone each dependency
from `repos.json` only when the dependency directory is missing. The current
Windows worktree has all four dependencies present.

Status: mitigated for local work. Use `tools/official_firmware_windows.ps1` or
`tools/official_firmware_ubuntu.sh` instead of manually reasoning through the
official fetch step during routine setup.

## 2026-07-07: Windows ESP-IDF Build Blocked By Missing idf.py

Symptom: `.\tools\official_firmware_windows.ps1 -Action build-idf` stops before
compilation because `idf.py` is missing and `IDF_PATH` is not set.

Action taken: recorded the expected toolchain version in
`docs/official_firmware_build.md`. The script now gives a direct diagnostic.

Status: unresolved until ESP-IDF `v5.4.2` is installed and exported in the
current PowerShell session.

## 2026-07-07: Windows Desktop Smoke Build Blocked By Missing C/C++ Compiler

Symptom: the official desktop CMake build cannot configure because no C or C++
compiler is visible in PATH. The first CMake attempt reported missing
`CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`.

Action taken: updated the Windows helper script to fail earlier with a clear
message when `cl`, `gcc`, and `clang` are all missing.

Status: unresolved until a Visual Studio Developer PowerShell, MinGW, or LLVM
environment is used.

## 2026-07-07: Ubuntu Script Syntax Check Blocked On This Windows Host

Symptom: invoking `bash` on the current Windows host starts a broken WSL
instance that fails before running the command. Therefore `bash -n
tools/official_firmware_ubuntu.sh` could not be used from this host.

Action taken: kept the Ubuntu script as a standalone file for a real Ubuntu
24.04 environment. Do not treat the Windows WSL failure as evidence about the
script or the official firmware source.

Status: pending validation on Ubuntu 24.04.

## 2026-07-07: Official libvterm Tarball Download Failed In PowerShell

Symptom: downloading
`https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz` with
`Invoke-WebRequest` failed with a TLS protocol alert:

`Authentication failed because the remote party sent a TLS alert: 'ProtocolVersion'.`

Action taken: cloned `https://github.com/neovim/libvterm.git` with depth 1,
removed its nested `.git`, and kept the resulting source snapshot in
`third_party/libvterm-src/`.

Status: workaround accepted for starting the port quickly. Revisit the source
provenance before release-quality integration.

## 2026-07-07: No Hardware Validation In Current Phase

Symptom: the Tab5 board is not currently available to the user.

Action taken: no build, flash, or runtime claims are made. New code is recorded
as source-level scaffold only.

Status: resolved for the official firmware baseline on Ubuntu 24.04 as of
2026-07-08. The connected Tab5 was built, flashed, and boot-log validated before
terminal migration work. This does not validate the future terminal port itself.

## 2026-07-08: ESP-IDF Monitor Needs TTY In Codex

Symptom: `idf.py monitor` failed in plain non-TTY command execution with:

`Monitor requires standard input to be attached to TTY. Try using a different terminal.`

Action taken: reran the monitor command with a PTY. The monitor started but did
not emit boot text in the timed capture window, so runtime validation used a
lower-level reset/read flow: `esptool chip_id` to hard reset, followed by direct
pyserial capture at `115200` baud.

Status: workaround accepted for this session. Use the raw serial capture method
again if `idf.py monitor` is awkward inside Codex.

## 2026-07-08: ESP-IDF Toolchain Kept Project-Local

Symptom: this Ubuntu host had `~/esp/v5.5/esp-idf/export.sh`, but exporting it
failed because its Python environment was missing. The official firmware target
requires ESP-IDF `v5.4.2`, not v5.5.

Action taken: cloned ESP-IDF `v5.4.2` into `toolchains/ubuntu/esp-idf-v5.4.2`
and installed the `esp32p4` tools with
`IDF_TOOLS_PATH=toolchains/ubuntu/espressif`.

Status: resolved. Future Ubuntu builds should source the project-local v5.4.2
environment unless the official firmware requirement changes.

## 2026-07-08: Official Baseline Boot Has Nonfatal Warnings

Symptom: the unmodified official firmware boot log contains several warnings or
errors even though the app continues to launch:

- RTC/system time synced to `2062-03-07`.
- LEDC warned that GPIO 22 was not usable or conflicted.
- The audio path logged `Mode 1 conflict sample_rate 44100 with 48000`.

Action taken: recorded these as official-baseline observations from the
2026-07-08 Ubuntu flash/boot validation, not as terminal-port regressions.

Status: open for later visual/runtime review. Do not attribute these messages to
the terminal migration unless they newly change after port work begins.

## 2026-07-07: Adapter Callback Access Control

Symptom: the first C++ adapter pass placed a `VTermScreenCallbacks` table at
file scope while callback functions were private static methods. That would
violate C++ access rules.

Action taken: moved the callback table into `TerminalSession::begin()`, where
private static callbacks are accessible.

Status: fixed at source level; not yet compiler-validated.
