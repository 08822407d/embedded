# 接手协议(HANDOFF)

本文件是"如何接手并持续维护本项目"的完整说明。**首次接手请通读一遍**;之后作为查阅手册。
根目录 `CLAUDE.md` 是它的精简索引版。

## 1. 为什么有这套东西
本地以 Claude Code 为主、Codex 为辅协作开发。Claude Code 的对话上下文是**易失**的:

- 关闭 / 清空会话、上下文超限被截断 → 任务记忆没了;
- 移动项目文件夹 → Claude Code 那套按绝对路径定位、且跨所有 embedded 项目共享的记忆(`MEMORY.md`)随之失联;
- 手滑删文件 → 没入库就找不回。

所以:**凡是"接手任务所必需的信息",一律落盘到本项目 `docs/` 并入 git。** 这套文档随项目目录走、能被 git 找回,是唯一可靠的真相来源。

## 2. 核心原则
- 真相在 **磁盘 `docs/` + git 历史**,不在对话里。
- 对话产出的任何结论,没写进 `docs/` 就当它不存在。
- 写完就 `commit`,git 是防误删 / 防丢失的安全网。

## 3. 分工契约(Claude Code ↔ Codex)
| 角色 | 负责 | 写到哪 |
|---|---|---|
| Claude Code(主) | 需求分析、方案与架构设计、任务拆解、验收 | `REQUIREMENTS.md`、`DESIGN.md`、`DECISIONS.md` |
| Codex | 按设计实现代码、写 / 跑测试、调试 | `src/`、`test/`,必要时补 `OPERATIONS.md` |
| 两者 | 每段工作结束更新进度 | `STATUS.md` |

**交接动作**:CC 把 `DESIGN.md` 里某一块定稿 → 在 `STATUS.md` 标"已派给 Codex:<什么>" → Codex 实现并回报 → CC 验收后在 `STATUS.md` 标"已验证"。

## 4. 新会话接手流程
1. 读根 `CLAUDE.md`(通常自动加载)。
2. 读 `docs/STATUS.md` —— 立刻知道在哪、下一步、卡点。
3. 按需读 `REQUIREMENTS`(要做啥)、`DESIGN`(怎么搭)、`DECISIONS`(为什么这么定)、`OPERATIONS`(怎么编译 / 烧录 / 测)。
4. 看最近 git 历史补足"上次具体改了什么":`git log --oneline -20`。
5. 接着干;每段工作结束按第 5 节维护。

## 5. 维护纪律(什么时候更新哪个)
- **每段工作收尾 / 感觉上下文要满**:更新 `STATUS.md`(覆盖"进行中 / 下一步",别无限堆历史 —— 历史交给 git)。
- **做了非显然的技术选择**:在 `DECISIONS.md` 追加一条(日期 / 选了什么 / 为什么 / 否了什么)。
- **需求或验收变了**:改 `REQUIREMENTS.md`。
- **架构或接口变了**:改 `DESIGN.md`。
- **发现新的编译 / 烧录 / 调试坑或命令**:补 `OPERATIONS.md`。
- 改完一律 `git commit`。

## 6. 提交约定(轻量即可)
- 文档更新:`docs: <动词> <对象>`,例 `docs: update STATUS after roller pinout probe`。
- 代码:沿用现有简洁祈使风格,例 `Add roller speed control`。
- 想做快照备份可以,但 **绝不能用"删文件"的方式备份** —— 上一个项目就是这么把整套文档删没的。

## 7. 恢复手册(出事了怎么办)
- **上下文丢了**:读 `docs/STATUS.md` + `git log --oneline -30`,即可重建任务状态。
- **某文件被删 / 改坏(已提交过)**:`git restore -- docs/STATUS.md`,或 `git checkout <commit> -- <路径>` 取回旧版。
- **删了还没提交**:`git restore <路径>`(改动未暂存时),或从 `git stash` / `git reflog` 找。
- **整个目录被误删但提交过**:`git checkout HEAD -- M5_projs/Atom_Generic/Atom_Roller_Bala/`。
- **要迁移项目到别处**:整目录复制即可(`docs/`、`CLAUDE.md` 都在里面随之迁走);到新位置后视情况 `git init` 重新接上安全网(旧仓库历史仍留在原仓库)。

## 8. CLAUDE.md 是树状的(按需生长)
- 根 `CLAUDE.md` 只放导航 + 必守规则,保持轻量,不堆全部内容。
- 等 Codex 建出代码后,可在子目录(如 `src/`、某个复杂模块、`tools/`)放**局部 `CLAUDE.md`**,只记该目录的局部约定;Claude Code 进入该目录工作时才加载,既不撑大根文件,也让上下文按需就近。
- (可选)若希望每次会话自动带上当前进度,可在根 `CLAUDE.md` 里加一行 `@docs/STATUS.md` 自动内联;**默认不开**,以保持 token 精简 —— 靠"第一步读 STATUS"的指令即可达到同样效果。
