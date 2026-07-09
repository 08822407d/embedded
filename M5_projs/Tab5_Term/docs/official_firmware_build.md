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
