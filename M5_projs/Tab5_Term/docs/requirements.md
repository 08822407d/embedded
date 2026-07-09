# Requirements

## Product Goal

Add a terminal feature to the M5Stack Tab5 official firmware.

The user-facing behavior currently required is minimal:

- the official firmware GUI will add a button;
- clicking the button opens a terminal popup or terminal view;
- GUI appearance and interaction details are intentionally postponed.

## Terminal Core

The selected terminal emulation target is `libvterm`.

The previous PlatformIO project implemented a home-grown terminal core. That
project remains archived for reference, but new official-firmware work should
not continue expanding the old `terminal_core`.

## Current Constraints

- The physical Tab5 became available for the 2026-07-08 official-firmware
  baseline validation. Do not assume it remains available in future sessions;
  recheck USB device identity before flashing.
- Official-firmware build and flash are now allowed when the user asks for
  baseline validation or migration testing. Keep terminal-port changes separate
  from unmodified official-firmware baseline checks.
- Continue to focus on source layout, third-party import, adapter boundaries,
  and durable records before adding product GUI behavior.

## Persistence Requirement

Project context must remain recoverable after chat compaction or a machine
change. Record original requirements, design decisions, implementation details,
known issues, and validation state in repo-local documents.
