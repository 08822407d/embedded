# Tab5 Minimal UART Viewer

This PlatformIO project is a minimal UART viewer for M5Stack Tab5 / ESP32-P4.
It displays printable bytes received from an external Linux board login UART on
the Tab5 screen. It is not a full terminal emulator.

The first version intentionally does not implement ANSI / VT100 escape
sequences, keyboard input, reverse transmit, colors from the remote side, cursor
movement, or a complex UI.

## Hardware

Default wiring:

- For M5Stack Module LLM `ttyS1` / M5-Bus `TRM_TXD/TRM_RXD`, the tested
  Tab5 pins are `RX=G7` and `TX=G6` (M-Bus pin 15/16).
- External Linux board TX -> Tab5 RX pin.
- External Linux board GND <-> Tab5 GND
- Tab5 TX is only required for bidirectional shell/login testing.

If bidirectional input is needed later:

- Tab5 TX pin -> external device RX

The earlier generic G38/G37 assumption did not match the Module LLM `ttyS1`
M5-Bus login path on the tested Tab5 + Module LLM stack.

Default UART settings:

- 115200 8N1
- 3.3 V TTL UART only

Do not connect RS-232 level signals or 5 V TTL signals directly to Tab5 GPIO.

USB serial is used only for flashing and debug logs. It is not mixed with the
external login UART.

## Current Verified Status

- PlatformIO env: `tab5_min_uart_terminal`
- Current login UART pins: `RX=G7`, `TX=G6`
- Current login UART baud: `115200 8N1`
- USB debug/flash port on the first development machine was `/dev/ttyACM0`
  (`VID:PID 303A:1001`, Espressif USB JTAG/serial). Treat this as a local
  observation, not a fixed upload port.
- The USB monitor bridge is enabled. Typing in `pio device monitor` is forwarded
  to the login UART, and bytes received from the login UART are echoed back to
  the monitor and rendered on the Tab5 display.
- The Module LLM login shell was verified by sending `id` through the Tab5
  bridge and receiving a root shell response.

## Build

```sh
pio run -e tab5_min_uart_terminal
```

## List Ports

```sh
pio device list
python -m serial.tools.list_ports -v
```

## Upload

Do not permanently hard-code `upload_port` in `platformio.ini`. Pick the Tab5
USB debug port from the port list and upload with:

```sh
pio run -e tab5_min_uart_terminal -t upload --upload-port <PORT>
```

If upload fails, put the Tab5 into download mode and retry. Do not assume upload
succeeded unless PlatformIO reports success.

## Monitor

```sh
pio device monitor --port <PORT> --baud 115200
```

The monitor shows USB debug logs and a copy of received UART bytes/text so you
can confirm that external UART data is arriving.

With the current bridge build, the monitor can also be used as a temporary
keyboard path:

```sh
pio device monitor --port <PORT> --baud 115200
```

After opening the monitor, type a shell command such as `id` and press Enter.
The command is forwarded from USB debug CDC to the Module LLM login UART.

## Continue On Another Machine

See [HANDOFF.md](HANDOFF.md) for the handoff checklist, known local toolchain
notes, tested hardware wiring, and next planned work on USB-A keyboard input.

When committing from this checkout, note that this project lives inside a larger
git working tree. Scope commits to this directory unless intentionally syncing
the sibling projects too, for example:

```sh
git status -- M5_projs/Tab5_Term
git add M5_projs/Tab5_Term
git commit -m "Add Tab5 UART terminal handoff notes"
git push origin master
```

## Troubleshooting

If the screen shows nothing, first confirm that M5Unified and M5GFX were pulled
from their latest GitHub versions as configured in `platformio.ini`.

If the external board output does not appear on screen, verify:

- External TX is connected to Tab5 `G7` for the tested Module LLM `ttyS1`
  M5-Bus login path.
- Grounds are connected.
- The external UART is 3.3 V TTL, not RS-232 or 5 V TTL.
- The external UART is configured for 115200 8N1.
