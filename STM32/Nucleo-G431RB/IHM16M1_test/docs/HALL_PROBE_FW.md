# Hall probe firmware

> Scope: read-only Hall sensor diagnostic firmware for NUCLEO-G431RB + X-NUCLEO-IHM16M1. The firmware must never drive the motor power stage.

## Safety boundary

- Motor bus/external supply remains disconnected during this task; only USB/Nucleo power is allowed.
- Firmware configures only:
  - USART2 VCP on PA2/PA3 for text output.
  - Hall inputs on PC6/PC7/PC8.
- Firmware does not configure TIM1, PWM outputs, PA8/PA9/PA10, PB13/PB14/PB15, STSPIN830 enable/gate signals, ADC current sensing, or any motor-control stack.
- Every flash attempt requires HITL confirmation of authorization, disconnected motor bus, and target board identity.

## Hardware facts used

- UM2415 Rev 4 p.9 Table 6 defines J1 as the Hall/encoder connector:
  - pin 1: A+ / Hall 1
  - pin 2: B+ / Hall 2
  - pin 3: Z / Hall 3
  - pin 4: 5 V supply from Nucleo development board
  - pin 5: Ground
- UM2415 Rev 4 p.9 Section 4.5 states the board provides pull-up resistors R20/R21/R22 for Hall/encoder open-drain or open-collector outputs.
- UM2415 Rev 4 p.11/p.12 schematics show J1 Hall nets H1/H2/H3, +5V, and the Nucleo connector routing.
- MCSDK 6.4.1 board database maps the selected/default Hall solution for NUCLEO-G431RB + X-NUCLEO-IHM16M1 to:
  - H1 = PC6 / TIM3_CH1
  - H2 = PC7 / TIM3_CH2
  - H3 = PC8 / TIM3_CH3
- MCSDK 6.4.1 NUCLEO-G431RB control-board database maps the default VCP UART to USART2:
  - TX = PA2
  - RX = PA3

Conclusion for the user's current power setup: Hall connector power is documented as `5 V supply from Nucleo development board`, so external IHM16M1 motor-bus power is not required for this Hall-read test. If the actual motor/Hall assembly requires a different sensor supply than J1 pin 4, stop and inspect that motor-specific wiring before continuing.

## Firmware design

Location: `hall_probe_fw/`

- `src/main.c`: minimal CMSIS/bare-register firmware.
- `STM32G431RB_FLASH.ld`: local linker script for STM32G431RB 128 KiB flash / 32 KiB RAM.
- `Makefile`: builds with CubeCLT GCC and CMSIS from the installed STM32CubeG4 package.
- `build_win.ps1`: Windows build wrapper.
- `monitor_win.ps1`: serial monitor helper using .NET `SerialPort`.

Runtime behavior:

- Prints a safety banner over USART2 at 115200 8N1.
- Samples PC6/PC7/PC8 with a small software stability filter.
- Reports:
  - raw Hall bits as H1/H2/H3
  - nominal six-step electrical sector and nominal electrical angle
  - accepted transition count
  - H1 rising-edge count
  - invalid Hall states
  - skipped/non-adjacent transitions
- Send `r` or `R` over serial to reset counters.

Pole-pair measurement protocol:

1. Keep the motor bus disconnected; use USB/Nucleo power only.
2. Reset counters through serial with `r`.
3. User slowly turns the rotor exactly one mechanical revolution by hand.
4. Pole-pair count is the H1 rising-edge count over that one mechanical revolution.
5. Cross-check: total accepted transitions should be approximately `6 * pole_pairs`.

## Offline build

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\hall_probe_fw\build_win.ps1
```

Result on 2026-07-08:

```text
text=1964 data=4 bss=4 dec=1972 hex=7b4
output=hall_probe_fw\build\hall_probe_fw.elf
```

No flashing was performed during the offline build step.

## Flash and serial check

HITL-1 on 2026-07-08:

- User confirmed flash authorization.
- User confirmed IHM16M1 motor bus/external supply disconnected and only USB/Nucleo power present.
- User confirmed target board is NUCLEO-G431RB with ST-Link SN `002A00403234510E33353533`.

Flash command:

```powershell
& 'C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe' `
  -c port=SWD sn=002A00403234510E33353533 `
  -w .\hall_probe_fw\build\hall_probe_fw.elf -v -rst
```

Result:

- Connected target voltage: 3.28 V.
- Device ID: `0x468`, STM32G43x/G44x, flash 128 KiB.
- Programmed `1.92 KB` at `0x08000000`.
- Verification succeeded.
- MCU software reset performed.

Initial serial check on `COM4`, `115200 8N1`:

```text
hall=010 sector=4 elec_deg_nom=240 invalid=0 skipped=0
```

Counter reset through serial command `r` was verified:

```text
Counters reset.
hall=010 sector=4 elec_deg_nom=240 trans=0 h1_rise=0 invalid=0 skipped=0
```

## Hand-turn capture

HITL-2 on 2026-07-08:

- User prepared the motor for safe hand rotation with motor bus still disconnected.
- Codex opened `COM4` for 60 seconds and sent `r` to reset counters.
- User hand-turned the rotor approximately one mechanical revolution.

Observed Hall sequence in the user's rotation direction:

```text
010 -> 011 -> 001 -> 101 -> 100 -> 110 -> 010
```

This is a valid six-state Hall sequence. During the capture:

```text
final trans=25 h1_rise=4 invalid=0 skipped=0
```

Interpretation:

- `h1_rise=4` over one mechanical revolution determines the motor pole-pair count as **4 pole pairs**.
- A 4-pole-pair motor should produce about `6 * 4 = 24` accepted Hall transitions per mechanical revolution.
- The capture ended at `trans=25`, one Hall sector beyond the exact starting state, which is consistent with the rotor being turned slightly past the start mark.
- `invalid=0` and `skipped=0` show that the Hall wiring and read chain were coherent for this hand-turn direction.

Hall-to-electrical-angle chain implemented in the diagnostic firmware:

| Hall H1/H2/H3 | Nominal sector | Nominal electrical angle |
|---|---:|---:|
| `100` | 0 | 0 deg |
| `101` | 1 | 60 deg |
| `001` | 2 | 120 deg |
| `011` | 3 | 180 deg |
| `010` | 4 | 240 deg |
| `110` | 5 | 300 deg |

In the captured hand-turn direction, the nominal angle moved in the decreasing order `240 -> 180 -> 120 -> 60 -> 0 -> 300 -> 240`.

## Test log

- 2026-07-08: task redlines and referenced docs read before implementation.
- 2026-07-08: user stated IHM16M1 has no external supply connected; only Nucleo power is present.
- 2026-07-08: UM2415/MCSDK evidence checked; Hall J1 5 V is from the Nucleo development board.
- 2026-07-08: read-only Hall probe firmware implemented and built offline.
- 2026-07-08: HITL-1 completed; firmware programmed and verified through ST-Link SN `002A00403234510E33353533`.
- 2026-07-08: initial serial read on `COM4` succeeded and showed stable valid Hall state `010`.
- 2026-07-08: HITL-2 hand-turn capture completed; `h1_rise=4`, `trans=25`, `invalid=0`, `skipped=0`.
- 2026-07-08: pole-pair conclusion = **4 pole pairs**.
