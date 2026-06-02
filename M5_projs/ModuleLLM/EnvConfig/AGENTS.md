# AGENTS.md — M5Stack Module LLM Kit（AX630C）本地配置任务规则

## 0. 任务定位

本仓库不是普通软件开发项目。主要目标是通过 ADB/UART/SSH 对 M5Stack Module LLM Kit（AX630C）的内置 Linux/Ubuntu 系统进行安全、可恢复、可审计的配置与更新。

默认任务类型：设备运维 / Linux 配置 / 文档整理 / 自动化辅助。  
默认不做业务软件开发，不编译 StackFlow，不训练或转换模型，除非用户明确要求。

## 1. 支持的主机环境

本任务必须同时兼容至少两类开发主机：

- Windows 10：优先使用 PowerShell；ADB 命令为 `adb.exe` 或 PATH 中的 `adb`。
- Linux 发行版：优先使用 Bash；ADB 命令为 PATH 中的 `adb`。

每次会话都不能假设当前主机与上次相同。必须重新确认：

- Host OS 与 shell。
- `adb` 是否存在、版本是否能打印、ADB server 是否能启动。
- `ssh` 是否存在。
- 当前 USB/ADB 设备列表。

以后在任何新主机或新环境执行本项目任务时，至少先做两项检查任务：

1. Host ADB 工具检查：确认 `adb` 是否可调用；若没有发现 `adb`，停止并询问用户是否安装，不能擅自安装。
2. USB/ADB 设备扫描：确认当前 USB/PnP/`adb devices -l` 看到的设备；不能沿用旧 serial。

若系统中已安装 `adb` 但 PATH 未配置，优先设置本项目推荐环境变量 `ADB_BIN=<adb 或 adb.exe 绝对路径>` 后继续；持久写入用户/系统环境变量前仍需用户确认。配置正确后必须执行 `adb version`，并按可信来源检查/更新 Host 侧 Android SDK Platform Tools。

不要把 Windows 路径、Linux 路径、USB 端口名、ADB serial 作为跨机器永久事实。只能把它们记录为“某次会话观察值”。

## 2. 每次会话启动必须先读

在采取任何动作前，必须阅读并基于以下文件恢复上下文：

1. `context/PROJECT_STATE.md`
2. `context/PLAN_CURRENT.md`
3. `context/CONFIG_REGISTRY.md`
4. `context/DOCS_INDEX.md`
5. `context/RUNBOOK.md`
6. `context/SESSION_HANDOFF.md`
7. 当前要执行的 `tasks/*.md`

如果这些文件冲突，以 `PROJECT_STATE.md` 和 `SESSION_HANDOFF.md` 中最新日期的事实为准；仍不确定时，先通过只读命令复核，不要猜测。

## 3. ADB 与 USB 设备选择硬规则

官方操作路径以 ADB 为主，因此 ADB 是本任务的第一连接方式。每次通过 ADB 接触设备前，必须先完成 ADB preflight：

1. Host：确认 `adb version` 成功。
2. Host：执行 `adb start-server`。
3. Host：执行 `adb devices -l` 并保存输出。
4. 若没有 `device` 状态设备，停止，不继续执行 `adb shell/push/pull`。
5. 若有多个 `device` 状态设备，禁止使用不带 `-s <serial>` 的 `adb shell/push/pull`；必须让用户选择，或使用 `ADB_SERIAL=<serial>` / 脚本参数显式指定。
6. 每次会话、每次换线、每次换 USB 口、每次设备重启后，都要重新扫描；不能沿用上次端口或 serial 作为默认目标。

允许不指定 serial 的情况只有一个：`adb devices -l` 显示且解析后只有一个 `device` 状态设备。即便如此，也要把当次使用的 serial 记录到 `inventory/` 输出中。

## 4. 最高风险红线

严禁主动执行以下操作，除非用户在当前会话中明确、逐条确认：

- 对 `/dev/mmcblk0` 做分区、格式化、擦除、重建分区表、`fdisk`、`parted`、`mkfs`、`dd of=/dev/mmcblk0*` 等操作。
- 覆盖系统启动相关分区、bootloader、uboot、kernel、rootfs。
- 执行固件整机刷写、镜像底包更新。
- 清空 `/opt`、`/etc`、`/usr`、`/var/lib/dpkg`、`/var/lib/apt`。
- 改动网络、防火墙、SSH、账户密码前不做备份。
- 存储用户的真实密码、token、私钥、Wi-Fi 密码到仓库。

若用户要求危险动作，必须先说明风险、备份点、恢复路径和替代方案。

## 5. 默认工作方式

优先做小步、可验证、可回滚的改动：

1. 先只读探测：确认主机、连接方式、设备型号、系统版本、磁盘、网络、apt 源、已安装包。
2. 查 `context/CONFIG_REGISTRY.md`：确认相关配置项的采纳状态、检测命令、依赖关系、正确配置步骤和替代方案。
3. 当用户使用 Linux/UART/终端相关术语不精确时，以用户描述的应用场景为准，反推需要检查或修改的配置层，不按字面术语缩小问题。
4. 备份当前状态：把关键输出存入 `inventory/before/`，把配置文件备份到 `backups/`。
5. 生成执行计划：列出命令、预期输出、依赖、失败处理、回滚方法。
6. 用户确认后才执行会改变设备状态的命令。
7. 执行后记录实际输出摘要，更新 `PROJECT_STATE.md`、`CONFIG_REGISTRY.md`、`CHANGELOG.md`、`SESSION_HANDOFF.md`。

若检测到现场存在未记录配置，禁止直接覆盖或删除。必须先记录证据、判断影响范围，并在 `CONFIG_REGISTRY.md` 的“未记录配置观察”中登记，再决定采纳、保留、修复或回滚。

## 6. 命令执行约束

- 能用只读命令确认的，不通过猜测推进。
- 所有设备命令必须说明目标运行端：Host / ADB shell / UART shell / SSH shell。
- 需要改文件时，先备份原文件，例如 `cp -a file file.bak.$(date +%Y%m%d-%H%M%S)`。
- apt 安装前必须先 `apt update`，并检查磁盘空间。
- 模型包通常较大，安装前必须说明空间影响并按需安装。
- 不要无依据安装大量包，不要把 Module LLM 当通用 Ubuntu 服务器折腾。
- 长命令优先写入 `scripts/` 并让用户审阅。
- 脚本默认 Bash `set -eu` 或 PowerShell `$ErrorActionPreference = 'Stop'`；危险脚本必须有 dry-run 或显式确认变量。

## 7. 文档与上下文维护

每轮任务结束前必须更新：

- `context/PROJECT_STATE.md`：实际状态。
- `context/CONFIG_REGISTRY.md`：采纳配置、检测方法、依赖、未记录配置、替代方案和整体最佳方案。
- `context/PLAN_CURRENT.md`：当前实施方案版本。
- `context/SESSION_HANDOFF.md`：可复制到新机器/新会话的恢复包。
- `context/CHANGELOG.md`：本轮做了什么、输出保存在哪里、仍有何风险。
- 如有新结论，更新 `context/DOCS_INDEX.md` 的“已提取事实”。

不要把大段官方文档复制进 AGENTS.md。AGENTS.md 只保留规则；详细上下文放在 `context/`。

### 7.1 跨开发机 GitHub 交接触发

如果用户明确说明“当前开发机上的任务需要保存”“下次任务要更换开发机”“要在下一台开发机继续”等含义，必须理解为：用户要通过 GitHub 保存当前工作，并在下一台开发机拉取完整项目继续。

此时不能只给口头总结，必须先生成或更新可续接上下文：

- `context/PROJECT_STATE.md`：当前阶段、已完成/未完成任务、最新设备和主机观察值。
- `context/PLAN_CURRENT.md`：当前实施方案、下一步顺序、风险和依赖。
- `context/SESSION_HANDOFF.md`：新开发机可直接复制执行的恢复包，包含读取顺序、环境检查、下一步动作、关键输出路径。
- `context/CHANGELOG.md`：本轮实际做了什么，哪些输出/证据文件需要随仓库保留。
- `context/CONFIG_REGISTRY.md`：若本轮涉及配置项，记录采纳状态、检测命令、依赖和替代方案。

随后执行 GitHub 交接准备：

1. 检查 `git status`，区分本项目文件、设备证据输出和无关工作树改动。
2. 检查仓库中不得包含真实密码、token、私钥、Wi-Fi 密码等敏感信息。
3. 只纳入本项目续接所需文件；遇到无关改动或不确定文件，先说明并等待用户确认。
4. 若 Git remote 和认证可用，按用户“保存/换机继续”的明确意图提交并推送到 GitHub；若无法推送，生成明确的本地待提交状态和阻塞原因。
5. 最终回复必须说明新开发机如何续接：拉取仓库后先读哪些文件、先运行哪些主机/ADB 检查、下一步任务是什么。

## 8. 设备事实锚点

已知设备方向：

- M5Stack Module LLM Kit = Module LLM + Module13.2 LLM Mate。
- SoC：AX630C。
- 主要访问方式：ADB、UART、SSH。
- 系统：官方称内置 Ubuntu。
- 软件更新主要通过 M5Stack apt 源与 `apt`。
- 恢复/镜像更新属于高风险任务，另行确认。
- `/dev/mmcblk0` 是板载 eMMC 系统盘，绝不可擅自分区。

若现场探测结果与此处不一致，以现场探测结果为准，并记录差异。

## 9. 输出风格

- 直接给可执行步骤，不写空泛教程。
- 对每个动作标明风险级别：只读 / 低风险 / 中风险 / 高风险。
- 对每个阶段给验证命令和预期结果。
- 不确定时先复核，不编造 API、包名、路径、硬件能力。
