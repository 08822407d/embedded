# M5Stack Module LLM Kit（AX630C）Codex 本地任务书

目标：让 Codex 在本地目录中持续、可迁移、可恢复地协助完成 Module LLM Kit 的 Linux/ADB/UART/SSH/apt 配置工作。该项目不是软件开发工程，默认不编写业务程序；所有工作围绕设备状态确认、配置、备份、更新、恢复与记录。

本任务书按“双主机环境”设计：你可以在 Windows 10 或 Linux 发行版上继续同一任务。每次会话都必须重新检查当前主机、ADB 工具和 USB/ADB 设备列表，不能默认沿用上一次的 USB 端口、串口号或 ADB serial。

## 推荐启动方式

在本目录执行：

```bash
codex
```

给 Codex 的首条提示：

```text
这是 M5Stack Module LLM Kit（AX630C）本地运维配置任务。当前主机可能是 Windows 10，也可能是 Linux。请先阅读 AGENTS.md、context/PROJECT_STATE.md、context/CONFIG_REGISTRY.md、context/DOCS_INDEX.md、context/RUNBOOK.md、context/PLAN_CURRENT.md，以及 tasks/00_intake.md。先只做状态复核和下一步计划，不要修改设备。若要接触设备，必须先确认 adb 已安装可用，并执行 adb devices -l 重新扫描当前 USB/ADB 设备。
```

恢复旧任务时使用：

```text
这是一次恢复会话。请先阅读 AGENTS.md、context/SESSION_HANDOFF.md、context/PROJECT_STATE.md、context/CONFIG_REGISTRY.md、context/CHANGELOG.md、context/PLAN_CURRENT.md。当前主机可能不同，USB 端口和 ADB serial 也可能不同；请先列出你理解的当前状态、已完成事项、风险和下一步，然后等待我确认。
```

## 首次主机检查

Linux：

```bash
bash scripts/host_check.sh
```

Windows PowerShell：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\host_check.ps1
```

这一步只检查本机和 ADB 设备列表，不修改 Module LLM。

新主机最低要求：

1. 先做 Host ADB 工具检查。
2. 再做 USB/ADB 设备扫描。

若没有发现 `adb`，先询问是否安装；若已安装但 PATH 未配置，优先设置 `ADB_BIN` 指向 `adb`/`adb.exe` 绝对路径。配置正确后记录 `adb version`，并按可信来源检查/更新 Platform Tools。

## ADB 设备探测

Linux：

```bash
bash scripts/device_probe_adb.sh
# 多个 ADB 设备时：
ADB_SERIAL=<serial-from-adb-devices> bash scripts/device_probe_adb.sh
```

Windows PowerShell：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
# 多个 ADB 设备时：
$env:ADB_SERIAL="<serial-from-adb-devices>"
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
```

规则：每次换线、换 USB 口、重启设备、换主机后都重新运行 `adb devices -l`。如果有多个 ADB 设备，必须显式指定目标 serial。

## 文件分工

- `AGENTS.md`：Codex 必须遵守的短规则。保持精简，避免超出 Codex 项目说明大小限制。
- `context/PROJECT_STATE.md`：项目当前事实状态，迁移机器时最重要。
- `context/CONFIG_REGISTRY.md`：采纳配置、检测命令、依赖、替代方案、未记录配置和整体最佳方案。
- `context/PLAN_CURRENT.md`：当前实施方案版本。
- `context/RUNBOOK.md`：可复用操作流程和命令骨架。
- `context/DOCS_INDEX.md`：官方文档入口、已提取事实和查证日期。
- `context/SESSION_HANDOFF.md`：丢失任务或换机器时的恢复包。
- `tasks/*.md`：分阶段任务书。每次只推进一个任务。
- `inventory/`：设备探测输出、配置快照、前后对比。
- `backups/`：导出的配置、设备备份、项目 bundle。
- `scripts/`：仅放幂等、低风险、可审阅的辅助脚本，尽量同时提供 Bash 和 PowerShell 版本。

## 迁移建议

1. 优先通过 GitHub 保存并迁移当前工作；换开发机前要求 Codex 刷新 `PROJECT_STATE.md`、`PLAN_CURRENT.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。
2. 如果用户明确说明“当前任务需要保存、下次换开发机继续”，Codex 应先更新交接上下文，再检查 `git status`、敏感信息和提交范围，最后提交并推送到 GitHub（remote/auth 可用时）。
3. 不要把私钥、真实密码、token、Wi-Fi 密码写入仓库。
4. 新开发机 clone/pull 后，先阅读 `AGENTS.md`、`context/SESSION_HANDOFF.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/RUNBOOK.md`，再重新做 Host ADB 工具检查和 USB/ADB 设备扫描。
5. 如需离线备用，再生成 `git bundle` 与压缩包。

## 配置登记

所有配置类任务都应先查 `context/CONFIG_REGISTRY.md`。如果现场配置和记录不一致，先登记证据和影响范围，不直接覆盖；如果某个问题已有多种方案，把采纳方案、未采纳方案、取舍原因和当前整体最佳方案集中维护在该文件中。
