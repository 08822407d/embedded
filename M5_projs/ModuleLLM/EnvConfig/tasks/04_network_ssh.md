# 04 Network and SSH — 网络与 SSH 验证

状态：未开始  
风险等级：只读到中风险

## 目标

确认设备联网方式和 SSH 访问路径；必要时规划改密和 SSH 配置备份。

## 前置条件

设备已通过 ADB/UART 登录；RJ45 或 USB 网络可用。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## 步骤

1. 设备端 `ip addr`、`ip route`、DNS 检查。
2. Host 端 ping/ssh 验证。
3. 如需要改 SSH 或密码，先备份配置，列计划，等待确认。
4. 不把新密码写入任何文件。

## 验收标准

SSH 可用或不可用原因明确；网络状态记录。

## 输出位置

`inventory/before/network.txt`, `context/PROJECT_STATE.md`
