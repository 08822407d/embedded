# HANDOFF — 完整接手协议(分工 · 纪律 · 恢复手册)

> 给任何接手本项目的 AI 会话 / 人。`CLAUDE.md` 是轻量入口,本文件是展开版。

## 1. 恢复上下文的顺序(新会话照做)
1. 读 [`../CLAUDE.md`](../CLAUDE.md) —— 项目定位、红线、分工。
2. 读 [`STATUS.md`](STATUS.md) —— 现在到哪、下一步、卡点。
3. 按需读 [`REQUIREMENTS.md`](REQUIREMENTS.md) / [`DESIGN.md`](DESIGN.md) / [`DECISIONS.md`](DECISIONS.md) / [`FACTS.md`](FACTS.md) / [`OPERATIONS.md`](OPERATIONS.md)。
4. 看 `git log` 最近提交,对齐磁盘真相。
- **你没有对话记忆;不在这些文件 + git 里的"印象"一律不可信。**

## 2. 分工契约
| 角色 | 负责 | 产出 |
|---|---|---|
| **Claude Code(主)** | 需求分析、方案 / 架构设计、任务拆解、**验收** | `docs/`(含 `CODEX_TASK_*.md` 任务书) |
| **Codex(辅)** | 照 `DESIGN.md` / 任务书写代码、编译、跑测试、调试 | `src/`、测试、把实测结果回写 `docs/` |

**为省 Claude 额度**:具体编码 / 反复编译调试尽量交 codex;CC 聚焦需求与设计、定"测什么 / 怎样算过",把"怎么测"留给 codex 自行设计。

## 3. Codex 任务书(`CODEX_TASK_*.md`)写法(分派时套用)
1. **顶部声明**:"读完即可自主执行" + 项目根**绝对路径** + "真相以 `docs/` 为准,动手前先读 FACTS/DECISIONS/OPERATIONS/REQUIREMENTS"。
2. **§0 权限预检(关键)**:开工前把所有**会触发权限弹窗**的动作(联网 clone / pip、串口 `/dev/ttyACM*`、烧录、工作区外写)各用**最小探针**跑一遍,让旁边**值守的用户一次性批准**,再自主跑主线,避免中途卡死。
3. **已验证事实直接喂**(引 FACTS 的 F-x),省 codex 再查;另列**"你自己查"清单**(读哪些源码 / 官方文档),别让 CC 代查。
4. **目标已定、别推翻**;给执行阶段。
5. **测试**:CC 给"**测什么**"(可观测、可复现的验收目标),"**怎么测**"由 codex 读文档自行设计;产出 + 实测结果回写 `docs/` 并更新 STATUS。
6. **纪律 / 边界**:改动限本项目目录,别破坏 monorepo 其它项目;红线照 `CLAUDE.md`。

## 4. 维护纪律(每次收尾)
1. **更 [`STATUS.md`](STATUS.md) → `git commit`**(在仓库根 `/home/cheyh/projs/embedded`)。不提交 = 没存。
2. 非显然技术选择 → 追加 [`DECISIONS.md`](DECISIONS.md)(只追加)。
3. 需求 / 设计变化 → 同步 [`REQUIREMENTS.md`](REQUIREMENTS.md) / [`DESIGN.md`](DESIGN.md)。
4. 新核实的事实 → [`FACTS.md`](FACTS.md)(带来源 + 日期)。
5. 代码模块可放**局部 `CLAUDE.md`**;根文件保持轻量。
6. 带"切换开发机 / 环境"意图时,先确认再出可他机接手的交接单。

## 5. 提交约定 + 备份铁律
- **提交信息**:文档用 `docs: <动词> <对象>`(如 `docs: 建立持久化体系骨架`);代码用简洁祈使句(如 `Add 三相 PWM 互补输出初始化`)。
- ⚠️ **铁律:备份绝不用"删文件"的方式。** 上一个项目就是用删文件做"备份",把整套文档弄丢的(见 [`DECISIONS.md`](DECISIONS.md) #001)。要快照就**复制**或靠 git,**绝不删**。
- **关键跨会话约定内联**:红线 / 中文回复 / 先探测 / 省 codex 额度等关键约定已**写进本仓库**(`CLAUDE.md` + 本文件),理由 —— 换开发机 / 全新会话可能**没有全局记忆**(`~/.claude/.../MEMORY.md` 按绝对路径 `/home/cheyh/projs/embedded` 作键、不随仓库走),仓库必须自洽,新环境照仓库做即可。

## 6. 恢复手册(出事了怎么办)
- **上下文丢了**:读 [`STATUS.md`](STATUS.md) + `git log --oneline -30`,即可重建任务状态。
- **文件被删 / 改坏(已提交过)**:`git restore -- <路径>`;取旧版 `git checkout <commit> -- <路径>`。
- **删了还没提交**:`git restore <路径>`(改动未暂存时);或从 `git stash` / `git reflog` 找。
- **整目录误删但提交过**:`git checkout HEAD -- STM32/Nucleo-G431RB/IHM16M1_test/`。
- ⚠️ **未入库 = 安全网未激活**:在本项目**首次 `git add` + `commit` 之前,以上 git 恢复手段全部无效**。骨架建好 / 接手后第一件事就是激活安全网(见 STATUS 下一步)。
- **迁移到别处**:整目录复制即可(`docs/`、`CLAUDE.md`、`.claude/` 随之走);到新位置确认 git 是否仍跟踪。

## 7. 红线(完整版见 `CLAUDE.md`)
未经本会话明确授权,**不编码实现、不烧录、不上电机电源 / 不驱动电机**。板子常无人值守,电机物理转动不可远程急停。烧录前确认目标确为 G431RB(先探测)、电机断电或空载、PWM 默认 0 占空。
