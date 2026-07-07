# Tab5 Official Firmware Terminal Port

This repository has switched from the standalone PlatformIO UART-terminal
firmware to a porting workspace for adding a terminal popup to the M5Stack Tab5
official firmware.

The previous PlatformIO project is archived under:

`archive/platformio_tab5_terminal_20260707/`

The new terminal emulation target is `libvterm`. The current work does not
define the final GUI. The intended product behavior is only this: the official
firmware GUI will gain a button, and pressing that button will open a terminal
view.

Current status:

- `third_party/libvterm-src/` contains an MIT-licensed `libvterm` source
  snapshot imported from `neovim/libvterm`.
- `port/libvterm_adapter/` contains the first firmware-facing adapter skeleton.
- Official firmware build helper scripts are available for Windows and Ubuntu;
  see `docs/official_firmware_build.md`.
- No flash or hardware validation has been performed in this phase because the
  board is not available.

Start with `docs/current_work.md`, `docs/official_firmware_build.md`, and
`docs/libvterm_port_plan.md` before continuing implementation.
