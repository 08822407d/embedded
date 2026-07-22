# STATUS — 现在到哪 / 下一步 / 卡点

> **每次会话先读本文件;收尾必更新它,然后 `git commit`。** 这是恢复上下文的第一站。
> 维护原则:**只覆盖『进行中 / 下一步 / 卡点』,别无限堆历史 —— 历史交给 git。**

**最后更新:2026-07-16**

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
- 正式工程有效配置包括:Hall PC6/7/8、120°/58°、4 对极、PWM 30kHz、三电阻双 ADC、**Iqmax/nominal current=0.3A、默认速度=100rpm**、USART2 MCP 1843200;Start/Stop 与 potentiometer 均关闭,源码无自动 `MC_StartMotor1` 调用。
- **首次带电前离线审计与降险准备已完成**:见 [`FOC_BRINGUP_PREP.md`](FOC_BRINGUP_PREP.md) / F-32~F-35。Ke 0.449→0.4 已定论为输出/只读上报量化,不进入当前 Hall FOC 运行时控制,无需仅为此重生成;F-23 序列换算后与 MCSDK `POSITIVE` 状态机完全一致。保护、默认安全态、MCP 可写项、100rpm/3-5s 首测参数、遥测与风险台账已落盘。
- **DECISIONS #006-B 已执行并离线验证(F-36)**:用户在 WB 将 Iqmax/nominal current `0.5→0.3A`、默认目标 `500→100rpm` 并用内置 CubeMX 重生成;Codex 未再次生成,仅以 CubeIDE 1.19.0 headless clean build Debug。结果 `0 errors,0 warnings`,size=`text36484/data976/bss7152`,新 ELF/HEX/BIN 留在 ignored `Debug/`;构建前后60项生成源码hash一致。三组PI最终数值与旧版相同,符合控制对象参数未变、只改变限幅/reference的预期。其余 Hall/采样/PWM/保护/MCP/Ke/无自动Start全部复核不变。
- **首次闭环带电 runbook 草稿已完成(仍未执行)**:见 [`FOC_BRINGUP_RUNBOOK.md`](FOC_BRINGUP_RUNBOOK.md)。草稿从生成源码与 MotorPilot 6.4.2 寄存器表核对了 STATUS/FAULTS、speed/Hall/Iq/Id/Vbus/temp、M1 data ID、`SPEED_RAMP=[S32 rpm,U16 ms]`、Start/Stop/Fault Ack 的精确语义,并按“安全闸→探测→逐次烧录 HITL→监视→100rpm点动→异常/急停→Ke复核→分级放开”组织。醒目标明 0.3A 仅为软件限幅、现有电源无保险丝;所有带电判断仍须 CC/用户现场定。本次只写 docs,未连接板卡、烧录、启动 MotorPilot/Workbench、上电、转电机、重生成或修改生成树。
- ✅ **首次闭环带电 bring-up 成功(2026-07-14,F-37)**:mode=UR 重烧+verify(HOTPLUG erase 曾失败)→ 板上确为 0.3A/100rpm FOC 固件;MotorPilot 上电 IDLE→Start→RUN 无故障,**电机闭环霍尔 FOC 正常旋转、方向/换相正确、电流受控**。整链验证通过,DECISIONS #003"闭环调速"目标达成。**低速下限 ~500rpm**(霍尔60°低速难点,#003风险③坐实);用户接受,应用侧用外接减速箱解决。GUI 花屏=显卡驱动 bug,更新驱动已修;Oray 远控/虚拟显示器已卸。
- [`DECISIONS.md`](DECISIONS.md) #003 路线已完成:标定→生成→烧录→**闭环调速验证成功**。剩下是可选的整定/表征/应用集成(见下一步)。
- 🔬 **位置控制可行性已离线研究完(2026-07-16,F-38,见 [`POSITION_CTRL_ANALYSIS.md`](POSITION_CTRL_ANALYSIS.md))**:①当前固件**只有速度/力矩控制,根本没编译位置控制**(源码核实);②官方 MCSDK 位置模式**要求正交编码器为主传感器,霍尔因分辨率过低不被允许**(ST 确认 + `TC_Init` 绑定编码器句柄印证),而本电机**无编码器轴**;③"整圈数"精度**不需要官方位置模式**,应用层数霍尔圈即可。三条路 A(加编码器,精度高/要硬件)/B(应用层数圈粗控,匹配"整圈数"、不加硬件、**推荐**)/C(魔改,不推荐)。**待用户选方向。** 纯离线,未碰硬件。
- 🎯 **位置控制已拍板 B 方案(DECISIONS #007,2026-07-16)**:应用层数圈粗控,Phase-1 PC 侧 PoC **零固件改动**;用户依据 = 大减速比减速箱稀释电机侧整圈误差、低于机械工艺公差,且应用转速远高于 ~500rpm 下限。源码预检修正前提:`MC_REG_CURRENT_POSITION` 零实现不可用(F-39),改用 `MC_REG_HALL_EL_ANGLE`(s16 电角度)解回绕计圈(4 电气圈=1 机械圈)。需求落 REQUIREMENTS §6;任务书 [`CODEX_TASK_revcount_poc.md`](CODEX_TASK_revcount_poc.md) 已写好,**待用户派发 codex 执行**。

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
- [x] 正式 FOC 首次带电前离线审计:Ke/Hall/保护/MCP/参数/遥测/风险已落 [`FOC_BRINGUP_PREP.md`](FOC_BRINGUP_PREP.md);未碰硬件/未重生成
- [x] **ESC-1/#006-B 已关闭**:用户选择回 WB 将 Iqmax/nominal current 改0.3A、默认目标改100rpm并重生成;Codex离线重构建和差异复核通过(F-36)
- [x] 将 [`FOC_BRINGUP_PREP.md`](FOC_BRINGUP_PREP.md) 落成精确寄存器/现场顺序的 [`FOC_BRINGUP_RUNBOOK.md`](FOC_BRINGUP_RUNBOOK.md) **DRAFT**;Codex 纯离线起草完成,未执行其中任何步骤
- [x] **CC 安全审核 runbook 通过并定稿**;用户就首次闭环测试给出 HITL-FLASH/HITL-POWER 授权
- [x] 烧录前 HITL(授权+母线断开/USB-only+目标板/SN正确);mode=UR 重烧+verify 通过
- [x] **首次闭环带电已完成并成功(F-37)**:Speed mode、IDLE→Start→RUN 无故障、电机正常旋转/换相/电流受控;实测**低速下限 ~500rpm**,用户接受(应用侧减速箱)
- [x] 位置控制方向已拍板:**B**(DECISIONS #007);需求 REQUIREMENTS §6;任务书 [`CODEX_TASK_revcount_poc.md`](CODEX_TASK_revcount_poc.md) 已写好
- [ ] **用户把 [`CODEX_TASK_revcount_poc.md`](CODEX_TASK_revcount_poc.md) 派发给 codex**:§1 选型(MotorPilot 脚本 vs 独立 MCP 客户端,优先调研高频遥测通道)→ §2 工具离线实现+自测 → §3 USB-only 手转计圈验证(母线断开、只读、用户手转)→ §4 带电 N 圈停止试验(**须新 HITL-POWER + 用户值守 + CC 旁站**)→ 数据/统计/结论落盘
- [ ] (可选)运行包络表征/整定:Ke 现场验证、速度 PI 低速表现、放开电流/转速分级建议
- [ ] 若只想复测 Windows 工具链健康,运行 `hw_smoke_win\build_win.ps1`;该脚本只生成/编译磁盘工程,仍然不要烧录测试固件

## 卡点 / 待用户拍板
- **Ke 输出量化已关闭为阻塞项(F-32)**:`0.4`不进入当前 Hall FOC 运行时控制,无需仅为此重生成。三次 Ke=0.62/0.385/0.449 的离散只保留为自动 PI 模型/整定风险,首测观察速度环即可;以后启用 observer/磁链算法再重测。
- **Hall 电气正向已静态自洽(F-33),机械正方向未定义**:F-23 手转序列就是 MCSDK POSITIVE,但当时未记 CW/CCW;且58°依赖Profiler后相/Hall线未变。两项都只能现场确认,不能离线假设。
- **供电/Iqmax 软件决策已关,硬件残余风险仍在**:项目speed-mode Iqmax已重生成到0.3A、默认目标100rpm;但现有12.6V/1.9A仍无可调限流/保险丝,`IQMAX`不是短路硬件trip。现场仍须有人值守并能立即断母线。
- **保护边界(F-34)**:PA11/BKIN2是combined driver-protection而非可设阈值OCP/刹车;`M1_OCP_TOPOLOGY=NONE`,PC4只测功率板温度。不能依赖它们保护微型电机绕组/短路。
- **现场 runbook 仍是待审 DRAFT**:精确操作顺序已写入 [`FOC_BRINGUP_RUNBOOK.md`](FOC_BRINGUP_RUNBOOK.md),但 CC 尚未审核,用户也尚未就本次首次闭环测试重新授权。任何后续烧录、连接 MotorPilot、给母线上电、Start/转电机都不在本任务授权内;须分别按 runbook/HITL 重新授权、有人值守、随时能断电。
