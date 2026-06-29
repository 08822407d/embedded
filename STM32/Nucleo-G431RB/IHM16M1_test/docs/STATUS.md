# STATUS — 现在到哪 / 下一步 / 卡点

> **每次会话先读本文件;收尾必更新它,然后 `git commit`。** 这是恢复上下文的第一站。
> 维护原则:**只覆盖『进行中 / 下一步 / 卡点』,别无限堆历史 —— 历史交给 git。**

**最后更新:2026-06-29**

## 现在到哪
- 持久化骨架 + `.claude/settings.json` 免授权已建并入 git,**安全网已激活**。
- **设备探测完成**:NUCLEO-G431RB 在线(ST-Link SN / Device ID / 工具链见 [`FACTS.md`](FACTS.md) F-3~F-6);**端口不固定,每次烧录/调试前现场扫**(见 [`OPERATIONS.md`](OPERATIONS.md) §0)。
- **需求与路线已定**(见 [`REQUIREMENTS.md`](REQUIREMENTS.md) + [`DECISIONS.md`](DECISIONS.md) #003):用 **ST X-CUBE-MCSDK** 的 Motor Profiler 标定散货三相 BLDC(自带霍尔)→ MC Workbench 生成霍尔 FOC 固件 → 烧录调速。硬件 = ST 官方套件 **P-NUCLEO-IHM03** 组合(F-7)。
- **工具链**:本机 CubeMX 6.15 / G4 FW 1.6.1 / CubeCLT 1.18 / CubeIDE 1.18.1 已就位,**只差 X-CUBE-MCSDK 6.4.1 待装**(F-9/F-10,需用户登录 st.com)。
- **仍未**:装 MCSDK、标定电机、写/生成代码、烧录、给电机上电。

## 下一步(按顺序)
- [ ] **装 X-CUBE-MCSDK 6.4.1**(用户主导,登录 st.com;本机其余环境已齐)
- [ ] 核实硬件电气配置:**IHM16M1 采样跳线(单/三电阻,霍尔 FOC 需三电阻)**、霍尔→G431 引脚、母线电压范围 → 查 UM2415 / 看板子 → [`FACTS.md`](FACTS.md)
- [ ] 跑 **Motor Profiler** 标定散货电机(电机空载、可急停、有人值守)
- [ ] **MC Workbench** 配置(三电阻 + 霍尔反馈)生成 FOC 工程 → 编译烧录 → 闭环调速验证

## 卡点 / 待用户拍板
- **X-CUBE-MCSDK 未装**(推进前提)。
- **采样跳线配置未知**:霍尔 FOC 必须三电阻,需确认 IHM16M1 当前配置。
- **电机能否被 Motor Profiler 顺利标定未知**(散货、可能是 gimbal/低电流 → 已知有坑,见 REQUIREMENTS 待确认)。
- **电机供电方案**(母线电压/电流、电源)未定。
