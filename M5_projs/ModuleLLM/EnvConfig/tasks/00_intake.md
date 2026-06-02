# 00 Intake — 需求与环境盘点

状态：未开始  
风险等级：只读 / 不接触设备

## 目标

确认用户目标、当前主机环境、连接方式、是否允许联网查文档、是否允许 Codex 执行本地命令。  
本任务必须兼容 Windows 10 与 Linux 发行版，且不能假设本次 USB 端口、串口名或 ADB serial 与上次相同。

## 前置条件

无。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 先识别当前 Host OS 与 shell，再选择 Bash 或 PowerShell 命令。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## 步骤

1. 确认当前主机：Windows 10 / Linux 发行版、shell、是否允许运行本地只读命令。
2. 确认 ADB 预期安装方式：已在 PATH 中、Android SDK Platform Tools 目录、还是尚未安装。
3. 确认用户当前拥有的硬件：Module LLM、Module13.2 LLM Mate、Type-C 数据线、网线、串口/调试板。
4. 确认本阶段目标：只读探测、apt 更新、OpenAI API 插件、SSH 配置、网络配置等。
5. 明确不会执行任何设备修改。
6. 下一步转入 `tasks/02_host_setup.md`，先验证 ADB 是否已安装并能正常工作。

## 验收标准

`PROJECT_STATE.md` 已填入本次主机环境、用户目标、ADB 安装状态未知/待查；下一任务明确。

## 输出位置

`context/PROJECT_STATE.md`, `context/CHANGELOG.md`
