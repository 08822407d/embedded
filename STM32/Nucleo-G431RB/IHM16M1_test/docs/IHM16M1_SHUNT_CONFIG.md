# X-NUCLEO-IHM16M1 current-sensing topology

> Scope: UM2415-only/current document analysis plus MCSDK board database cross-check. No flashing, no firmware run, no Motor Pilot/Workbench GUI, no motor drive action.

## Source

- ST official PDF: `UM2415 - Rev 4 - March 2024`, "Getting started with the X-NUCLEO-IHM16M1 three-phase brushless motor driver board based on STSPIN830 for STM32 Nucleo".
- URL: https://www.st.com/resource/en/user_manual/um2415-getting-started-with-the-xnucleoihm16m1-threephase-brushless-motor-driver-board-based-on-stspin830-for-stm32-nucleo-stmicroelectronics.pdf
- Local ignored copy: `hw_ref/ihm16m1/source/um2415-rev4-202403.pdf`
- Cross-check file: `C:\Program Files (x86)\STMicroelectronics\MC_SDK_6.4.1\Utilities\PC_Software\STMCWB\assets\hardware\board\power\X-NUCLEO-IHM16M1.json`

## Screenshot Set

Open these files when checking the physical board:

- UM2415 p.7, Table 2, mode/topology settings: `hw_ref/ihm16m1/shots/um2415_p07_table2_algorithm_sensing_topology.png`
- UM2415 p.7, Table 3, J5/J6 op-amp settings: `hw_ref/ihm16m1/shots/um2415_p07_table3_opamp_j5_j6.png`
- UM2415 p.14, Figure 8, current-sensing schematic: `hw_ref/ihm16m1/shots/um2415_p14_current_sensing_schematic.png`
- UM2415 p.5, Figure 2, board component positions: `hw_ref/ihm16m1/shots/um2415_p05_figure2_component_positions.png`
- UM2415 p.16, BOM rows for JP4/JP7/J5/J6/M7/M8: `hw_ref/ihm16m1/shots/um2415_p16_bom_jp4_jp7_j5_j6.png`

## Current Physical Board Conclusion

User visual inspection on 2026-07-03 confirms:

- J5 and J6 are both closed with shunt caps.
- JP4 and JP7 on the bottom side are both open: clean separated solder-bridge pads, no solder bridge, wire, or component connecting them.
- J3 was observed as connected on the side away from the Hall-sensor connector. This is recorded as an observation only; the current-sensing topology conclusion below does not depend on interpreting J3 pin numbering.

Therefore this physical IHM16M1 board is in the UM2415 default **FOC 3-shunt current-sensing** topology.

## Configuration Map

| Mode | JP4 / JP7 bottom solder bridges | J5 / J6 jumpers | J2 | J3 | Meaning |
|---|---|---|---|---|---|
| FOC, 3-shunt | Open | Closed | 2-3 | 1-2 | UM2415 default configuration. Three independent phase shunts are used. |
| FOC, single shunt | Closed | Closed | 1-2 | 2-3 | FOC single-shunt configuration. All op-amp channels sense the same current. |
| 6-step | Closed | Open | 1-2 | 2-3 | 6-step configuration, not the target FOC path. |

Key details:

- JP4 and JP7 are bottom-side solder bridges, not ordinary removable jumpers. In the BOM they are `OPEN` solder bridges by default.
- J5 and J6 are two-pin THT headers. For FOC they must be closed with shunt caps. For 6-step they are open.
- J2/J3 select the STSPIN830 SNS/current limiter path and VREF threshold mode. They are part of the UM2415 mode table, but the single-vs-three current-sensing topology is primarily determined by JP4/JP7 plus the FOC/6-step J5/J6 state.
- The current-sense resistors are R57/R63/R70, each 0.33 ohm. In single-shunt topology the equivalent shunt value is 0.11 ohm.
- MCSDK 6.4.1 board JSON agrees: `ThreeShunt_AmplifiedCurrents` says default is JP4/JP7 open and J5/J6 closed; `SingleShunt_AmplifiedCurrents` says close JP4/JP7 and close J5/J6.

## 0-ohm resistor routing

The factory 0-ohm resistor network routes current feedback to the Nucleo morpho connector:

- Phase U current feedback: R29 to CN7.30.
- Phase V current feedback: R42 to CN10.24 and R41 to CN10.18.
- Phase W current feedback: R36 to CN7.34 and R34 to CN10.15.

These 0-ohm links are factory-default routing choices in UM2415/MCSDK. They are not the normal user-facing switch between single-shunt and three-shunt operation; JP4/JP7 and J5/J6 are the inspection targets for this task.

## Physical Inspection Checklist

Do not change hardware for this task. Only inspect.

1. Keep motor power off. If any external supply is attached to the IHM16M1 power input, turn it off before inspection.
2. Use the Figure 2 screenshot to locate J5/J6 near the upper-right area of the IHM16M1 board. In the provided top-side photo, this area is visible, but the final answer should come from direct visual inspection.
3. Report whether J5 and J6 each have a black shunt cap connecting their two pins:
   - caps installed = closed
   - caps missing = open
4. Flip/inspect the underside for JP4 and JP7 solder bridges:
   - open = two pads not bridged by solder
   - closed = solder blob or bridge connects the pads
5. If easy, also report J2 and J3 positions:
   - default FOC 3-shunt: J2 = 2-3, J3 = 1-2
   - FOC single-shunt: J2 = 1-2, J3 = 2-3
6. Determine current sensing:
   - JP4/JP7 open + J5/J6 closed = FOC 3-shunt, factory default.
   - JP4/JP7 closed + J5/J6 closed = FOC single-shunt.
   - JP4/JP7 closed + J5/J6 open = 6-step/current-limiter setup, not the target FOC setup.
   - Any mixed/inconsistent state = do not run; record exactly what is seen.

## Hall FOC Clarification

The old shorthand "Hall FOC must be three-shunt" is too absolute.

UM2415 documents both `FOC (3-shunt)` and `FOC (single shunt)` configurations for this board. MCSDK 6.4.1 also lists both three-shunt and single-shunt current-sensing hardware variants, and separately lists Hall sensor speed/position sensing. Therefore Hall feedback itself does not make three-shunt mandatory.

For this project route, three-shunt remains the preferred/default target because:

- It is the UM2415 default FOC configuration.
- MCSDK release notes state Motor Profiler is not available for single-shunt current sensing.
- Three-shunt gives more direct phase-current observability and is the safer baseline for an unknown motor.

## HITL Record

- HITL-1, 2026-07-03: user authorized UM2415 download and installing `pymupdf` for PDF rendering.
- HITL-2, 2026-07-03: user inspected the board and provided a bottom-side photo; JP4/JP7 are open, J5/J6 are closed. Conclusion: current physical board = default FOC 3-shunt.
