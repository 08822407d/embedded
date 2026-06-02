# Tab5 Minimal UART Viewer

This PlatformIO project is a minimal UART viewer for M5Stack Tab5 / ESP32-P4.
It displays printable bytes received from an external Linux board login UART on
the Tab5 screen. It is not a full terminal emulator.

The first version intentionally does not implement ANSI / VT100 escape
sequences, keyboard input, reverse transmit, colors from the remote side, cursor
movement, or a complex UI.

## Hardware

Default wiring:

- External Linux board TX -> Tab5 M5-Bus G38 / RXD0
- External Linux board GND <-> Tab5 GND
- Tab5 TX is not required for the first version

If bidirectional input is needed later:

- Tab5 M5-Bus G37 / TXD0 -> external device RX

Default UART settings:

- 115200 8N1
- 3.3 V TTL UART only

Do not connect RS-232 level signals or 5 V TTL signals directly to Tab5 GPIO.

USB serial is used only for flashing and debug logs. It is not mixed with the
external login UART.

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

## Troubleshooting

If the screen shows nothing, first confirm that M5Unified and M5GFX were pulled
from their latest GitHub versions as configured in `platformio.ini`.

If the external board output does not appear on screen, verify:

- External TX is connected to Tab5 G38.
- Grounds are connected.
- The external UART is 3.3 V TTL, not RS-232 or 5 V TTL.
- The external UART is configured for 115200 8N1.
