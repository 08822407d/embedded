# 08 Handoff and Backup — 迁移与恢复包

状态：未开始  
风险等级：低风险

## 目标

生成可迁移项目状态，支持换机器继续。

当用户明确说明当前开发机任务需要保存、下次任务更换开发机、要在另一台开发机继续时，本任务视为 GitHub 交接保存任务：需要先刷新所有续接上下文，再提交并推送到 GitHub（若 remote/auth 可用）。

## 前置条件

项目目录已有最新状态文件。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## 步骤

1. 更新 `SESSION_HANDOFF.md`。
2. 更新 `PROJECT_STATE.md`、`PLAN_CURRENT.md`、`CONFIG_REGISTRY.md`（如本轮涉及配置）。
3. 更新 `CHANGELOG.md`。
4. 检查仓库中无密码/token/私钥/Wi-Fi 密码。
5. 执行 `git status --short`，区分本项目续接文件、设备证据输出和无关改动。
6. 若存在无关改动或不确定文件，先让用户确认提交范围，不擅自 revert。
7. 生成 git commit。
8. 若 GitHub remote 和认证可用，推送当前分支到 GitHub。
9. 如需离线备用，再生成 `git bundle` 和压缩包。

## 验收标准

- 新机器可通过 GitHub clone/pull 获得完整项目内容。
- 新机器可读取 `SESSION_HANDOFF.md` 恢复任务。
- `CHANGELOG.md` 明确记录本轮保存范围、commit/push 状态和任何阻塞原因。

## 输出位置

`backups/*.bundle`, `backups/*.tgz`, `context/SESSION_HANDOFF.md`
