# Debug Framebuffer Capture

The debug framebuffer capture records the exact logical pixels that M5GFX
presents for the current Tab5 display orientation. It is intended to replace a
camera photograph when the question is about digital rendering rather than the
physical LCD panel.

## What It Can Verify

- glyph rasterization and fallback glyphs;
- foreground and background colors;
- status-bar and terminal-region placement;
- clipping, overlap, stale pixels, and cursor rendering;
- deterministic pixel equality between two stable screens.

It does not verify backlight brightness, panel color cast, viewing angle,
ghosting, bad pixels, touch alignment, enclosure mounting, or whether the
physical display is comfortable to read. Those checks still require the real
device and, when necessary, a photograph or direct user observation.

## Firmware Environments

| Environment | Input path | Capture support |
| --- | --- | --- |
| `tab5_terminal_regression` | USB CDC terminal injection | Enabled |
| `tab5_screen_capture` | Normal 921600 login-UART bridge | Enabled |
| `tab5_min_uart_terminal` | Normal 921600 login-UART bridge | Disabled |

`ENABLE_SCREEN_CAPTURE_DIAGNOSTICS` defaults to `0`. The ordinary formal
firmware therefore has no active capture command. The capture environments
read the existing MIPI DSI RGB565 framebuffer one row at a time and allocate
only temporary row buffers; they do not keep a second full-screen framebuffer.

## Capture Command

Build and flash the environment that matches the required test:

```powershell
.\tools\tab5.ps1 build tab5_terminal_regression
.\tools\tab5.ps1 flash tab5_terminal_regression -Port COM3
```

or:

```powershell
.\tools\tab5.ps1 build tab5_screen_capture
.\tools\tab5.ps1 flash tab5_screen_capture -Port COM3
```

After the screen reaches a stable state:

```powershell
.\tools\tab5.ps1 screenshot -Port COM3 `
  -OutputPath .logs\screenshots\current.png
```

The command writes:

- `current.png`: lossless RGB image for inspection;
- `current.rgb565`: exact little-endian RGB565 framebuffer bytes;
- `current.json`: dimensions, rotation, CRC32, SHA-256, and output paths.

The default output directory is `.logs/screenshots/`. These generated files
are local test artifacts and are not checked into git.

## Pixel Comparison

Use a prior `.rgb565` file as the baseline:

```powershell
.\tools\tab5.ps1 screenshot -Port COM3 `
  -OutputPath .logs\screenshots\current.png `
  -Baseline .logs\screenshots\accepted.rgb565 `
  -ChannelTolerance 0
```

The metadata then includes changed-pixel count, percentage, maximum and mean
channel delta, and the bounding box of all changed pixels. A `.diff.png` image
marks the changed region. Use tolerance `0` for deterministic firmware output;
raise it only when a documented conversion or rendering path justifies it.

## Wire Protocol

The host sends the private USB CDC frame:

```text
ESC ] 777 ; screen-capture? BEL
```

The device responds with an ASCII header, exactly `width * height * 2` raw
bytes, and an ASCII trailer:

```text
TAB5SHOT BEGIN v=1 width=1280 height=720 format=rgb565le bytes=1843200 rotation=3
<raw RGB565 bytes>
TAB5SHOT END crc32=XXXXXXXX
```

`tools/capture_screen.py` validates the byte count and CRC before writing any
accepted result. It configures DTR and RTS before opening the USB serial port;
opening first can reset the ESP32-P4 and accidentally capture a startup screen.

## Operating Constraints

Streaming a 1280x720 RGB565 frame transfers 1,843,200 bytes and temporarily
blocks the main firmware loop. Capture only a stable screen and avoid producing
login-UART output during the transfer. This is test instrumentation, not a
runtime remote-desktop feature.

After testing, restore the formal firmware:

```powershell
.\tools\tab5.ps1 build tab5_min_uart_terminal
.\tools\tab5.ps1 flash tab5_min_uart_terminal -Port COM3
.\tools\tab5.ps1 probe -Port COM3
```

## Validation Record

On 2026-06-12:

- `tab5_terminal_regression` captured a 1280x720 Stage 7 screen with CRC32
  `329912D5`;
- an immediate repeated capture compared against the first with zero changed
  pixels;
- `tab5_screen_capture` captured the real 921600 login-shell screen with CRC32
  `972C7498`;
- the board was restored to `tab5_min_uart_terminal`, and the shell probe
  returned `shell-path-ok: m5stack-LLM`.
