# HW Smoke Test — NUCLEO-G431RB + X-NUCLEO-IHM16M1

Date: 2026-06-30
Project root: `/home/cheyh/projs/embedded/STM32/Nucleo-G431RB/IHM16M1_test`

## Safety Scope

This smoke test stayed inside the requested read-only/offline boundary:

- No flash erase, mass erase, option-byte write, firmware download, upload target, reset-run, `-run`, `-go`, OpenOCD programming, or motor/PWM exercise was performed.
- ST-Link/SWD use was read-only: probe listing, hot-plug connection, memory reads, and option-byte display only.
- Serial/VCP was enumerated only; it was not opened for TX/RX.
- The firmware artifact was generated and built on disk only; it was not programmed to the board.

## Pre-Read Inputs

Read before acting:

- `docs/CODEX_TASK_hw_comm_smoke.md`
- `docs/FACTS.md`
- `docs/OPERATIONS.md` section 0
- `docs/DECISIONS.md` #003

## Permission / Tool Probes

Commands and key results:

```sh
STM32_Programmer_CLI -l
```

- Tool resolved to `/opt/st/stm32cubeclt_1.18.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI`.
- Actual banner: STM32CubeProgrammer v2.19.0.
- ST-Link probe found:
  - Board: `NUCLEO-G431RB`
  - ST-Link SN: `002A00403234510E33353533`
  - ST-Link FW: `V3J16M9`
- UART list also showed:
  - Espressif USB JTAG serial debug unit at `/dev/ttyACM0`
  - STLINK-V3 for this board at `/dev/ttyACM1`

```sh
ls -l /dev/serial/by-id/
```

- Target VCP link: `usb-STMicroelectronics_STLINK-V3_002A00403234510E33353533-if02 -> ../../ttyACM1`
- Non-target Espressif link: `usb-Espressif_USB_JTAG_serial_debug_unit_F0:9E:9E:32:91:C8-if00 -> ../../ttyACM0`

```sh
/opt/st/stm32cubeclt_1.18.0/GNU-tools-for-STM32/bin/arm-none-eabi-gcc --version
```

- `arm-none-eabi-gcc (GNU Tools for STM32 13.3.rel1.20240926-1715) 13.3.1 20240614`

```sh
make --version
```

- `/usr/bin/make`, GNU Make 4.3
- Note: no executable `make` was found under `/opt/st/stm32cubeclt_1.18.0`; build therefore uses system `make` with CubeCLT `GCC_PATH`.

```sh
ls -l /home/cheyh/STM/STM32CubeMX/STM32CubeMX
```

- CubeMX executable exists and is executable.

Local docs used for CubeMX CLI syntax:

- `/home/cheyh/STM/STM32CubeMX/help/UM1718.pdf`
- Confirmed `STM32CubeMX -q <script>`, `config load`, `project toolchain Makefile`, `project path`, and `project generate`.

## Device / Communication Smoke

Read-only SWD command:

```sh
/opt/st/stm32cubeclt_1.18.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI \
  -c port=SWD mode=HOTPLUG sn=002A00403234510E33353533 \
  -r32 0xE0042000 4 \
  -r16 0x1FFF75E0 2 \
  -ob displ
```

Key output:

- ST-Link SN: `002A00403234510E33353533`
- Board: `NUCLEO-G431RB`
- Voltage: `3.27V`
- Connect mode: `Hot Plug`
- Device ID: `0x468`
- Device name: `STM32G43x/G44x`
- Flash size: `128 KBytes`
- DBGMCU IDCODE read: `0xE0042000 : 20036468` (low 12 bits = `0x468`)
- Flash size data register read: `0x1FFF75E0 : 0080` (128 KiB)
- Option bytes display succeeded; notable summary:
  - `RDP : 0xAA (Level 0, no protection)`
  - `BOR_LEV : 0x0`
  - User option bytes displayed without write

Result: PASS. ST-Link to MCU SWD communication is working for read-only access.

## CubeMX / Toolchain Smoke

Generated project inputs:

- Seed `.ioc`: `hw_smoke/g431_hw_smoke.ioc`
- CubeMX script: `hw_smoke/cubemx_generate.txt`
- Generated project: `hw_smoke/g431_hw_smoke/`
- Firmware package path requested in script: `/home/cheyh/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.1`

Generation command:

```sh
/home/cheyh/STM/STM32CubeMX/STM32CubeMX -q \
  /home/cheyh/projs/embedded/STM32/Nucleo-G431RB/IHM16M1_test/hw_smoke/cubemx_generate.txt
```

Generation result: PASS.

Generated GPIO configuration:

- `LED2_Pin = GPIO_PIN_5`
- `LED2_GPIO_Port = GPIOA`
- `GPIO_MODE_OUTPUT_PP`, `GPIO_NOPULL`, `GPIO_SPEED_FREQ_LOW`
- `HAL_TIM_MODULE_ENABLED` and `HAL_USART_MODULE_ENABLED` remain commented out in `stm32g4xx_hal_conf.h`; only the GPIO-only minimum was generated.

Build command:

```sh
make -C hw_smoke/g431_hw_smoke \
  GCC_PATH=/opt/st/stm32cubeclt_1.18.0/GNU-tools-for-STM32/bin
```

Build result: PASS.

Size summary:

```text
text  data  bss   dec   hex   filename
4372  12    1572  5956  1744  build/g431_hw_smoke.elf
```

Artifacts left on disk:

- `hw_smoke/g431_hw_smoke/build/g431_hw_smoke.elf` (86,100 bytes)
- `hw_smoke/g431_hw_smoke/build/g431_hw_smoke.bin` (4,384 bytes)
- `hw_smoke/g431_hw_smoke/build/g431_hw_smoke.hex` (12,420 bytes)

Build products are ignored by `.gitignore` via `hw_smoke/**/build/`.

## Pass / Fail Matrix

| Layer | Target | Result | Evidence |
|---|---|---:|---|
| Probe | ST-Link online, SN match | PASS | `STM32_Programmer_CLI -l` reported `NUCLEO-G431RB`, SN `002A00403234510E33353533` |
| Probe | Device ID `0x468` | PASS | SWD hot-plug connect reported `Device ID : 0x468`; IDCODE read low 12 bits = `0x468` |
| SWD read | Flash size readable | PASS | Connect reported `128 KBytes`; `0x1FFF75E0 : 0080` |
| SWD read | Option bytes readable | PASS | `-ob displ` reported RDP/BOR/user config |
| Serial enum | VCP by fixed SN | PASS | by-id STLINK-V3 link points to `../../ttyACM1`; Espressif is separate at `ttyACM0` |
| Toolchain | CubeMX headless generation | PASS | `STM32CubeMX -q hw_smoke/cubemx_generate.txt` generated `hw_smoke/g431_hw_smoke/` |
| Toolchain | Makefile build with CubeCLT GCC | PASS | `make ... GCC_PATH=...` produced `.elf/.bin/.hex` |

## Notes / Caveats

- CubeProgrammer actual banner was v2.19.0, although earlier facts expected v2.20.0. The executable is still the one under CubeCLT 1.18.0.
- CubeMX automatically performed updater/network checks while starting, even in `-q` mode. No ST web documentation lookup or `curl` probe was needed; local UM1718 was sufficient.
- CubeMX generated `.ioc` metadata includes `FirmwarePackage=STM32Cube FW_G4 V1.6.3`, while `CustomerFirmwarePackage` points to the installed local package `STM32Cube_FW_G4_V1.6.1`; only `STM32Cube_FW_G4_V1.6.1` exists under the local repository.
- No USART firmware was generated; USART2/VCP was optional in the task and serial was validated only at enumeration level.
