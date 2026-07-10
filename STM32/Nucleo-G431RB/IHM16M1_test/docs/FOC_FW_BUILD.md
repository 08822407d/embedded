# FOC_FW_BUILD - GM16020-06 Formal Hall FOC Firmware

Date: 2026-07-10

Project root: `C:\Users\cheyh\Desktop\embedded\STM32\Nucleo-G431RB\IHM16M1_test`

## Scope and Safety Result

This checkpoint covers an **offline Debug build and generated-configuration audit only**. No board enumeration, STM32CubeProgrammer/ST-Link command, flash, erase, reset, MotorPilot, MC Workbench motor-driving function, motor bus power, PWM run, or motor motion was performed.

The MC Workbench/CubeMX-generated tree `GM16020-06_foc/` was not hand-edited. It is ignored by `/*_foc/`; the reproducible input `GM16020-06_foc.stwb6` remains outside `.gitignore`. Therefore the manual-change register in [`GITIGNORE_AND_REGEN.md`](GITIGNORE_AND_REGEN.md) section 4 remains empty.

## Inputs Verified

- Seed: `GM16020-06_foc.stwb6`, 76,983 B, SHA-256 `EBDB87B9173C32ADF50B3FB7DE1A7EA3BB762077324A5E3A9D05C68F358459A0`.
- Generated Eclipse project: `GM16020-06_foc\STM32CubeIDE\`; project name `GM16020-06_foc`.
- Build configuration: `GM16020-06_foc/Debug`.
- Linker script: `GM16020-06_foc\STM32CubeIDE\STM32G431RBTX_FLASH.ld`.
- Generator stack recorded by the project: MCSDK/MC Workbench 6.4.2; the generated `.ioc` records `MotorControl.CUBE_MX_VER=6.14.1`. This project-embedded value must not be confused with the separately installed standalone CubeMX 6.18.0.
- Build tool: STM32CubeIDE 1.19.0 console launcher in Eclipse headless-build mode; object conversion/inspection used CubeCLT 1.18.0 GNU tools.

## Headless Build

The clean build was run without opening the CubeIDE GUI:

```powershell
$ide = "C:\Program Files\STMicroelectronics\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeidec.exe"
$project = (Resolve-Path "GM16020-06_foc\STM32CubeIDE").Path
$workspace = Join-Path $env:TEMP ("codex-stm32cubeide-foc-build-" + (Get-Date -Format "yyyyMMdd-HHmmss"))

& $ide --launcher.suppressErrors -nosplash `
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
  -data $workspace -no-indexer -import $project `
  -cleanBuild "GM16020-06_foc/Debug" -verbose -printErrorMarkers
```

Result: `Headless build completed successfully`; `Build Finished. 0 errors, 0 warnings.` The first build succeeded without the transient CMSIS-DSP include workaround required by the older Motor Profiler project. The generated `.cproject` and all generated sources remained unchanged. A post-build process check found no `stm32cubeide`, `stm32cubeidec`, `java`, or `javaw` process.

## Artifacts Left on Disk

The ignored local output directory is `GM16020-06_foc\STM32CubeIDE\Debug\`:

| File | Size | SHA-256 |
|---|---:|---|
| `GM16020-06_foc.elf` | 2,076,396 B | `A99C8DA760CE564D0FB517D66E81985AC6411AA23EA942E0367AE29AC090CC56` |
| `GM16020-06_foc.hex` | 105,456 B | `98AE4B37DD9DE313768FBE8EC150E1A2A316446AE70936A0F3DED11BD7044C52` |
| `GM16020-06_foc.bin` | 37,460 B | `7E15AABE9DCCF5C26E17BA3B6BB3A406E14489B7F9D4CB618117C403586BCC7E` |
| `GM16020-06_foc.map` | 906,238 B | not recorded |
| `GM16020-06_foc.list` | 1,004,348 B | not recorded |

CubeIDE produced ELF/MAP/LIST. HEX and BIN were exported offline:

```powershell
arm-none-eabi-objcopy -O ihex   GM16020-06_foc.elf GM16020-06_foc.hex
arm-none-eabi-objcopy -O binary GM16020-06_foc.elf GM16020-06_foc.bin
```

`arm-none-eabi-size` reports:

```text
text=36484  data=976  bss=7152  dec=44612  hex=ae44
```

`arm-none-eabi-readelf -h` confirms an ELF32 little-endian ARM executable, ARM EABI5 hard-float, entry point `0x08000545`.

## Effective Generated Configuration Audit

The values below were checked in the generated `.ioc` and C headers/sources, not inferred only from the Workbench UI:

| Area | Effective generated value |
|---|---|
| Motor | `GM16020-06`; 4 pole pairs; Rs `1.41 ohm`; Ls `0.00019 H`; nominal current `0.5 A`; max speed `19806 rpm`; nominal bus value rounded to `13 V` |
| Hall feedback | Main sensor = Hall; auxiliary sensor = none; 120-degree placement; electrical phase shift `58 deg`; TIM3 on PC6/PC7/PC8 |
| PWM/output | TIM1, `30000 Hz`; high-side U/V/W = PA8/PA9/PA10; GPIO enables = PB13/PB14/PB15; driver-protection input = PA11/TIM1_BKIN2 |
| Current sensing | Three shunts, external amplifiers, 2 ADCs; U = PA1/ADC1_IN2, V shared = PB11/ADC1_IN14 + ADC2_IN14, W = PA7/ADC2_IN4; shunt `0.33 ohm`; gain `1.53`; `Tnoise=500 ns`; `Trise=1000 ns` |
| Analog protection | Bus voltage = PA0/ADC1_IN1; temperature = PC4/ADC2_IN5; over-voltage `15 V`; under-voltage `7 V`; over-voltage action = turn off PWM |
| Default control | Speed mode, target `500 rpm`; auto-calculated current/speed PI; open loop, over-modulation, discontinuous PWM, flux weakening, potentiometer, start/stop button, ESC, and Motor Profiler all disabled |
| Communications | MCP over USART2 enabled, PA2 TX / PA3 RX, `1843200` baud, asynchronous data log enabled |

A source search found no application call to `MC_StartMotor1`; the generated firmware does not automatically request motor start merely by booting. This is a static source observation, not permission to flash or energize it.

## Ke Quantization Review Item

The seed preserves the profiler value `BEmfConstant=0.4490450918674469 Vrms phase-to-phase/kRPM`, while the generated `.ioc`, `pmsm_motor_parameters.h`, and exported motor configuration register contain `0.4`. The generated source uses this rounded value for reported motor configuration; source-reference search did not find it in the Hall FOC current/speed control path.

No generated file was changed to force additional precision. Keep this discrepancy together with the profiler run-to-run Ke spread (`0.385` to `0.62`) as a review item before the first energized test. Pole pairs, Rs, Ls, Hall placement/displacement, current topology, and pin mapping matched the intended configuration.

## Stop Point and Future Safety Gate

The project is deliberately paused at **generated + offline-build-verified**. No flash or run follows automatically.

Before any future flash, obtain a new same-session HITL confirmation covering all three items: explicit authorization, motor bus disconnected/USB-only, and target board identity NUCLEO-G431RB ST-Link SN `002A00403234510E33353533`. Before later applying motor bus power, use a separate authorization and physical safety check with attended emergency power removal; review the Ke item and the lack of a current-limited supply/inline fuse first.
