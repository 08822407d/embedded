# STATUS — 现在到哪 / 下一步 / 卡点

> **每次会话先读本文件;收尾必更新它,然后 `git commit`。** 这是恢复上下文的第一站。
> 维护原则:**只覆盖『进行中 / 下一步 / 卡点』,别无限堆历史 —— 历史交给 git。**

**最后更新:2026-07-03**

## 现在到哪
- 持久化骨架 + 安全红线已建并入 git;本项目继续按 [`SYNC_WORKFLOW.md`](SYNC_WORKFLOW.md) 双机 `commit + push` 交接。
- **Linux 侧硬件/通信/工具链冒烟测试已完成**(只读 + 离线编译,未烧录):见 [`HW_SMOKE_TEST.md`](HW_SMOKE_TEST.md)。
- **Windows 侧硬件/通信/工具链冒烟测试已完成**(只读 + 离线编译,未烧录/未跑 MCSDK):见 [`HW_SMOKE_TEST_WIN.md`](HW_SMOKE_TEST_WIN.md)。同一块板 SN `002A00403234510E33353533` 在线,当前 Windows VCP 实测为 `COM4`(只作本次记录;以后仍按 SN 重扫),SWD HOTPLUG 只读读到 Device ID `0x468` / flash `128 KBytes` / option bytes,Windows 本机工具链编译出 `.elf/.bin/.hex`。
- **Windows 侧 MCSDK 已安装**:`C:\Program Files (x86)\STMicroelectronics\MC_SDK_6.4.1`;MC Workbench `STMCWB.exe` v6.4.1、MotorPilot 存在。仅做文件盘点,未启动会驱动硬件的功能。
- **IHM16M1 当前实物采样拓扑已确认 = 默认 FOC 三电阻**(只读 + 下载 UM2415 + PDF 截图 + 用户看板反馈,未启动 MCSDK GUI/未驱动硬件):见 [`IHM16M1_SHUNT_CONFIG.md`](IHM16M1_SHUNT_CONFIG.md)。用户确认 `J5/J6` 闭合、底面 `JP4/JP7` 均 open;UM2415/MCSDK 均确认这就是默认 FOC 三电阻。FOC 单电阻也受支持,旧说法"霍尔 FOC 必须三电阻"已校正为"本项目因 Motor Profiler/未知电机路线优先三电阻"。
- 需求与路线仍为 [`DECISIONS.md`](DECISIONS.md) #003:用 ST X-CUBE-MCSDK 的 Motor Profiler 标定散货三相 BLDC(自带霍尔) → MC Workbench 生成霍尔 FOC 固件 → 编译烧录 → 闭环调速。
- **仍未**:核实霍尔引脚/母线电压,运行 Motor Profiler,生成正式 FOC 工程,烧录,给电机上电,让电机转动。

## 下一步(按顺序)
- [ ] 核实其余硬件电气配置:霍尔→G431 引脚、母线电压范围 → 查 UM2415/UM2538/MCSDK 板卡定义/看板子 → 记录到 [`FACTS.md`](FACTS.md)
- [ ] 用户主导、有人员值守、确认电源/急停/空载安全后,在 Windows 侧跑 **Motor Profiler** 标定散货电机
- [ ] 用 **MC Workbench** 配置(三电阻 + 霍尔反馈)生成 FOC 工程
- [ ] 编译正式 FOC 工程;在明确授权、电机电源安全状态确认后,再进入烧录/调试步骤
- [ ] 若只想复测 Windows 工具链健康,运行 `hw_smoke_win\build_win.ps1`;该脚本只生成/编译磁盘工程,仍然不要烧录测试固件

## 卡点 / 待用户拍板
- **电机供电方案**(母线电压/电流、电源、急停/固定方式)未定。
- **Motor Profiler 标定风险**仍在:散货电机、可能是 gimbal/低电流类型,标定可能需要调整电压/转速/负载安全条件。
- 任何烧录、运行、Motor Profiler、MC Workbench 生成/下载、给电机扩展板上电或让电机转动的步骤,都必须重新获得明确授权并有人值守。
