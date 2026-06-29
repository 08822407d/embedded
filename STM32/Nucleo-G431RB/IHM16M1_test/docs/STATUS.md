# STATUS — 现在到哪 / 下一步 / 卡点

> **每次会话先读本文件;收尾必更新它,然后 `git commit`。** 这是恢复上下文的第一站。
> 维护原则:**只覆盖『进行中 / 下一步 / 卡点』,别无限堆历史 —— 历史交给 git。**

**最后更新:2026-06-29**

## 现在到哪
- 持久化骨架(`CLAUDE.md` + `docs/` 七件套)+ `.claude/settings.json` 免授权已建并入 git,**安全网已激活**。
- **设备探测完成**:NUCLEO-G431RB 在线(ST-Link SN / Device ID / 已装工具链见 [`FACTS.md`](FACTS.md) F-3~F-6);**端口不固定,每次烧录/调试前现场扫**(见 [`OPERATIONS.md`](OPERATIONS.md) §0)。
- **工具链**:本机 CubeCLT 1.18.0 / CubeIDE 1.18.1(非最新,最新见 F-6);X-CUBE-MCSDK 疑未装。**已决:暂缓升级/安装,等需求与工具链路线定了一起办**。
- **仍未**:编写功能代码、选定工具链路线、烧录、给电机上电。

## 下一步(按顺序)
- [ ] 用户说明项目**具体需求 / 控制目标** → 填 [`REQUIREMENTS.md`](REQUIREMENTS.md)
- [ ] 定**工具链路线**(走不走 MCSDK FOC、CubeIDE 2.x vs CubeCLT+VSCode)→ 连带决定升级/装 MCSDK → [`OPERATIONS.md`](OPERATIONS.md) / [`DECISIONS.md`](DECISIONS.md)
- [ ] 核实**硬件电气事实**(IHM16M1 驱动芯片、母线电压、电流采样方式、G431 的 TIM/ADC/GPIO 映射)→ [`FACTS.md`](FACTS.md)
- [ ] 出**架构设计** → [`DESIGN.md`](DESIGN.md);随后写 `CODEX_TASK_*.md` 分派 codex 实现

## 卡点 / 待用户拍板
- 项目**具体用途**未定(IHM16M1 可做 BLDC/PMSM FOC、六步换向、或仅做某子功能验证)。
- **工具链路线**未定 —— 直接影响 OPERATIONS、代码组织、以及升级/装 MCSDK 的时机。
- 电机型号 / 供电 / 是否有编码器或霍尔传感器,未知。
