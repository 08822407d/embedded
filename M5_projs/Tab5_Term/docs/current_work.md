# Current Work

## 2026-07-08 Ubuntu Official Firmware Baseline

Official firmware source was freshly cloned on Ubuntu 24.04 into the ignored
worktree:

- Ubuntu worktree: `worktrees/ubuntu/M5Tab5-UserDemo/`
- Official repo commit: `68b19d3`
- Dependency commits from `repos.json` shallow clones:
  - `mooncake`: `0dfc177`
  - `mooncake_log`: `41d00d9`
  - `lvgl`: `7f07a129e`
  - `smooth_ui_toolkit`: `3a749e5`

ESP-IDF `v5.4.2` was installed under ignored project-local tool paths:

- ESP-IDF source: `toolchains/ubuntu/esp-idf-v5.4.2/`
- `IDF_TOOLS_PATH`: `toolchains/ubuntu/espressif/`
- ESP-IDF commit: `f5c3654`

Ubuntu build result:

- Command shape:
  `export IDF_TOOLS_PATH="$PWD/toolchains/ubuntu/espressif"; source toolchains/ubuntu/esp-idf-v5.4.2/export.sh; cd worktrees/ubuntu/M5Tab5-UserDemo/platforms/tab5; idf.py build`
- Build passed with ESP-IDF `v5.4.2`.
- App: `m5stack_tab5`, version `V0.4`.
- Binary: `build/m5stack_tab5.bin`.
- Binary size: `0x57bab0` bytes.
- Smallest app partition: `0xa00000` bytes.
- Free app partition space: `0x484550` bytes, `45%`.

Ubuntu flash result:

- Target port:
  `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_30:ED:A0:E2:E2:48-if00`
  -> `/dev/ttyACM0`
- Detected chip: ESP32-P4 revision `v1.3`, MAC `30:ed:a0:e2:e2:48`.
- Command shape:
  `idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_30:ED:A0:E2:E2:48-if00 flash`
- Flash passed at `460800` baud.
- Bootloader, app, and partition table writes all reported verified hashes.

Runtime boot-log validation:

- A raw pyserial capture after `esptool chip_id` hard reset recorded `14413`
  bytes at `115200` baud.
- The boot log reached `app_main()`, initialized PSRAM, I2C, codec, IMU, INA226,
  RTC, display, touch, USB host, HID, RS485, and Wi-Fi/AP services.
- Display/touch evidence included ST7123 detection, display resolution
  `720x1280`, touch firmware version `3(1.71.1.3)`, and LCD backlight `100%`.
- The app reached `AppStartupAnim`, then closed it and opened `AppLauncher`.
- Wi-Fi AP startup was logged as `M5Tab5-UserDemo-WiFi` with DHCP server
  `192.168.4.1`.
- Nonfatal runtime anomalies seen in this baseline log: RTC/system time synced to
  `2062-03-07`, LEDC warned that GPIO 22 was not usable or conflicted, and the
  audio path logged one `Mode 1 conflict sample_rate 44100 with 48000` error.
  The firmware continued into `AppLauncher`, so treat these as official-baseline
  observations unless they become user-visible blockers.

Notes:

- `idf.py monitor` needs a TTY-backed stdin. Non-TTY Codex command execution
  fails before opening the monitor. A PTY-backed timed monitor started but did
  not emit boot text in the capture window; raw pyserial after an esptool reset
  was the reliable boot-log method in this session.
- Build output had warnings from the official source and IDF config, but no
  compiler/linker/build failure. Do not treat those warnings as terminal-port
  regressions unless they become runtime blockers.

## 2026-07-07 Official Firmware Build Probe

Official firmware source has been cloned locally for build integration work:

- Windows worktree: `worktrees/windows/M5Tab5-UserDemo/`
- Official repo commit: `68b19d3`
- Dependency commits from `repos.json` shallow clones:
  - `mooncake`: `0dfc177`
  - `mooncake_log`: `41d00d9`
  - `lvgl`: `7f07a129e`
  - `smooth_ui_toolkit`: `3a749e5`

New host-specific helper scripts:

- Windows: `tools/official_firmware_windows.ps1`
- Ubuntu 24.04: `tools/official_firmware_ubuntu.sh`

The scripts use separate ignored paths:

- `worktrees/windows/`
- `worktrees/ubuntu/`
- `toolchains/windows/`
- `toolchains/ubuntu/`

Windows build probe results:

- `.\tools\official_firmware_windows.ps1 -Action check`: passed as a tool
  inventory command. It found `git`, `python`, `cmake`, and `ninja`.
- `.\tools\official_firmware_windows.ps1 -Action build-idf`: blocked before
  firmware compilation because `idf.py` is not available and `IDF_PATH` is not
  set.
- `.\tools\official_firmware_windows.ps1 -Action build-desktop`: blocked before
  source compilation because no C/C++ compiler is visible in PATH.

This means no current failure is attributed to the official firmware source or
the `libvterm` adapter. The Windows host needs ESP-IDF `v5.4.2` exported before
the real Tab5 firmware build can be attempted.

## 2026-07-07 PlatformIO Archive And Official Firmware Port Start

The standalone PlatformIO Tab5 terminal firmware has been moved into
`archive/platformio_tab5_terminal_20260707/`.

This is an intentional project direction change. The active work is now to port
terminal functionality into the M5Stack Tab5 official firmware. The selected
terminal emulation target is `libvterm`, not the previous home-grown
`terminal_core`.

Board status: the physical Tab5 is not currently available, so this phase must
avoid build, flash, or hardware validation steps.

GUI scope for now: no final GUI design is required. The only product-level
assumption is that the official firmware GUI will add one button that opens a
terminal popup/view.

Done in this checkpoint:

- Archived tracked PlatformIO source, docs, tests, tools, and `platformio.ini`.
- Moved local ignored PlatformIO/VSCode/build-output directories into the same
  archive area for recovery.
- Imported `libvterm` source snapshot into `third_party/libvterm-src/`.
- Added a first-pass `port/libvterm_adapter` wrapper that separates terminal
  emulation from GUI rendering and transport.
- Added new repo-local persistent records for requirements, decisions,
  problems, and project operating notes.

Next work:

- Wire the adapter into the actual official firmware tree once its source is
  present in this workspace.
- Decide whether `third_party/libvterm-src` remains a vendored snapshot or is
  replaced by a submodule/subtree once the official firmware repository layout
  is known.
- Add build-system integration for the official firmware build, not PlatformIO.
- Implement the GUI popup glue after the official firmware GUI framework and
  navigation model are inspected.

Known unverified items:

- `libvterm` source has not been compiled in this repository after import.
- Adapter code has not been compiled against the official firmware.
- Runtime memory use on ESP32-P4 has not been measured.
- No rendered output has been tested on a physical Tab5 in this phase.

Source-level checks run in this checkpoint:

- `.\tools\check_port_layout.ps1`: passed.
- `git diff --check -- . ':!third_party/libvterm-src'`: passed.

Build and hardware checks intentionally skipped because the board and official
firmware build tree are not available in this workspace phase.
