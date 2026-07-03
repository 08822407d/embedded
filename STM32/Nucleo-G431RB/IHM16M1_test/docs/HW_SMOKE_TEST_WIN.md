# HW Smoke Test WIN - NUCLEO-G431RB + X-NUCLEO-IHM16M1

Date: 2026-07-03
Project root: `C:\Users\cheyh\Desktop\embedded\STM32\Nucleo-G431RB\IHM16M1_test`

## Safety Scope

This Windows smoke test stayed inside the requested read-only/offline boundary:

- No flash erase, mass erase, option-byte write, firmware download, target reset-run, `run`, `go`, OpenOCD programming, Motor Profiler run, MC Workbench generation/download, motor PSU action, PWM exercise, or serial TX/RX was performed.
- Hardware access was limited to ST-Link/USB enumeration and SWD hot-plug read-only probes.
- The firmware artifact was generated and built on disk only; it was not programmed to the board.

## Pre-Read Inputs

Read before acting:

- `docs/CODEX_TASK_hw_comm_smoke_win.md`
- `docs/FACTS.md`
- `docs/OPERATIONS.md` section 0
- `docs/DECISIONS.md` #003
- `docs/SYNC_WORKFLOW.md`

## HITL Record

### HITL-1

Request:

```text
目的:确认目标板确实已接到这台 Windows，并且首次接入相关驱动/提示已由用户处理；继续仅做只读 SWD 探测 + 离线编译。
```

User reply:

```text
我确认硬件已经通过mini-usb线连接到了本PC上
```

Result: accepted as completion for the required Windows board-connection confirmation. Continued with read-only probes only.

## Tool / Environment Inventory

Commands and key results:

```powershell
Get-Command STM32_Programmer_CLI.exe,arm-none-eabi-gcc.exe
```

- `STM32_Programmer_CLI.exe`: `C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`
- `arm-none-eabi-gcc.exe`: `C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe`

```powershell
Get-ChildItem "C:\Program Files\STMicroelectronics","C:\Program Files (x86)\STMicroelectronics"
```

Key installed ST tools:

- STM32CubeCLT 1.18.0: `C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0`
- STM32CubeIDE 1.19.0: `C:\Program Files\STMicroelectronics\STM32CubeIDE_1.19.0`
- STM32CubeMX 6.17.0-RC5: `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeMX\STM32CubeMX.exe`
- Standalone STM32CubeProgrammer CLI 2.18.0: `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`
- CubeCLT STM32CubeProgrammer CLI 2.19.0: `C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`
- CubeIDE bundled STM32CubeProgrammer CLI 2.20.0 under `STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\...externaltools.cubeprogrammer...`
- X-CUBE-MCSDK 6.4.1: `C:\Program Files (x86)\STMicroelectronics\MC_SDK_6.4.1`
  - MC Workbench: `Utilities\PC_Software\STMCWB\STMCWB.exe`, file/product version 6.4.1
  - MotorPilot: `Utilities\PC_Software\STMotorPilot\MotorPilot.exe`

```powershell
arm-none-eabi-gcc.exe --version
make.exe --version
STM32_Programmer_CLI.exe --version
```

- GCC: `arm-none-eabi-gcc.exe (GNU Tools for STM32 13.3.rel1.20240926-1715) 13.3.1 20240614`
- Make: GNU Make `4.4.1_st_20231030-1220` from CubeIDE 1.19.0
- CubeProgrammer CLI used for probing: STM32CubeProgrammer `2.19.0`

Installed G4 firmware packages:

- `C:\Users\cheyh\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.1`
- `C:\Users\cheyh\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.2`

## Device / COM Enumeration

```powershell
& "C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -l
```

Key output:

- ST-Link SN: `002A00403234510E33353533`
- ST-Link FW: `V3J17M10`
- Board Name: `NUCLEO-G431RB`
- UART/VCP:
  - Board Name: `NUCLEO-G431RB`
  - ST-LINK SN: `002A00403234510E33353533`
  - Port: `COM4`
  - Description: `STMicroelectronics STLink Virtual COM Port`
  - Manufacturer: `STMicroelectronics`
- Other serial ports present: Bluetooth `COM9` and `COM10`; no Espressif COM port was seen in this run.

PnP cross-check:

- `USB\VID_0483&PID_374E\002A00403234510E33353533` was present and OK.
- `ST-Link Debug`, MBED storage, and `NOD_G431RB` entries were present and OK.

## SWD Read-Only Communication

Commands:

```powershell
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG sn=002A00403234510E33353533 -r32 0xE0042000 1
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG sn=002A00403234510E33353533 -r16 0x1FFF75E0 1
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG sn=002A00403234510E33353533 -ob displ
```

Key output:

- Board: `NUCLEO-G431RB`
- Voltage: `3.27V` / `3.28V`
- Connect mode: `Hot Plug`
- Device ID: `0x468`
- Revision ID: `Rev X`
- Device name: `STM32G43x/G44x`
- Flash size: `128 KBytes`
- DBGMCU IDCODE read: `0xE0042000 : 20036468` (low 12 bits = `0x468`)
- Flash-size data register: `0x1FFF75E0 : 0080` (128 KiB)
- Option bytes display succeeded; notable summary:
  - `RDP : 0xAA (Level 0, no protection)`
  - `BOR_LEV : 0x0`
  - User option bytes displayed without write

Result: PASS. Windows can communicate with the same board over ST-Link/SWD for read-only access.

## CubeMX / Toolchain Smoke

Tracked inputs:

- Seed `.ioc`: `hw_smoke_win/g431_hw_smoke_win.ioc`
- CubeMX script: `hw_smoke_win/cubemx_generate_win.txt`
- Build/post-generation script: `hw_smoke_win/build_win.ps1`
- Generated project: `hw_smoke_win/g431_hw_smoke_win/` (ignored)
- Firmware package: `C:\Users\cheyh\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.2`

Generation command pattern:

```powershell
& "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeMX\STM32CubeMX.exe" -q "...\hw_smoke_win\cubemx_generate_win.txt"
```

On this Windows install the launcher returned immediately while the underlying `javaw.exe` continued generating. To capture synchronous output, the same CubeMX entry point was also run through `java.exe`; it reported `OK` for `config load`, firmware path, project name/path, Makefile toolchain, and `project generate`.

Windows CubeMX 6.17.0-RC5 generated two issues in the Makefile project:

- It listed `Src/sysmem.c` and `Src/syscalls.c` but failed to copy them because it concatenated the project path with an absolute path.
- It duplicated generated source entries and left one missing line-continuation backslash in the `C_SOURCES` block.

`hw_smoke_win/build_win.ps1` makes the build reproducible by:

- Copying `sysmem.c` and `syscalls.c` from `C:\Users\cheyh\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.2\Projects\NUCLEO-G431RB\Templates\STM32CubeIDE\Example\User`.
- Normalizing the generated Makefile `C_SOURCES` block to one source entry per file.
- Temporarily prepending CubeCLT GCC to `PATH`.
- Invoking CubeIDE GNU Make.

Full reproducible generation/build command verified:

```powershell
& "hw_smoke_win\build_win.ps1"
```

Build result: PASS.

Size summary:

```text
text  data  bss   dec   hex   filename
4400  12    1572  5984  1760  build/g431_hw_smoke_win.elf
```

Artifacts left on disk:

- `hw_smoke_win/g431_hw_smoke_win/build/g431_hw_smoke_win.elf` (87,352 bytes)
- `hw_smoke_win/g431_hw_smoke_win/build/g431_hw_smoke_win.bin` (4,412 bytes)
- `hw_smoke_win/g431_hw_smoke_win/build/g431_hw_smoke_win.hex` (12,489 bytes)

These generated files are ignored by `.gitignore`.

## Pass / Fail Matrix

| Layer | Target | Result | Evidence |
|---|---|---:|---|
| HITL | Manual board connection confirmation | PASS | User confirmed mini-USB connection to this Windows PC |
| Probe | ST-Link online, SN match | PASS | `STM32_Programmer_CLI -l` reported SN `002A00403234510E33353533`, board `NUCLEO-G431RB` |
| Probe | Windows COM by fixed SN | PASS | `STM32_Programmer_CLI -l` mapped target VCP to `COM4`; PnP showed VID:PID `0483:374E` with target SN |
| SWD read | Device ID `0x468` | PASS | Hot-plug SWD connect reported `Device ID : 0x468`; IDCODE read `20036468` |
| SWD read | Flash size readable | PASS | Connect reported `128 KBytes`; flash-size register `0x0080` |
| SWD read | Option bytes readable | PASS | `-ob displ` reported RDP/BOR/user config |
| Tool inventory | ST Windows tools found | PASS | CubeCLT, CubeIDE, CubeMX, CubeProgrammer, MCSDK paths and versions recorded |
| Toolchain | CubeMX headless generation | PASS with postgen caveat | Generation completed; Windows path defects are handled by `build_win.ps1` |
| Toolchain | Makefile build with Windows tools | PASS | CubeIDE Make + CubeCLT GCC produced `.elf/.bin/.hex` |

## Notes / Caveats

- `COM4` is an observed value for this run only. Future sessions must re-enumerate by ST-Link SN, not reuse the COM number blindly.
- MCSDK was only inventoried by installed files and versions. Motor Profiler, MotorPilot, and MC Workbench were not launched for hardware-driving work.
- CubeMX may check updater/network state even in command-line mode; no installation, update, login, or GUI click was performed.
