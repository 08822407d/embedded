# Decision Log

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
