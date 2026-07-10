# STATUS — 现在到哪 / 下一步 / 卡点

> **每次会话先读本文件;收尾必更新它,然后 `git commit`。** 这是恢复上下文的第一站。
> 维护原则:**只覆盖『进行中 / 下一步 / 卡点』,别无限堆历史 —— 历史交给 git。**

**最后更新:2026-07-10**

## 现在到哪
- 持久化骨架 + 安全红线已建并入 git;本项目继续按 [`SYNC_WORKFLOW.md`](SYNC_WORKFLOW.md) 双机 `commit + push` 交接。
- **Linux 侧硬件/通信/工具链冒烟测试已完成**(只读 + 离线编译,未烧录):见 [`HW_SMOKE_TEST.md`](HW_SMOKE_TEST.md)。
- **Windows 侧硬件/通信/工具链冒烟测试已完成**(只读 + 离线编译,未烧录/未跑 MCSDK):见 [`HW_SMOKE_TEST_WIN.md`](HW_SMOKE_TEST_WIN.md)。同一块板 SN `002A00403234510E33353533` 在线,当前 Windows VCP 实测为 `COM4`(只作本次记录;以后仍按 SN 重扫),SWD HOTPLUG 只读读到 Device ID `0x468` / flash `128 KBytes` / option bytes,Windows 本机工具链编译出 `.elf/.bin/.hex`。
- **Windows 侧 MCSDK 已升级至 6.4.2**:`C:\Program Files (x86)\STMicroelectronics\MC_SDK_6.4.2`;MC Workbench `STMCWB.exe` product version 6.4.2、MotorPilot 存在,原 6.4.1 安装目录已不在。仅做文件盘点,未启动会驱动硬件的功能。
- **IHM16M1 当前实物采样拓扑已确认 = 默认 FOC 三电阻**(只读 + 下载 UM2415 + PDF 截图 + 用户看板反馈,未启动 MCSDK GUI/未驱动硬件):见 [`IHM16M1_SHUNT_CONFIG.md`](IHM16M1_SHUNT_CONFIG.md)。用户确认 `J5/J6` 闭合、底面 `JP4/JP7` 均 open;UM2415/MCSDK 均确认这就是默认 FOC 三电阻。FOC 单电阻也受支持,旧说法"霍尔 FOC 必须三电阻"已校正为"本项目因 Motor Profiler/未知电机路线优先三电阻"。
- **只读霍尔诊断固件已实现、离线编译通过,并在 HITL-1 授权后烧录/校验成功**:见 [`HALL_PROBE_FW.md`](HALL_PROBE_FW.md)。UM2415 确认 J1 Hall 5V 来自 Nucleo,因此当前无 IHM16M1 外部母线、仅 USB/Nucleo 供电的条件满足霍尔读取前提。固件只读 PC6/PC7/PC8 并通过 USART2 PA2/PA3 输出,不配置 PWM/TIM1/桥臂 enable/电机控制栈;串口初读 `hall=010` 且 `r` 清零命令有效。
- **霍尔手转极对数测量已完成**:HITL-2 用户手转约一机械圈,串口采集到连续有效序列 `010 -> 011 -> 001 -> 101 -> 100 -> 110 -> 010`,最终 `h1_rise=4`, `trans=25`, `invalid=0`, `skipped=0`;结论为当前散货电机 **4 对极**。`trans=25` 比理论 24 多 1 个 Hall sector,表示手转略过起点,不影响极对数结论。
- **目标电机已完整定型 = GM16020-06**(三相八线带霍尔,DC12V,空载 18200RPM,空载 0.16A,**4 对极**,6 槽,高 KV≈1517;详见 F-24)。属"高转速/低电流/低电感"微型电机,Motor Profiler 已知有坑,需预期要调参。
- **12V 母线电源已备妥**(F-25);标定时须经限流实验电源、限流起步 ≤0.5–1A。
- **Windows CubeMX 已升级 6.17.0-RC5 → 6.18.0**(F-12,2026-07-08 用户手工全量安装,运行时核验);MCSDK 6.4.2 内置 CubeMX 已成功生成正式工程且后续构建通过,当前组合不再阻塞。
- **Motor Profiler 固件 Phase B(仅离线编译)已完成**:MC Workbench 生成工程 `GM16020-06_profile` 以 CubeIDE 1.19.0 headless 构建 Debug 成功,`.elf/.hex/.bin` 留在 `GM16020-06_profile\STM32CubeIDE\Debug\`。该工程漏配 CMSIS-DSP include,构建命令以临时 `-I ...\Drivers\CMSIS\DSP\Include` 补齐,未手改生成树。完整命令、产物及首次失败记录见 [`PROFILER_FW_BUILD.md`](PROFILER_FW_BUILD.md)。该 Phase B 任务本身未烧录/未访问硬件;后续 Phase C/D 结果见下一条。
- **Motor Profiler 标定(Phase C 烧录 + Phase D 标定)已完成**:CC 在用户授权+母线断电下用 `STM32_Programmer_CLI` 烧 profiler 固件(verified、认 SN),用户操作 MotorPilot 完成**电机+霍尔完整标定**。结果见 **F-28**:Rs=1.41Ω / Ls=0.19mH / 极对数=4 / Ke(BEmf)=0.449 / 霍尔 placement=58°、displacement=120°;⚠️ Ke run 间跳动(0.385~0.62),FOC 时须验证/微调。原 json 在被忽略目录,已备份 [`docs/profiler_results/GM16020-06.json`](profiler_results/GM16020-06.json)。母线范围 7–45V 确认(F-29)。
- **正式霍尔 FOC 工程已生成并完成离线构建验证**:种子 `GM16020-06_foc.stwb6` 已保留,生成树 `GM16020-06_foc/` 按 [`GITIGNORE_AND_REGEN.md`](GITIGNORE_AND_REGEN.md) 忽略。STM32CubeIDE 1.19.0 headless clean build Debug 一次通过(`0 errors,0 warnings`),`.elf/.hex/.bin` 留在本机 ignored `Debug/`;未手改生成树。完整配置审计、命令、hash 和产物见 [`FOC_FW_BUILD.md`](FOC_FW_BUILD.md) / F-30/F-31。
- 正式工程有效配置包括:Hall PC6/7/8、120°/58°、4 对极、PWM 30kHz、三电阻双 ADC、默认速度 500rpm、USART2 MCP 1843200;Start/Stop 与 potentiometer 均关闭,源码无自动 `MC_StartMotor1` 调用。**注意:**种子 Ke=0.449045... 被生成 C 参数量化为 0.4,首次带电前仍要复核。
- [`DECISIONS.md`](DECISIONS.md) #003 路线已完成到“正式工程生成 + 离线编译”。**用户选择在此暂停**;正式 FOC 尚未烧录,未做首次闭环上电/调速,本次构建也未访问硬件。

## 下一步(按顺序)
- [x] 烧录前按任务书 HITL 确认:授权 + IHM16M1 外部母线断开/仅 USB/Nucleo 供电 + 目标板 SN `002A00403234510E33353533`
- [x] 烧录 `hall_probe_fw\build\hall_probe_fw.elf`,打开 VCP 串口,读取 PC6/PC7/PC8 Hall 状态
- [x] 用户手转转子一机械圈,用 H1 上升沿计数确定极对数,并用总 Hall transition 计数交叉验证
- [x] 目标电机定型(GM16020-06)、极对数=4、12V 电源备妥、CubeMX 升级 6.18.0 → 已落盘 F-24/F-25/F-12
- [x] CC 起草 **Motor Profiler 标定 SOP**(用户驱动 GUI + CC 旁站,非 codex 任务):见 [`MOTOR_PROFILER_SOP.md`](MOTOR_PROFILER_SOP.md)
- [x] Motor Profiler Phase B:仅以 headless 离线编译 `GM16020-06_profile(Debug)` 并产出 `.elf/.hex/.bin`;Phase C 未执行
- [x] **Phase C 烧录 + Phase D 标定已完成**:用户授权+母线断电下烧 profiler 固件,MotorPilot 完整标定电机+霍尔,结果落 F-28 + 备份 json(无保险丝,靠 Imax 0.5A 低起步 + 用户 1 秒断电兜底,标定顺利)
- [x] MC Workbench 6.4.2 生成正式霍尔 FOC 工程 `GM16020-06_foc`;种子入库、生成树忽略策略与重生成协议已落实
- [x] STM32CubeIDE 1.19.0 headless clean build Debug,生成 `.elf/.hex/.bin`;无需 CMSIS-DSP `-I` 绕过,无生成树手改
- [ ] **当前主动暂停,不要自动继续。** 若用户恢复:先读 [`FOC_FW_BUILD.md`](FOC_FW_BUILD.md) 的 Ke 量化项,再为烧录单独执行 HITL(明确授权 + 母线断开/仅 USB + 目标板 SN 正确);烧录不等于授权上母线或转电机
- [ ] 首次闭环带电另行获得授权并现场值守;先解决/接受限流与保险丝风险,从低速低电流开始,核对 Hall 方向/相序/故障状态/空载转速与 BEMF,必要时修正 Ke/PI
- [ ] 若只想复测 Windows 工具链健康,运行 `hw_smoke_win\build_win.ps1`;该脚本只生成/编译磁盘工程,仍然不要烧录测试固件

## 卡点 / 待用户拍板
- **Ke 标定值存疑**:三次 run 得 0.62/0.385/0.449,取 0.449;生成 FOC、首次跑起来后须**验证空载转速/BEMF 是否符合预期**,必要时微调 Ke。Rs/Ls/极对数/霍尔角均稳。
- **生成器 Ke 精度项**:`.stwb6` 保留 0.449045...,但生成 `.ioc`/C 参数为 0.4;未擅自手改。源码核对显示该值主要进入导出的 motor configuration register,仍须在首次上电前由后续负责人决定是否回 Workbench 调整或接受并实测。
- **供电仍无可调限流**(12.6V/1.9A,无保险丝):标定阶段靠 Imax 低起步 + 1 秒断电已顺利完成;将来闭环调速电流更大时,仍建议想办法加限流/保险丝。
- 任何后续烧录、给电机上电、让电机转动(尤其 FOC 闭环调速,电流可能远高于标定的 0.5A)的步骤,都必须重新获得明确授权、有人值守、随时能断电。
