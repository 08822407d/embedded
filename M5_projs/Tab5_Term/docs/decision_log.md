# Decision Log

## 2026-07-18: Use A Metric-Driven Window-Style Terminal Tile

Decision: keep the 144x108 terminal tile but render it as a compact application
window with a 17 px title bar, 16 px outer radius, three 9x9 colored controls,
a 34 px `>_` prompt, and a separate 20 px `Term` label. The prompt is placed by
its rendered 35x21 px bounds with 9 px inner left/top padding. The colored
controls share one vertical coordinate; the first circle begins at the rounded
window's horizontal tangent so all three sit below the same straight edge.

Reason: the application-window silhouette is more recognizable than a plain
screen while preserving the accepted 4:3 footprint. Separate text metrics keep
later label changes from silently resizing or moving the prompt. Deriving the
control group from the corner geometry removes the apparent top-edge mismatch
that occurred when the first circle overlapped the rounded-corner transition.

Decision: use `tools/update_launcher_terminal_asset.py` to merge only the
reviewed launcher rectangle into the existing RGB565 C asset and assert zero
changes outside it.

Reason: visual reviews are made against full screenshots containing live GUI
values. A bounded merge prevents voltage, temperature, time, or other runtime
overlays from accidentally becoming static launcher pixels and makes future
icon iterations reproducible.

## 2026-07-18: Separate The 4:3 Visual Tile From Its Touch Target

Decision: draw the visible terminal tile as a centered 144x108 rectangle while
retaining the existing transparent 146x144 `PanelPower` touch target. Repaint
the original `>_` raster glyph at its native proportions instead of resizing
the old square tile as a whole.

Reason: the nearly square visible tile read as a generic button rather than a
terminal screen. Separating visual bounds from interaction bounds gives the art
an exact 4:3 display proportion without reducing the convenient touch area or
changing its callback. Repainting rather than squashing preserves stroke weight
and glyph shape.

## 2026-07-18: Bake Static Terminal Art Into A Copied Background

Decision: retain the official `launcher_bg.c` unchanged, create the separate
`launcher_bg_terminal.c` RGB565 asset, and make `LauncherView` use the copied
asset. The terminal `Container` remains at the same size and position solely as
a fully transparent touch target; it no longer paints a background, border, or
label.

Reason: the removed sleep labels are baked into the official bitmap. The prior
`245/255`-opaque LVGL overlay allowed them to remain faintly visible, and static
launcher art does not need a second runtime rendering layer. A named copy keeps
the upstream asset available for comparison and makes the customized main
screen explicit. Linker section garbage collection retains only the selected
1.8 MiB map, so this source-level copy does not duplicate the final firmware
payload.

This supersedes the 2026-07-09 visible-overlay decision. The historical entry
below remains as the reason the first placeholder was implemented quickly.

## 2026-07-11: Use A Validated Runtime Policy With Explicit NVS Save

Decision: split automatic fan selection into `module_fan_policy.*`, keep one
thread-safe active policy in the service, and expose atomic updates through the
existing OSC-777 debug command task. Runtime updates wake the control task and
apply immediately. Only `module-fan-policy=save` writes a versioned NVS blob;
the 5-second control loop never writes Flash.

Reason: thresholds, hysteresis, duty, interval, and sensor-failure behavior now
form a real configuration boundary rather than a small compile-time table. A
pure policy object keeps those rules independent of I2C and FreeRTOS, while
whole-candidate validation prevents a partially parsed or unsafe curve from
becoming active. Explicit persistence avoids both rebuilds for normal tuning
and unnecessary Flash wear during experimentation.

Decision: use NVS rather than a text file as the primary store. The official
firmware already has a 24 KiB NVS partition, while its SPIFFS partition is not
mounted in the normal startup path. The stored record has magic, version, size,
level count, fixed-width fields, and reserved bytes; NVS supplies record-level
integrity, and the policy layer supplies semantic validation. Missing or invalid
records select compiled defaults.

Decision: centralize NVS initialization in `persistent_storage.*` and route the
existing Wi-Fi initialization through it. This prevents two features from
independently owning NVS lifecycle while preserving the official recovery
behavior for no-free-pages and version-mismatch errors.

Rejected: periodic auto-save and a primary JSON/SPIFFS settings file. They add
Flash wear or filesystem startup/lifecycle work without improving the current
CDC tuning workflow. A future GUI or file importer can call the same validated
service API without changing the automatic controller.

## 2026-07-11: Revise Module Fan Start Thresholds

Decision: use automatic start thresholds `40/50/60/65 C` with duty
`29/49/69/98%`. Preserve the existing 5 C hysteresis, giving exit thresholds
`35/45/55/60 C`.

Reason: this directly implements the user's revised cooling preference while
retaining the existing table-driven policy and anti-chatter behavior. No task,
I2C, telemetry, or manual-control behavior changes with this adjustment.

## 2026-07-11: Keep Module Fan Control In A Native IDF Device Service

Decision: implement Module Fan v1.1 as a small register-level ESP-IDF device
wrapper plus one 5-second automatic-control service task. Use the BSP-provided
system-I2C handle and the ESP32-P4 internal temperature sensor. Do not port the
Arduino M5Stack fan library or add manual-control state in this checkpoint.

Reason: the official firmware is ESP-IDF-native, and its system I2C bus is
already shared by codec, RTC, camera, power monitor, IO expanders, touch, and
IMU devices. A contained low-rate task adds no calls to the official launcher
loop and lets the IDF I2C driver serialize transactions on the existing bus.
The SoC temperature is also the closest match to the Raspberry Pi-style policy
being reused and was already exposed by the official CPU-temperature panel.

The service does not change EXT 5V. It creates the fan device handle only after
an ACK plus identity-register check, removes that handle after three failed
polls, and keeps only the task and shared bus reference needed for hot-plug
probing. Any communication error also causes PWM configuration and duty to be
re-applied on the next poll, covering a module reset shorter than the three-poll
detach window. Dynamic duty changes are not saved to the module's flash.

## 2026-07-11: Publish Fan Snapshots Through The Existing Debug Writer

Decision: the automatic task publishes a mutex-protected snapshot and never
prints. Extend the existing OSC-777 screenshot command task with opt-in
`module-fan-log=start`, `module-fan-log=stop`, and `module-fan-log?` commands;
that task emits at most one line per new snapshot.

Reason: using the existing command task gives screenshots, command replies, and
fan telemetry a single device-side writer. Screenshot transfer is synchronous,
so telemetry pauses during the raw RGB565 frame and cannot split it. A monotonic
`uptime_ms` is used instead of RTC wall time because it remains useful even
when the board RTC is unset or incorrect.

Host-side consequence: only one program should open the serial port. The
dedicated telemetry tool combines command sending and reading and sends stop
when streaming is interrupted.

## 2026-07-11: Group Fan HAL Files And Separate Telemetry Formatting

Decision: keep shared `cpu_temperature.*` beside the existing HAL component
files, group the four fan-only device/service files under
`hal/components/module_fan/`, and place telemetry command state/formatting in
`app/debug/module_fan_telemetry.*`.

Reason: device/register access and the automatic task form one fan capability,
while CPU temperature remains shared with the official power-monitor GUI. Fan
line formatting belongs to the app debug layer, not to the hardware task or a
file whose only stated role is screen capture. The existing screenshot task
continues to own USB transport and OSC dispatch so there is still exactly one
device-side writer.

Superseded later on 2026-07-11: runtime configuration, persistence, validation,
and immediate task wakeup added enough policy-specific behavior to justify the
separate `module_fan_policy.*` class recorded above.

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

Status: superseded on 2026-07-18 by a dedicated copied background asset after
hardware screenshots exposed label bleed through the translucent overlay.

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
