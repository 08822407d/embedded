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
- **Windows CubeMX 已升级 6.17.0-RC5 → 6.18.0**(F-12,2026-07-08 用户手工全量安装,运行时核验);RC5 的 headless 生成 bug 是否消失待生成 FOC 工程时复验。
- **Motor Profiler 固件 Phase B(仅离线编译)已完成**:MC Workbench 生成工程 `GM16020-06_profile` 以 CubeIDE 1.19.0 headless 构建 Debug 成功,`.elf/.hex/.bin` 留在 `GM16020-06_profile\STM32CubeIDE\Debug\`。该工程漏配 CMSIS-DSP include,构建命令以临时 `-I ...\Drivers\CMSIS\DSP\Include` 补齐,未手改生成树。完整命令、产物及首次失败记录见 [`PROFILER_FW_BUILD.md`](PROFILER_FW_BUILD.md)。**Phase C 烧录未执行,未访问硬件、未运行 MotorPilot。**
- 需求与路线仍为 [`DECISIONS.md`](DECISIONS.md) #003:用 ST X-CUBE-MCSDK 的 Motor Profiler 标定散货三相 BLDC(自带霍尔) → MC Workbench 生成霍尔 FOC 固件 → 编译烧录 → 闭环调速。
- **电气/参数前提基本齐备**(采样拓扑=三电阻、极对数=4、霍尔脚 PC6-8、12V 电源、工具链 6.18.0)。**仍未**:运行 Motor Profiler、生成正式 FOC 工程、给电机上电、让电机转动。

## 下一步(按顺序)
- [x] 烧录前按任务书 HITL 确认:授权 + IHM16M1 外部母线断开/仅 USB/Nucleo 供电 + 目标板 SN `002A00403234510E33353533`
- [x] 烧录 `hall_probe_fw\build\hall_probe_fw.elf`,打开 VCP 串口,读取 PC6/PC7/PC8 Hall 状态
- [x] 用户手转转子一机械圈,用 H1 上升沿计数确定极对数,并用总 Hall transition 计数交叉验证
- [x] 目标电机定型(GM16020-06)、极对数=4、12V 电源备妥、CubeMX 升级 6.18.0 → 已落盘 F-24/F-25/F-12
- [x] CC 起草 **Motor Profiler 标定 SOP**(用户驱动 GUI + CC 旁站,非 codex 任务):见 [`MOTOR_PROFILER_SOP.md`](MOTOR_PROFILER_SOP.md)
- [x] Motor Profiler Phase B:仅以 headless 离线编译 `GM16020-06_profile(Debug)` 并产出 `.elf/.hex/.bin`;Phase C 未执行
- [ ] **后续独立任务才可进入 Phase C**:先完成 SOP 物理准备(拆指针/固定电机/**串保险丝补限流**/急停/值守),再由用户按 HITL 明确授权烧录;烧录后仍须用户操作 MotorPilot,CC 旁站解读。标不动则转 SOP §7 Plan B 手工测参(需仪表)
- ⚠️ **本次电源无可调限流**(12.6V/1.9A,F-25):靠"Motor Profiler Imax 低起步 + 串 1–1.5A 快熔保险丝 + 低速起步 + 值守随时断电"四层兜底
- [ ] 用 **MC Workbench** 配置(三电阻 + 霍尔反馈)生成 FOC 工程
- [ ] 编译正式 FOC 工程;在明确授权、电机电源安全状态确认后,再进入烧录/调试步骤
- [ ] 若只想复测 Windows 工具链健康,运行 `hw_smoke_win\build_win.ps1`;该脚本只生成/编译磁盘工程,仍然不要烧录测试固件

## 卡点 / 待用户拍板
- **电机供电已定 12V**(F-25);仍需在标定任务书里定死:限流值、急停方式、电机固定方式、**拆除轴上打印指针**(高速会飞出)。
- **Motor Profiler 标定风险**仍在:GM16020-06 为高 KV/低电流/低电感微型电机,标定可能失败或需调电压/转速/电流安全条件——预期非一次过。
- 本轮 Phase B 已按范围结束。任何后续烧录、运行、Motor Profiler、MC Workbench 生成/下载、给电机扩展板上电或让电机转动的步骤,都必须重新获得明确授权并有人值守。只读霍尔固件每次烧录前也必须按 [`CODEX_TASK_hall_probe_fw.md`](CODEX_TASK_hall_probe_fw.md) §-1 重新确认授权、母线断电、目标板正确。
