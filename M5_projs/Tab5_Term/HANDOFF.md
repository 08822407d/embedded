# Tab5 UART Terminal Handoff

This document captures the current working state so the project can be pushed to
GitHub and resumed on another development machine.

## Repository Scope

This project directory is:

```text
M5_projs/Tab5_Term
```

The git repository root on the first development machine was:

```text
/home/cheyh/projs/embedded
```

There were unrelated dirty files in sibling directories. When committing or
pushing this work, scope the commit to `M5_projs/Tab5_Term` unless those sibling
changes are intentionally part of the same sync.

## Current Firmware State

- PlatformIO environment: `tab5_min_uart_terminal`
- Platform: `pioarduino/platform-espressif32` at tag `54.03.21`
- Framework: Arduino
- Board setting: `esp32-p4-evboard`
- Display stack: M5Unified / M5GFX
- Login UART: `HardwareSerial LoginSerial(1)`
- Login UART pins currently configured in `include/app_config.h`:
  - RX: `G7`
  - TX: `G6`
  - Baud: `115200`
  - Format: `8N1`
- USB debug CDC remains separate from the external login UART.
- USB-to-login-UART bridge is currently enabled:
  - `ENABLE_USB_LOGIN_UART_BRIDGE=1`
  - `ENABLE_USB_BRIDGE_DEBUG_LOG=0`

## Verified Hardware Path

The original generic assumption `G38/G37` did not match the tested Module LLM
M5-Bus login path. Cross-checking with Module LLM context and M5Unified pin
tables showed that Module LLM `ttyS1` / M5-Bus `TRM_TXD/TRM_RXD` maps to Tab5
M-Bus pin 15/16:

```text
Module LLM TX -> Tab5 G7  (Tab5 RX)
Module LLM RX -> Tab5 G6  (Tab5 TX, needed for bidirectional shell)
GND           -> GND
```

The Tab5 firmware was verified through the USB monitor bridge. Sending:

```sh
id
```

through the Tab5 USB monitor returned a root shell response from the Module LLM
login UART.

## New Machine Bootstrap

From a fresh checkout, run:

```sh
pio --version
pio device list
python -m serial.tools.list_ports -v
pio run -e tab5_min_uart_terminal
```

Do not permanently set `upload_port` in `platformio.ini`. Identify the Tab5 USB
debug/programming port on the new machine. On the first machine it appeared as:

```text
/dev/ttyACM0
VID:PID 303A:1001
Espressif USB JTAG/serial
```

That port name is machine-local and may differ after replugging or on another
host.

Upload and monitor:

```sh
pio run -e tab5_min_uart_terminal -t upload --upload-port <PORT>
pio device monitor --port <PORT> --baud 115200
```

If upload fails, put Tab5 into download mode and retry. Do not assume the board
was flashed unless PlatformIO reports a successful upload.

## Local Toolchain Notes

The first development machine needed local PlatformIO package repair before the
ESP32-P4 Arduino build succeeded:

- The RISC-V toolchain package was empty/broken and was repaired by installing
  `riscv32-esp-elf` through Espressif IDF tools and linking it into
  `~/.platformio/packages/toolchain-riscv32-esp`.
- Missing Python packages were installed locally for PlatformIO tooling:
  `rich-click`, `PyYAML`, `intelhex`, `rich`, `esp-idf-size`.

A new machine may not need this if PlatformIO downloads clean packages. If the
same build failures recur, check the PlatformIO package install before changing
project code or switching frameworks.

## Expected Runtime Behavior

After boot:

- The Tab5 screen shows the status line with UART pins and baud rate.
- Bytes from `LoginSerial` are rendered on the display.
- Printable ASCII, CR, LF, and tabs receive minimal handling.
- ANSI / VT100 escape sequences are intentionally ignored.
- Bytes from `LoginSerial` are echoed to USB debug Serial.
- Bytes typed into USB monitor are forwarded to `LoginSerial` while the bridge
  flag remains enabled.

## Next Work: USB-A Keyboard

Goal: make Tab5 standalone by reading a USB keyboard from the Tab5 USB-A host
port and forwarding key input to `LoginSerial`.

Research summary:

- Tab5 has a USB-A host port.
- M5Unified exposes external output power control; `M5.begin()` defaults
  `cfg.output_power=true`, and Tab5 external USB power can also be explicitly
  enabled with `M5.Power.setExtOutput(true, m5::ext_USB)`.
- Arduino-ESP32 `USBHIDKeyboard` is device-side and is not suitable for reading
  an external keyboard.
- ESP-IDF USB host APIs are present in the local ESP32-P4 Arduino package
  (`usb/usb_host.h`, `libusb.a`, `libesp_hid.a`).
- ESP-IDF's HID host example uses the `usb_host_hid` component and implements
  boot-protocol keyboard report parsing.

Recommended next step:

1. Add a temporary `tab5_usb_keyboard_probe` env or compile-time feature flag.
2. Enable USB-A VBUS explicitly after `M5.begin()`.
3. Bring up USB host event handling in a FreeRTOS task.
4. Receive HID keyboard boot reports.
5. Convert basic US keyboard scancodes to ASCII.
6. Queue resulting bytes to the main loop and write them to `LoginSerial`.
7. Keep USB CDC/JTAG only for flashing and logs.

Do this as a probe first, before merging into the current terminal loop.

## Quick Validation Checklist

On the new machine:

```sh
pio run -e tab5_min_uart_terminal
pio device list
python -m serial.tools.list_ports -v
pio run -e tab5_min_uart_terminal -t upload --upload-port <PORT>
pio device monitor --port <PORT> --baud 115200
```

Then type:

```sh
id
```

Expected result: the Module LLM shell responds, and the response is visible in
both the USB monitor and on the Tab5 display.
