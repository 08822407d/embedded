# 05 APT StackFlow — M5Stack apt 源与基础包

状态：未开始  
风险等级：中风险

## 目标

按官方路径配置 M5Stack apt 源，更新索引，按需安装最小基础包。

## 前置条件

网络可用；`/etc/apt` 已备份；用户确认允许修改 apt 源。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## 步骤

1. 备份 `/etc/apt`。
2. 添加 StackFlow GPG key 和 apt source。
3. 执行 `apt update`。
4. 保存 `apt list | grep llm`。
5. 安装 `lib-llm llm-sys` 前再次确认。

## 验收标准

apt update 成功；可用包列表记录；安装动作有前后状态。

## 输出位置

`inventory/after/apt-update.txt`, `inventory/after/llm-available.txt`, `context/PROJECT_STATE.md`
