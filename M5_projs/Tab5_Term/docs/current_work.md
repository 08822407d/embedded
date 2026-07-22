# Current Work

## 2026-07-18 Window-Style Launcher Terminal Entry

The baked terminal entry now uses the accepted compact window-style artwork
instead of the earlier plain 4:3 screen. The official `launcher_bg.c` remains
unchanged; the customized launcher still selects the independent
`launcher_bg_terminal.c` RGB565 asset.

Implementation:

- The 144x108 window occupies `x=1130..1273`, `y=166..273`. Its outer corner
  radius is 16 px and its title bar is 17 px high.
- The title bar has red, yellow, and green 9x9 control dots. All three share
  the same top coordinate (`y=172`) and spacing. The red dot starts at
  `x=1146`, the window corner's horizontal tangent, so every dot lies beneath
  the same straight top edge.
- The `>_` prompt and `Term` label have independent nominal font sizes: 34 px
  and 20 px respectively. The rendered prompt is 35x21 px at `x=1143`,
  `y=194`, leaving 9 px from both the inner left edge and the content area's
  top edge. `Term` is centered beneath the window.
- The existing transparent 146x144 `PanelPower` touch target, click tone, and
  placeholder callback are unchanged.
- `tools/update_launcher_terminal_asset.py` decodes the existing RGB565 C
  array, replaces only `x=1127..1277`, `y=151..321` from a reviewed 1280x720
  preview, rewrites the original row-oriented C format, and refuses output if
  any pixel outside that rectangle changes.
- `port/official_firmware_patches/0001-launcher-terminal-button.patch` carries
  the copied asset and launcher changes for replay on official commit
  `68b19d3`.

Validation:

- ESP-IDF `v5.4.2` `idf.py reconfigure build` passed. The application remains
  `0x580290` bytes, leaving `0x47fd70` bytes (`45%`) in the smallest app
  partition. Binary SHA256 is
  `c63371e9e3158c12da21d39953dbf89389c4ee6c76b32578759af60d78030b00`.
- Flash through the stable Espressif by-id path passed. Esptool identified the
  ESP32-P4 rev `v1.3`; bootloader, application, and partition writes all
  reported verified hashes.
- The settled CDC screenshot is
  `.logs/screenshots/tab5-launcher-terminal-window-v12.png`: 1280x720
  RGB565LE, CRC32 `D19CF65E`, raw SHA256
  `d13f299e9daae43db03443285d6f086bcb12be00dd68e2d2b5776ed91c2c6802`,
  and PNG SHA256
  `d1dc4e3b3fe032b8cf7012e647024177fe5d93c24b1a5e9120c891a9d936472e`.
- Visual inspection confirms the accepted geometry and clean RGB565 colors.
  The 51,642-byte edited rectangle in the captured framebuffer is
  byte-for-byte identical to the baked asset, with zero differing bytes.
- A clean detached worktree at official commit `68b19d3` accepted and applied
  patches `0001`, `0002`, and `0003` in order; `git diff --check` passed after
  the complete stack was applied.

Next work:

- Directly tap-test the transparent terminal hit region when convenient. Its
  callback is unchanged, but this revision was validated by build, flash, and
  screenshot rather than physical touch input.
- Replace the placeholder callback with the real terminal view.

## 2026-07-11 Runtime-Configurable Module Fan Policy

The automatic Module Fan policy can now be changed without rebuilding or
flashing the application. Runtime changes take effect immediately in RAM;
Flash is written only by an explicit save command. The final device state is
the accepted `40/50/60/65 C` policy saved in NVS with a 5-second interval.

Implementation:

- `module_fan_policy.*` owns the validated four-level policy and hysteresis
  state. Temperatures are stored as 0.1 C fixed-point values; the control path
  still consumes the existing floating-point sensor reading.
- `module_fan_config_store.*` stores a fixed-width, versioned 48-byte blob in
  NVS namespace `module_fan`, key `policy_v1`. Missing or invalid data falls
  back to compiled defaults.
- `persistent_storage.*` provides serialized, idempotent NVS initialization.
  The existing Wi-Fi path now uses the same helper instead of independently
  initializing NVS.
- `module_fan_service.*` protects active/persisted policy state with a mutex and
  publishes source, dirty state, and revision. A changed policy notifies the
  automatic task, which samples and applies it immediately; normal waits use
  the configured interval.
- NVS writes run only in the debug command task after explicit `save`; the
  periodic fan task never writes Flash.
- `app/debug/module_fan_policy_commands.*` adds OSC-777 query, whole-curve set,
  interval set, failsafe set, save, load, and defaults commands. The complete
  candidate is validated before the active configuration is replaced.
- `tools/module_fan_policy.py` provides matching host actions and can save or
  collect controlled telemetry in the same serial session. This matters on the
  current host because opening USB Serial/JTAG resets the Tab5.

Validation:

- A host C++ policy test passed for threshold entry, hysteresis exit, highest
  level, sensor-read failsafe, and invalid threshold ordering.
- ESP-IDF `v5.4.2` `idf.py reconfigure build` passed. The app is `0x580590`
  bytes, with `0x47fa70` bytes (`45%`) free. SHA256 is
  `39f8d105cb79efc788ab54d062187a2e4bf5b32bfdb96acb3f67c6cad3b191ae`.
- Flash through the stable by-id path passed; bootloader, application, and
  partition table all reported verified hashes.
- First boot with no stored policy returned `source=defaults dirty=0`, interval
  `5000`, failsafe `50`, and the accepted curve.
- A RAM-only test raised the first level to 55 C. At `45.1 C`, policy revision
  `2` immediately applied 0%; RPM settled from residual `4830` to `0`. After a
  USB-triggered reset, the unsaved curve disappeared and the accepted 29% level
  resumed, reaching `5070 RPM`.
- A 6000 ms interval was explicitly saved. Two samples were exactly 6000 ms
  apart, and a subsequent reset returned `source=nvs interval_ms=6000`.
- Compiled defaults were then restored and explicitly saved. Final reset
  verification returned `source=nvs dirty=0 interval_ms=5000`; samples at
  `45.1 C` selected level 1, applied 29%, and measured `4680` and `5100 RPM`.
- A deliberately invalid curve with duplicate enter thresholds returned
  `TAB5FAN POLICY ERR action=set reason=invalid-curve` and was not applied.
- A final LVGL screenshot after another USB-triggered reset was complete at
  `1280x720`, RGB565LE, CRC32 `21484605`. The PNG is
  `.logs/screenshots/tab5-module-fan-runtime-policy.png`, SHA256
  `7599bfce2c625491744481b619f2401f6a6a30114003b5ca8f742b7d3a55471e`.
- Controlled telemetry was stopped at the end. Automatic control remains
  active independently of telemetry.

## 2026-07-11 Module Fan Threshold Revision

The accepted automatic start thresholds are now `40/50/60/65 C`. The existing
5 C hysteresis is preserved, so the corresponding exit thresholds are
`35/45/55/60 C`. Duty remains `29/49/69/98%`.

Validation:

- ESP-IDF `v5.4.2` build passed. The image remains `0x57d680` bytes with
  `0x482980` bytes (`45%`) free in the smallest app partition.
- Final binary SHA256:
  `cfd050f1c560f87daf2b1e56b473ba5d67f3ba6707ace8875c75153ad1eb69d0`.
- Flash to the stable Tab5 USB Serial/JTAG by-id path passed, with verified
  hashes for bootloader, app, and partition table.
- At `48.1 C`, five consecutive samples selected level `1`, target/applied duty
  `29%`, firmware `0x01`, and RPM values `4740`, `4770`, `4800`, `4800`, and
  `5190`, with `comm_ok=1` and no missed polls.
- Ctrl-C stopped telemetry and returned
  `TAB5FAN LOG enabled=0 interval_ms=5000`. Automatic fan control remains active
  independently of telemetry.
- The board is left running this revised formal policy. At the last measured
  `48.1 C`, its automatic target is 29%.

## 2026-07-11 Module Fan Automatic Control

The Ubuntu official-firmware worktree now contains a native ESP-IDF automatic
controller for M5Stack Module Fan v1.1. Manual duty control is intentionally
not part of this checkpoint.

Implementation:

- `platforms/tab5/main/hal/components/cpu_temperature.*` owns the ESP32-P4
  internal temperature-sensor handle and serializes reads shared by the
  existing CPU-temperature GUI and fan service.
- `platforms/tab5/main/hal/components/module_fan/` contains the fan-specific
  device and service files. The device implements the register protocol on the
  existing BSP system-I2C handle: address `0x18`, identity/firmware checks,
  24 kHz PWM, volatile duty changes, and RPM reads.
- `module_fan_service.*` runs one low-priority task at a 5-second interval. It
  starts only after the official HAL init path has completed. The compact
  policy table remains at the top of this file for straightforward tuning.
- `app/debug/module_fan_telemetry.*` owns fan telemetry command state and line
  formatting. `screen_capture_debug.cpp` remains responsible for the shared
  USB transport, OSC framing, output serialization, and screenshot stream.
- The automatic curve uses enter/exit temperature, duty pairs of
  `40/35 C -> 29%`, `50/45 C -> 49%`, `60/55 C -> 69%`, and
  `65/60 C -> 98%`. A failed temperature read requests a 50% failsafe duty.
- Duty is written only when the target changes. A communication error forces
  PWM configuration and duty to be re-applied on the next poll in case the
  module reset during a short disconnect. Three failed attached polls mark the
  module detached and remove its ESP-IDF I2C device handle. The service retains
  only the shared bus reference and its low-rate task for hot-plug detection.
- Existing EXT 5V policy is unchanged. Turning EXT 5V off through the official
  GUI makes an M5-Bus fan disappear; turning it back on allows the next probe
  to attach it again.

Controlled debug telemetry extends the existing OSC-777 USB Serial/JTAG task:

- `ESC ] 777 ; module-fan-log=start BEL` enables one `TAB5FAN SAMPLE` line for
  each new 5-second snapshot.
- `ESC ] 777 ; module-fan-log=stop BEL` disables it.
- `ESC ] 777 ; module-fan-log? BEL` reports whether it is enabled.
- Each sample contains an `uptime_ms` timestamp, temperature and validity,
  attachment/communication state, cooling level, target/applied duty, RPM,
  firmware version, and missed-poll count.
- The fan task never writes the serial port. The existing debug-command task
  owns command replies, fan telemetry, and screenshot streaming, so telemetry
  cannot be inserted inside a screenshot binary frame.
- `tools/module_fan_telemetry.py` provides `start`, `stop`, `status`, and
  `stream` actions. `stream` sends stop on normal Ctrl-C cleanup.

Portable replay artifact:

- `port/official_firmware_patches/0003-module-fan-auto-control.patch`
- The patch was generated relative to official HEAD with patches `0001` and
  `0002` already applied. A temporary-index replay check confirmed that all
  three patches reproduce the current fan-related worktree files exactly.

Validation:

- A first incremental link failed because the existing CMake glob had been
  configured before the new source files existed. `idf.py reconfigure`
  refreshed the source list; this was a build-cache issue rather than a source
  or API error.
- A clean reconfigured ESP-IDF `v5.4.2` build passed.
- Final app binary: `build/m5stack_tab5.bin`.
- Binary size: `0x57d680` bytes.
- Binary SHA256:
  `cfd050f1c560f87daf2b1e56b473ba5d67f3ba6707ace8875c75153ad1eb69d0`.
- Smallest app partition: `0xa00000` bytes.
- Free app partition space: `0x482980` bytes, `45%`.
- New code produced no compiler warnings in the final incremental build. The
  remaining warnings in `hal_esp32.cpp` predate this feature.
- Both Python debug tools pass `python3 -m py_compile`.

Hardware validation:

- The connected port was identified as Espressif `303A:1001`, serial
  `30:ED:A0:E2:E2:48`, at the stable by-id path ending in
  `30:ED:A0:E2:E2:48-if00`. Esptool confirmed ESP32-P4 revision `v1.3` and the
  matching MAC before flash.
- The final build was flashed successfully. Bootloader, app, and partition
  table writes all reported verified hashes.
- Before the threshold revision, the `50/45 C` first threshold produced samples
  at `43.1 C` and `45.1 C` with `attached=1`, `comm_ok=1`, fan firmware `0x01`,
  level `0`, target/applied duty `0`, and settled RPM `0`.
- An earlier temporary validation build lowered only the first threshold to
  `40/35 C`.
  At `45.1 C` it automatically selected level `1` and 29% target/applied duty;
  RPM progressed from startup `0` to `5040`, `5040`, and `5190`. This threshold
  was later accepted as the formal first level by the user.
- On the previous 50 C-start image, the first sample observed residual fan coast at
  `4890 RPM`, followed by repeated `0 RPM` samples with duty `0`. Ctrl-C sent
  the stop command and received `TAB5FAN LOG enabled=0 interval_ms=5000`.
- A separate final `module_fan_telemetry.py status` invocation also returned
  `enabled=0`, verifying the query branch after the telemetry source split.
- Screenshot capture after telemetry remained CRC-clean. An 8-second capture
  caught an incompletely rendered launcher after USB-open reset; a settled
  15-second capture was visually complete at `1280x720`, RGB565LE, CRC32
  `90CEBF21`, SHA256
  `99cbc10ac26a5271d31c8d001fbbb5f2c2187f06d7a38ffffdd051f0a8a1bc12`.
  The verified PNG is
  `.logs/screenshots/tab5-module-fan-final-settled.png`.
- Opening this USB Serial/JTAG TTY causes `CHIP_USB_UART_RESET`. The telemetry
  tool now waits 8 seconds by default for the debug task; the screenshot tool
  waits 15 seconds for the launcher to settle.

Remaining hardware checks:

- Physical unplug/replug and EXT 5V off/on detach/re-attach behavior were not
  exercised while the user was away from the board.
- Higher automatic levels, the temperature-read failsafe path, and three-poll
  detach recovery remain untested on hardware.
- The board is left running the revised `40/50/60/65 C` start-threshold firmware
  with telemetry disabled. At the last observed `48.1 C`, the fan target is
  29%.

## 2026-07-09 End-Of-Day Handoff

Today's official-firmware checkpoint is usable as the next starting point.

State to preserve:

- The connected Tab5 currently has a patched official firmware flashed from the
  Ubuntu worktree. It includes the launcher terminal-entry placeholder and the
  USB Serial/JTAG screenshot debug path.
- Build, flash, boot-log, and screenshot validation passed after the white
  startup-screen fix. The verified screenshot is
  `.logs/screenshots/tab5-launcher-terminal-entry-fixed.png` with CRC32
  `2B8CB402`.
- The official firmware source changes remain in the ignored worktree
  `worktrees/ubuntu/M5Tab5-UserDemo/`.
- Portable replay artifacts are tracked as patches:
  - `port/official_firmware_patches/0001-launcher-terminal-button.patch`
  - `port/official_firmware_patches/0002-screen-capture-debug.patch`
- The host capture tool to keep is `tools/capture_screen.py`.

Repository handoff notes:

- The git root for this checkout is `/home/cheyh/projs/embedded`, not this
  `Tab5_Term` directory.
- The parent repo remote is `origin = git@github.com:08822407d/embedded.git`.
  There is no `master` remote. A literal `git push -u master` would fail before
  pushing.
- Use path-scoped staging for this project, for example
  `git add --all M5_projs/Tab5_Term`, before committing from the parent repo.
- `worktrees/` and `toolchains/` are ignored. Parent-repo commits should carry
  docs, tools, archived/reference code, and patch files, not the whole official
  firmware checkout or ESP-IDF toolchain.

Recommended first actions next session:

- Confirm the Tab5 USB Serial/JTAG port before flashing or capturing again.
- Touch-test the terminal overlay on the physical device and verify that the
  old two sleep regions no longer trigger their original actions.
- Start replacing the placeholder terminal click handler with the first real
  terminal popup/view.
- Keep using `tools/capture_screen.py` after UI changes so visual regressions
  are observable from the host.

## 2026-07-09 Screenshot Debug And Runtime Launcher Validation

The Ubuntu official-firmware worktree now includes the first automated screen
capture debug path:

- Device-side source files changed in ignored official worktree:
  - `worktrees/ubuntu/M5Tab5-UserDemo/app/app.cpp`
  - `worktrees/ubuntu/M5Tab5-UserDemo/app/debug/screen_capture_debug.cpp`
  - `worktrees/ubuntu/M5Tab5-UserDemo/app/debug/screen_capture_debug.h`
  - `worktrees/ubuntu/M5Tab5-UserDemo/platforms/tab5/main/CMakeLists.txt`
  - `worktrees/ubuntu/M5Tab5-UserDemo/platforms/tab5/main/app_main.cpp`
- Host-side capture tool:
  - `tools/capture_screen.py`
- Portable tracked patch:
  `port/official_firmware_patches/0002-screen-capture-debug.patch`

Behavior added:

- The firmware listens on USB Serial/JTAG for the private OSC frame
  `ESC ] 777 ; screen-capture? BEL`.
- On request, it snapshots `lv_screen_active()` with LVGL's snapshot API,
  streams RGB565 little-endian bytes framed by `TAB5SHOT BEGIN` and
  `TAB5SHOT END crc32=...`, and throttles output through the
  `usb_serial_jtag` driver so large captures do not corrupt the CDC stream.
- The host tool writes PNG, raw RGB565, and JSON metadata files.
- The screenshot task starts after the launcher main loop has had a short
  chance to run. The USB Serial/JTAG driver is installed lazily only when a
  screenshot command is handled.
- A 250 ms post-HAL settle delay was added before `AppStartupAnim` is opened.
  Without this delay, this build could reach `AppStartupAnim on open` and then
  remain on the white startup background, apparently due to the first LVGL lock
  attempt racing the freshly started display/LVGL port.

Validation:

- `idf.py build` passed with ESP-IDF `v5.4.2`.
- App binary: `build/m5stack_tab5.bin`.
- Binary size: `0x57cac0` bytes.
- Smallest app partition: `0xa00000` bytes.
- Free app partition space: `0x483540` bytes, `45%`.
- `idf.py -p /dev/ttyACM0 flash` passed. Bootloader, app, and partition table
  writes all reported verified hashes.
- A raw pyserial boot-log capture after USB Serial/JTAG reset showed:
  `AppStartupAnim on close`, `AppLauncher on create`, `AppLauncher on open`,
  `launcher-view init`, and `screen-capture debug command task started`.
- Screenshot command passed:
  `python3 tools/capture_screen.py --port /dev/ttyACM0 --output .logs/screenshots/tab5-launcher-terminal-entry-fixed.png --open-delay 8 --timeout 90`
- Captured image metadata:
  - Resolution: `1280x720`
  - Format: `rgb565le`
  - CRC32: `2B8CB402`
  - SHA256:
    `f9c1dad8a6ccfc7cd97d0ae00fc6313d6d6249d6850acca39361a8c56390f022`
  - PNG:
    `.logs/screenshots/tab5-launcher-terminal-entry-fixed.png`

Visual result:

- The screenshot shows the official launcher rather than the previous white
  startup background.
- The terminal entry overlay is visible in the right-side power panel region,
  covering the old shake/RTC sleep entries with a dark rounded `>_` button.
- The old top `Sleep & Touch To Wakeup` button remains unchanged.

Next work:

- Touch-test the new terminal overlay on the physical Tab5 and confirm the
  placeholder click handler fires while the old two regions do not.
- Replace the placeholder click handler with the real terminal popup/view.
- Keep `tools/capture_screen.py` available for future GUI layout debugging.

## 2026-07-09 Launcher Terminal Entry Placeholder

Official firmware launcher power panel now has the first terminal entry point
in the Ubuntu worktree:

- Source files changed in ignored official worktree:
  - `worktrees/ubuntu/M5Tab5-UserDemo/app/apps/app_launcher/view/panel_power.cpp`
  - `worktrees/ubuntu/M5Tab5-UserDemo/app/apps/app_launcher/view/view.h`
- Portable tracked patch:
  `port/official_firmware_patches/0001-launcher-terminal-button.patch`

Behavior changed:

- The two right-side launcher power-panel entries for shake wakeup and 10-second
  RTC wakeup are no longer clickable from `PanelPower`.
- Their previous `SleepShakeWakeupWindow` and `SleepRtcWakeupWindow` classes and
  transparent hit regions were removed from this panel.
- A single visible terminal-style button now covers the combined old two-button
  region. It draws a rounded `>_` terminal glyph with LVGL/smooth_ui_toolkit.
- Clicking the new entry currently plays the existing click tone and logs
  `terminal button clicked`. Opening the real terminal view is intentionally not
  implemented yet.

Validation:

- Command shape:
  `export IDF_TOOLS_PATH="$PWD/toolchains/ubuntu/espressif"; source toolchains/ubuntu/esp-idf-v5.4.2/export.sh; cd worktrees/ubuntu/M5Tab5-UserDemo/platforms/tab5; idf.py build`
- Build passed with ESP-IDF `v5.4.2`.
- App binary: `build/m5stack_tab5.bin`.
- Binary size: `0x57ae40` bytes.
- Smallest app partition: `0xa00000` bytes.
- Free app partition space: `0x4851c0` bytes, `45%`.

Next work:

- Flash and visually/touch-test this launcher entry on a physical Tab5 before
  marking runtime behavior verified.
- Replace the placeholder click handler with terminal view/popup launch once the
  terminal GUI container is designed.
- If upstream launcher art changes, recheck whether the old labels remain baked
  into `launcher_bg.c` and whether this overlay still covers them cleanly.

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
