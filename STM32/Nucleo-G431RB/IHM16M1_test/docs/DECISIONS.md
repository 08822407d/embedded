# DECISIONS — 关键选择及其理由

> **只追加,别推翻已决之事。** 每条:编号 + 日期 + 决定 + 理由 + (可选)备选与放弃原因。
> 若要变更某条决策,**新增一条**说明"取代 #00X 及原因",不要原地删改历史。

---

## #001 — 采用 C6L 式持久化体系 + Codex 分工(2026-06-29)
- **决定**:本项目沿用 M5 `C6L_WalkieTalkie` 的方法论 —— 根 `CLAUDE.md` 接手入口 + `docs/` 七件套(STATUS/REQUIREMENTS/DESIGN/DECISIONS/OPERATIONS/FACTS/HANDOFF);CC 主需求/设计/验收,Codex 经 `docs/CODEX_TASK_*.md` 主实现。
- **理由**:对话上下文随时会断,真相须落盘 + git;把具体编码交 codex 省 Claude 额度。用户明确要求参考这两套机制。
- **适配**:红线从 C6L 的"持续射频发射"改为本项目的"未授权不烧录 / 不上电机电源 / 不驱动电机转动";工具链与外设事实针对 STM32G431 + IHM16M1 重写。
- ⚠️ **铁律(血泪)**:**备份绝不用"删文件"的方式** —— 上一个被删的 matrix/agent-context 模板就是这么把整套文档弄丢的。要快照用复制或 git。
- **配套**:`.claude/settings.json` 把只读/安全命令加 `allow` 白名单减弹窗,把烧录命令(多工具链候选)设 `ask` 必问,作红线的技术兜底。

## #002 — 工具链(待定)
- _尚未决定。候选:STM32CubeMX/CubeIDE、ST MCSDK(FOC 代码生成)、PlatformIO(ststm32)、裸 HAL/LL。定下后补本条并写 OPERATIONS.md。_
