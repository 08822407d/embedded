# Decision Log

## 2026-07-09: Use USB Serial/JTAG OSC 777 For LVGL Screenshots

Decision: add a private debug command on USB Serial/JTAG using the existing
OSC-777 style frame:

`ESC ] 777 ; screen-capture? BEL`

The device replies with a small ASCII `TAB5SHOT BEGIN` header, raw RGB565LE
frame bytes from `lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565)`,
and a `TAB5SHOT END crc32=...` footer. The host tool converts the stream to
PNG and checks CRC before writing output files.

Reason: this keeps GUI debugging scriptable from Codex without adding an
interactive monitor dependency. It also matches the archived PlatformIO
terminal firmware's debug convention closely enough that old host-side ideas
remain reusable.

Implementation detail: large binary transfers use the ESP-IDF
`usb_serial_jtag` driver with chunking, periodic TX flushes, and temporary log
level reduction. Plain console `write()` was not reliable enough for full
screen RGB565 frames.

## 2026-07-09: Start Screenshot Debug After Launcher Settles

Decision: start the screenshot command task from `app_main()` only after the
main app loop has run for a short time, and install the USB Serial/JTAG driver
lazily when a screenshot command is actually served.

Reason: the screenshot facility is for debugging normal GUI state. It should
not perturb the official firmware's startup sequence or claim the USB console
driver before the UI is ready.

## 2026-07-09: Add A Short Post-HAL LVGL Settle Delay

Decision: wait 250 ms after HAL initialization before opening the startup
animation app.

Reason: after adding the debug path and testing with repeated USB
Serial/JTAG resets, the app could reach `AppStartupAnim on open` and remain on
the white startup background. A short settle delay let the freshly started
display/LVGL port become ready before the first app-level LVGL lock. Boot logs
then reached `AppLauncher on open`, and host screenshots captured the launcher.

## 2026-07-09: Use A Visible LVGL Overlay For The First Terminal Entry

Decision: replace the launcher power panel's shake-wakeup and 10-second sleep
entries with one visible LVGL/smooth_ui_toolkit `Container` button that draws a
`>_` terminal glyph and owns the combined old hit region.

Reason: the displayed launcher labels for these entries are not source strings
in `panel_power.cpp`; they appear to be baked into the generated launcher
background asset. A visible LVGL overlay cancels the old touch behavior, hides
the obsolete labels in the current art, and avoids regenerating bitmap assets
before the terminal GUI design is mature.

Rejected direction for now: editing `launcher_bg.c` directly. It is generated
image data and would make the first terminal-entry iteration harder to review
and replay across fresh official-firmware worktrees.

## 2026-07-07: Separate Official Firmware Worktrees By Host OS

Decision: keep official firmware clones and build outputs under ignored
host-specific paths:

- `worktrees/windows/`
- `worktrees/ubuntu/`
- `toolchains/windows/`
- `toolchains/ubuntu/`

Reason: the user expects development on both Windows 10 and Ubuntu 24.04.
ESP-IDF exports, Python environments, desktop CMake caches, path separators, and
native compiler choices are host-specific. Sharing one official firmware build
directory would make failures hard to diagnose and could corrupt a working
configuration from the other host.

## 2026-07-07: Archive PlatformIO Project In-Repo

Decision: move the existing standalone PlatformIO Tab5 terminal firmware into
`archive/platformio_tab5_terminal_20260707/`.

Reason: the project direction changed from standalone firmware to porting
terminal functionality into the M5Stack Tab5 official firmware. Keeping the old
project in-tree preserves working code, docs, validation scripts, hardware
findings, and debug history for reference.

## 2026-07-07: Use libvterm As Terminal Emulation Target

Decision: use `libvterm` as the terminal core for the official firmware port.

Reason: `libvterm` is a C terminal-emulation library with a screen model and
damage callbacks, which is a better fit for embedded GUI integration than
directly porting a full desktop terminal application.

Rejected direction for now: continue expanding the old custom `terminal_core`
as the official-firmware terminal core.

## 2026-07-07: Keep GUI Binding Thin

Decision: start with a GUI-neutral adapter, not a concrete popup design.

Reason: the user has not specified the official GUI layout, and the official
firmware source/layout has not yet been inspected. The known GUI requirement is
only that a button opens a terminal view.

## 2026-07-07: Vendor A Source Snapshot For Initial Work

Decision: import a source snapshot under `third_party/libvterm-src/`.

Reason: this allows adapter work to proceed immediately while the official
firmware repository layout and dependency policy are still unknown.

Follow-up: decide later whether the official firmware should use a vendored
snapshot, subtree, submodule, or package import.
