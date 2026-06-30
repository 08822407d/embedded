# SYNC_WORKFLOW — 双机(Linux ⇄ Windows)经 GitHub 的保存与交接

> 本项目**必然在两台不同 OS 的开发机间切换**:
> - **Linux(Ubuntu,本仓所在机)**:需求/设计/文档主仓;工具链编译(CubeCLT)、烧录(CubeProgrammer)。
> - **Windows PC**:跑 **X-CUBE-MCSDK**(Motor Profiler 标定 + MC Workbench 生成 FOC)——**MCSDK 仅 Windows 版**(见 DECISIONS #004)。
> 保存与交接**统一走 GitHub `origin`(`github.com:08822407d/embedded`)的 push / pull**。

## 核心纪律(变更:从"本地 commit"升级为"commit + push")
1. **"保存" = `git commit` + `git push`**。本地 commit 只是半成品;**没 push 到 origin = 另一台机器看不到 = 没交接**。
2. **"接手" = 先 `git pull`**,再读 [`STATUS.md`](STATUS.md)。开工前必 pull,拿对端最新进度。
3. **一次只在一台机器上推进**:换机前先在当前机 `commit + push`,到另一台先 `pull`。**别在两台机器同时改、各自 commit 不 pull** —— 会分叉,跨 OS 合并很痛。
4. **收尾顺序**:更新 [`STATUS.md`](STATUS.md) → `commit` → **`push`**。

## 跨 OS 注意(Linux ↔ Windows)
- **换行符**:`.gitattributes` 已设仓库内统一 LF;别在某机把 `core.autocrlf` 设错导致全文件翻动。
- **工具路径不同,绝不写死**:
  - Linux:CubeCLT `/opt/st/stm32cubeclt_1.18.0/...`、CubeMX `~/STM/STM32CubeMX`(FACTS F-5/F-10)。
  - Windows:多在 `C:\Program Files\STMicroelectronics\...`(MCSDK/CubeMX/CubeProgrammer);到 Windows 机实测后记入 FACTS。
- **串口/端口现场扫**:Linux `/dev/ttyACM*` 按 ST-Link SN 解析;Windows `COM*` 按同一 SN(`002A...3533`)在设备管理器认;见 [`OPERATIONS.md`](OPERATIONS.md) §0。
- **`.claude/settings.json` 是机器相关的**(内含 Linux 工具路径/免授权规则)。Windows 上若也用 Claude Code,需另建本机 settings;它不是跨机权威。
- **Claude 记忆 `~/.claude/.../memory/` 不随仓库走**。Windows 机没有这套记忆 → **仓库 docs 必须自洽**(本项目一贯原则);新环境照 docs 做即可。

## 不入库(避免污染 + 跨机冲突)
- 构建产物(`build/`、`Debug/`、`Release/`、`*.elf/.bin/.hex`)、CubeMX/MCSDK 生成的第三方 HAL/CMSIS、IDE 本地配置(`.settings/`)—— 见 `.gitignore`。
- 生成工程靠**种子(`.ioc`)+ 脚本 / Workbench 配置**复现(参照 `hw_smoke/` 与 C6L `DECISIONS #004` 哲学)。

## 接手速查(到任一台机器)
1. `git pull`(取最新)。
2. 读 [`../CLAUDE.md`](../CLAUDE.md) → [`STATUS.md`](STATUS.md) → 相关 docs。
3. 干活;收尾 `commit` + **`push`**。
4. 板子刚插到本机:按 [`OPERATIONS.md`](OPERATIONS.md) §0 重新探测设备(端口/COM 不固定)。
