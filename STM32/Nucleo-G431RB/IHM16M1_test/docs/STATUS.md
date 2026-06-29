# STATUS — 现在到哪 / 下一步 / 卡点

> **每次会话先读本文件;收尾必更新它,然后 `git commit`。** 这是恢复上下文的第一站。
> 维护原则:**只覆盖『进行中 / 下一步 / 卡点』,别无限堆历史 —— 历史交给 git。**

**最后更新:2026-06-29**

## 现在到哪
- 刚建立持久化体系骨架(`CLAUDE.md` + `docs/` 七件套),仿 M5 C6L 项目方法论,内容多为**留白模板**。
- 硬件平台已知:**Nucleo-G431RB + X-NUCLEO-IHM16M1**(三相电机驱动板),但**具体用途 / 控制方案尚未确认**。
- **尚未编写任何功能代码,未选定工具链,未烧录,未给电机上电。**

## 下一步(按顺序)
- [ ] **激活 git 安全网**:首次 `git add` + `commit` 本骨架 —— 在此之前所有 git 恢复手段无效(见 [`HANDOFF.md`](HANDOFF.md) §6)。
- [ ] 用户说明项目**具体需求 / 控制目标** → 填 [`REQUIREMENTS.md`](REQUIREMENTS.md)
- [ ] 确认**工具链**(STM32CubeMX/CubeIDE、ST MCSDK FOC、PlatformIO、或裸 HAL)→ 填 [`OPERATIONS.md`](OPERATIONS.md)
- [ ] 核实**硬件事实**(IHM16M1 驱动芯片、母线电压、电流采样方式、G431 的 TIM/ADC/GPIO 映射)→ 填 [`FACTS.md`](FACTS.md)
- [ ] 出**架构设计** → [`DESIGN.md`](DESIGN.md);随后写 `CODEX_TASK_*.md` 分派 codex 实现

## 卡点 / 待用户拍板
- 项目**具体用途**未定(IHM16M1 可做 BLDC/PMSM FOC、六步换向、或仅做某子功能验证)。
- **工具链**未定 —— 直接影响 OPERATIONS 与代码组织方式。
- 电机型号 / 供电 / 是否有编码器或霍尔传感器,未知。
