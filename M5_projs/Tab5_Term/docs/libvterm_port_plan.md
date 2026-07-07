# libvterm Port Plan

## Goal

Replace the previous in-repo terminal parser/screen model with `libvterm` for
the official Tab5 firmware port. The firmware should embed terminal emulation
as a GUI component rather than as a standalone PlatformIO app.

## Boundary

Keep these responsibilities separate:

- Transport: UART, future SSH/Telnet/PPP, or any other byte source/sink.
- Terminal core: `libvterm` parser, screen state, keyboard/mouse encoding.
- GUI adapter: official firmware popup/window, font metrics, repaint scheduling.
- Device services: battery, power, fan, temperature, and status UI.

The current adapter intentionally does not include GUI layout code. It only
exposes dirty rectangles, cells, cursor movement, terminal properties, and host
output bytes.

## Imported Source

`third_party/libvterm-src/` is a source snapshot from `neovim/libvterm`.
It carries the MIT license in `third_party/libvterm-src/LICENSE`.

Reason for this source:

- The official upstream HTTPS tarball failed in this Windows PowerShell session
  due a TLS protocol alert.
- The GitHub mirror could be cloned quickly.
- The imported tree states that it is a fork of the original libvterm and an
  exact upstream mirror exists on its `mirror` branch.

This should be revisited once the official firmware repository layout is known.

## Adapter Shape

`port/libvterm_adapter/include/tab5_terminal_libvterm_adapter.h` defines:

- `TerminalSession`: owns `VTerm` and `VTermScreen`.
- `TerminalHostSink`: receives bytes that must be sent back to the host and GUI
  repaint/property events.
- `TerminalCell`: normalized cell data for the official GUI renderer.

Data flow:

1. Transport receives bytes from the login channel.
2. Firmware calls `TerminalSession::feedHostBytes(...)`.
3. `libvterm` updates its screen model.
4. `VTermScreenCallbacks.damage` reports dirty rectangles to the GUI.
5. GUI asks `TerminalSession::getCell(row, col, &cell)` while painting.
6. GUI converts `TerminalCell` into the official firmware font/draw calls.

Input flow:

1. GUI/input driver maps a key to Unicode or a terminal key enum.
2. Firmware calls `sendUnicode()` or `sendKey()`.
3. `libvterm` writes encoded bytes through the output callback.
4. `TerminalHostSink::writeHostBytes()` sends those bytes to the transport.

## Open Decisions

- Whether to use `VTermScreen` directly, as in the current adapter, or use the
  lower-level `VTermState` callbacks for a custom screen buffer.
- Whether to provide a fixed allocator for ESP32-P4 after measuring heap use.
- Whether the official firmware popup owns scrollback or delegates it to
  `libvterm` screen callbacks.
- Final rows/columns and font metrics in the official GUI.
- How to persist terminal settings in the official firmware settings model.

## Validation Later

When the board and official firmware source are available:

- Compile `libvterm` as part of the official firmware.
- Run a host-side byte-stream replay against the adapter without hardware.
- Validate `clear`, shell prompt, colored `htop`, `less`, `vim`/`nano`, resize,
  alternate screen, cursor visibility, and keyboard input.
- Measure heap usage before and after opening the terminal popup.
- Verify that closing the popup does not leave UART/transport callbacks active
  in a way that corrupts other firmware UI.
