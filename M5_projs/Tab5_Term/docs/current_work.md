# Current Work

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
