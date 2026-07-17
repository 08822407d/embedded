# Official Firmware Build Notes

## Source

The active upstream firmware source is:

- Repository: `https://github.com/m5stack/M5Tab5-UserDemo`
- Official Tab5 user demo build note:
  `https://docs.m5stack.com/en/esp_idf/m5tab5/userdemo`

The official ESP-IDF target build uses ESP-IDF `v5.4.2` and the firmware
directory `platforms/tab5`.

## Local Workspace Layout

The official firmware is intentionally kept outside tracked source:

- Windows 10 worktree: `worktrees/windows/M5Tab5-UserDemo/`
- Ubuntu 24.04 worktree: `worktrees/ubuntu/M5Tab5-UserDemo/`
- Optional host-specific toolchains: `toolchains/windows/`,
  `toolchains/ubuntu/`

These paths are ignored by git. This prevents Windows-generated build files,
ESP-IDF virtual environments, and Ubuntu build outputs from sharing the same
directory.

## Git And Patch Boundary

This project lives inside the larger git repository
`/home/cheyh/projs/embedded`. The current parent-repo remote is
`git@github.com:08822407d/embedded.git`.

The official firmware worktree under `worktrees/ubuntu/M5Tab5-UserDemo/` is
itself a clone of `https://github.com/m5stack/M5Tab5-UserDemo`, but the
`worktrees/` directory is ignored by the parent repo. Do not commit or push from
inside the official firmware worktree unless intentionally preparing a separate
fork/PR workflow.

For this port workspace, preserve local official-firmware modifications as
portable patch files under `port/official_firmware_patches/`. Commit those patch
files, repo-local docs, and helper tools from the parent repo with path-scoped
commands such as:

```bash
git add --all M5_projs/Tab5_Term
git commit -m "tab5 official firmware port checkpoint"
git push -u origin master
```

Do not use `git add --all` as a substitute for checking the parent-repo status;
the git root covers more than `M5_projs/Tab5_Term`.

## Windows 10 Commands

Check available tools and the local official repo:

```powershell
.\tools\official_firmware_windows.ps1 -Action check
```

Clone the official repo and dependencies into the Windows worktree:

```powershell
.\tools\official_firmware_windows.ps1 -Action setup
```

Attempt the ESP-IDF Tab5 build after ESP-IDF `v5.4.2` has been installed and
exported in the current PowerShell session:

```powershell
.\tools\official_firmware_windows.ps1 -Action build-idf
```

The desktop build is useful only as a host-side smoke test of the official GUI
code. It is not a Tab5 firmware build:

```powershell
.\tools\official_firmware_windows.ps1 -Action build-desktop
```

## Ubuntu 24.04 Commands

Install common host packages for the official desktop build:

```bash
sudo apt install build-essential cmake ninja-build libsdl2-dev
```

Check tools:

```bash
./tools/official_firmware_ubuntu.sh check
```

Clone the official repo and dependencies into the Ubuntu worktree:

```bash
./tools/official_firmware_ubuntu.sh setup
```

Attempt the ESP-IDF Tab5 build after exporting ESP-IDF `v5.4.2`:

```bash
./tools/official_firmware_ubuntu.sh build-idf
```

For the validated 2026-07-08 Ubuntu baseline, ESP-IDF `v5.4.2` was installed in
the ignored project-local toolchain tree. Use this environment shape:

```bash
export IDF_TOOLS_PATH="$PWD/toolchains/ubuntu/espressif"
source toolchains/ubuntu/esp-idf-v5.4.2/export.sh
cd worktrees/ubuntu/M5Tab5-UserDemo/platforms/tab5
idf.py build
idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_30:ED:A0:E2:E2:48-if00 flash
```

Optional desktop smoke test:

```bash
./tools/official_firmware_ubuntu.sh build-desktop
```

## Current Ubuntu Status

The current Ubuntu 24.04 host has a complete project-local ESP-IDF `v5.4.2`
environment for `esp32p4` under `toolchains/ubuntu/`.

The official repo has been cloned under `worktrees/ubuntu/M5Tab5-UserDemo/` at
commit `68b19d3`, with the four `repos.json` dependencies present at the same
shallow-clone commits recorded in `docs/current_work.md`.

`idf.py build` passed for `platforms/tab5`. The generated `m5stack_tab5.bin`
was flashed to the connected ESP32-P4 Tab5 over USB Serial/JTAG. A subsequent
raw serial boot capture reached `AppLauncher`, including display/touch, USB
host, RS485, and Wi-Fi/AP initialization logs.

As of 2026-07-09, the Ubuntu worktree also has local terminal-entry and
screenshot-debug patches applied. As of 2026-07-11 it also has the automatic
Module Fan service:

- `port/official_firmware_patches/0001-launcher-terminal-button.patch`
- `port/official_firmware_patches/0002-screen-capture-debug.patch`
- `port/official_firmware_patches/0003-module-fan-auto-control.patch`

The patched build was flashed to `/dev/ttyACM0` and verified by:

- boot log reaching `AppStartupAnim on close`, `AppLauncher on open`, and
  `screen-capture debug command task started`;
- host screenshot command:
  `python3 tools/capture_screen.py --port /dev/ttyACM0 --output .logs/screenshots/tab5-launcher-terminal-entry-fixed.png --open-delay 8 --timeout 90`;
- screenshot metadata: `1280x720`, `rgb565le`, CRC32 `2B8CB402`.

The 2026-07-11 Module Fan build used the same project-local ESP-IDF `v5.4.2`
environment. Because new files were added beneath an already-configured CMake
glob, the first build required:

```bash
idf.py reconfigure build
```

The final repeated `idf.py build` passed. `m5stack_tab5.bin` is `0x57d680`
bytes with `0x482980` bytes (`45%`) free in the smallest app partition. Its
SHA256 is
`cfd050f1c560f87daf2b1e56b473ba5d67f3ba6707ace8875c75153ad1eb69d0`.

The final image was flashed through the stable Espressif by-id path for serial
`30:ED:A0:E2:E2:48`. Esptool identified ESP32-P4 revision `v1.3`, and all three
flashed images reported verified hashes.

When hardware returns, controlled fan telemetry can be observed with one
serial-port owner:

```bash
python3 tools/module_fan_telemetry.py stream --port /dev/ttyACM0
```

Do not run that tool concurrently with `tools/capture_screen.py` or another
serial monitor on the same device.

Runtime validation found the real Module Fan at `0x18`, firmware `0x01`. The
current accepted start thresholds are `40/50/60/65 C`, with 5 C lower exit
thresholds and `29/49/69/98%` duty. After rebuilding and re-flashing this curve,
samples at `48.1 C` selected level 1 and 29%, with measured RPM from `4740` to
`5190` and no communication failures.

Opening the USB Serial/JTAG TTY resets this board. The fan telemetry tool waits
8 seconds by default for the debug task. The screenshot tool waits 15 seconds
for complete launcher rendering; the final settled screenshot is
`.logs/screenshots/tab5-module-fan-final-settled.png`, CRC32 `90CEBF21`.

### 2026-07-11 Runtime Fan Policy Build And Flash

The runtime-configurable policy revision was built with:

```bash
export IDF_TOOLS_PATH="$PWD/toolchains/ubuntu/espressif"
source toolchains/ubuntu/esp-idf-v5.4.2/export.sh
cd worktrees/ubuntu/M5Tab5-UserDemo/platforms/tab5
idf.py reconfigure build
idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_30:ED:A0:E2:E2:48-if00 flash
```

The current `m5stack_tab5.bin` is `0x580590` bytes, leaving `0x47fa70` bytes
(`45%`) in the smallest app partition. SHA256:
`39f8d105cb79efc788ab54d062187a2e4bf5b32bfdb96acb3f67c6cad3b191ae`.
Esptool again identified ESP32-P4 revision `v1.3`, MAC
`30:ed:a0:e2:e2:48`, and verified all three written images.

Policy management uses one serial owner:

```bash
python3 tools/module_fan_policy.py get --port /dev/ttyACM0
python3 tools/module_fan_policy.py set \
  --curve 40.0,35.0,29/50.0,45.0,49/60.0,55.0,69/65.0,60.0,98 \
  --port /dev/ttyACM0
python3 tools/module_fan_policy.py set-interval --interval 5000 --save \
  --port /dev/ttyACM0
python3 tools/module_fan_policy.py defaults --save --port /dev/ttyACM0
```

`set`, `set-interval`, and `set-failsafe` are RAM-only unless `--save` is
present. Because this board resets when the port opens, use one invocation with
`--save` when a changed value must survive. `--samples N` keeps telemetry in the
same session and sends the stop command before exit.

Hardware validation proved that an unsaved 55 C first threshold disappeared
after reset, while a saved 6000 ms interval survived reset and produced samples
exactly 6000 ms apart. The final saved state was restored to interval 5000 ms,
failsafe 50%, and curve `40/35@29`, `50/45@49`, `60/55@69`, `65/60@98`.
Final query reported `source=nvs dirty=0`; at `45.1 C` the fan selected level 1,
applied 29%, and measured `4680-5100 RPM`. Telemetry was disabled before the
host tool exited.

The final launcher smoke-test screenshot is
`.logs/screenshots/tab5-module-fan-runtime-policy.png`: `1280x720`, RGB565LE,
CRC32 `21484605`, PNG SHA256
`7599bfce2c625491744481b619f2401f6a6a30114003b5ca8f742b7d3a55471e`.
Visual inspection confirmed a complete official launcher rather than the prior
white-screen failure mode.

## Current Windows Status

The current Windows host has `git`, `python`, `cmake`, and `ninja`, but does
not have `idf.py` exported in the shell. Therefore an ESP-IDF Tab5 firmware
build cannot complete in this session until ESP-IDF `v5.4.2` is installed and
exported.

The current Windows host also does not expose a desktop C/C++ compiler in PATH.
The official desktop smoke build stops before source compilation for that
reason. Use a Visual Studio Developer PowerShell, MinGW, or LLVM if the desktop
smoke build is needed on Windows.

The official repo has been cloned under `worktrees/windows/M5Tab5-UserDemo/`.
Dependencies are cloned from `repos.json` with shallow clones so a dependency
fetch does not spend a long time on unrelated history.
