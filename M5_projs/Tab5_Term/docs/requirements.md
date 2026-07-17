# Requirements

## Product Goal

Add a terminal feature to the M5Stack Tab5 official firmware.

The user-facing behavior currently required is minimal:

- the official firmware GUI will add a terminal entry button;
- the first entry point is in the launcher power panel, replacing the former
  shake-wakeup and 10-second sleep entries with one terminal-style icon button;
- clicking the final button will open a terminal popup or terminal view;
- as of 2026-07-09, the button is allowed to be a placeholder that only emits a
  click tone/log while terminal view work remains unimplemented;
- official-firmware GUI work should keep the automated screenshot debug path
  available so display/layout regressions can be inspected from the host.

## Terminal Core

The selected terminal emulation target is `libvterm`.

The previous PlatformIO project implemented a home-grown terminal core. That
project remains archived for reference, but new official-firmware work should
not continue expanding the old `terminal_core`.

## Module Fan Automatic Control

- Support M5Stack Module Fan v1.1 with native ESP-IDF APIs; do not add an
  Arduino or `Wire` compatibility layer to the official firmware.
- Reuse the Tab5 BSP system-I2C/M5-Bus handle. Do not initialize, reset, or
  deinitialize the shared bus from the fan feature.
- The current scope is automatic control only. Manual duty commands and GUI
  settings remain deferred.
- Sample the ESP32-P4 internal temperature approximately every 5 seconds and
  use a configurable stepped policy with hysteresis. The accepted policy is
  `40/50/60/65 C` on thresholds, 5 C lower off thresholds, and
  `29/49/69/98%` duty.
- Use a conservative 50% target when the control temperature cannot be read.
- Detect the default module at I2C address `0x18` by more than ACK alone, release
  the module-specific device handle after repeated communication loss, and
  continue low-rate probing for hot-plug recovery.
- Do not override the official firmware's EXT 5V GUI state. Automatic control
  resumes after power and module communication return.
- Avoid unnecessary I2C traffic and module flash wear: poll at the control
  interval and write duty only when the requested value changes.

## Fan Debug Telemetry

- Periodic telemetry must be disabled by default and controlled with explicit
  start, stop, and status commands.
- Samples must include a monotonic timestamp, temperature, validity, attachment
  and communication state, target/applied duty, and RPM.
- The automatic-control task must not print directly. Command responses,
  telemetry, and raw screenshot transfer must use one serialized device-side
  writer so a sample cannot corrupt a screenshot frame.
- Host tools that read commands and telemetry must use one owner for the USB
  serial port. Concurrent monitor, telemetry, and screenshot processes are not
  a supported access pattern.

## Fan Runtime Policy Configuration

- Automatic policy values must be changeable at runtime without rebuilding or
  reflashing the firmware. Manual fan mode remains a separate future feature.
- A runtime change applies immediately in RAM. It must not write Flash unless
  the user sends an explicit save command.
- Persisted settings use a versioned NVS record. Missing, incompatible, or
  semantically invalid records fall back to compiled defaults.
- Whole-curve replacement is atomic: parse and validate all four levels before
  changing the active policy. Accepted values require strictly increasing
  enter and exit temperatures, exit no higher than its matching enter value,
  nondecreasing 1-100% level duties, 0-100 C thresholds, a 1-60 second control
  interval, and a 0-100% sensor-failure duty.
- The debug protocol must provide policy query, whole-curve set, interval set,
  failsafe set, explicit save, NVS reload, and compiled-default restore actions.
- Policy commands and replies share the existing serialized OSC-777 debug
  transport with telemetry and screenshots.

## Current Constraints

- The physical Tab5 was available for the 2026-07-11 Module Fan validation.
  Recheck USB device identity before future flashes; do not assume the same
  port number or continued physical availability in later sessions.
- Official-firmware build and flash are now allowed when the user asks for
  baseline validation or migration testing. Keep terminal-port changes separate
  from unmodified official-firmware baseline checks.
- Continue to focus on source layout, third-party import, adapter boundaries,
  and durable records before adding product GUI behavior.
- The launcher terminal-entry visual overlay was flash- and screenshot-verified
  on a physical Tab5 on 2026-07-09. Touch behavior is still pending direct
  tap-test validation.

## Persistence Requirement

Project context must remain recoverable after chat compaction or a machine
change. Record original requirements, design decisions, implementation details,
known issues, and validation state in repo-local documents.
