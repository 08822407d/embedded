# 01 Docs Ingest — 官方文档复核

状态：未开始  
风险等级：只读 / 联网阅读

## 目标

重新查证 M5Stack 与 Codex 官方文档，更新文档索引。

## 前置条件

主机可联网，或用户提供离线文档副本。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## 步骤

1. 打开 `context/DOCS_INDEX.md` 的官方链接。
2. 核对 Module LLM Kit 产品规格、连接方式、apt 源、刷机警告。
3. 核对 Codex AGENTS.md 发现规则和大小限制。
4. 只更新摘录，不复制整篇文档。

## 验收标准

`DOCS_INDEX.md` 有查证日期；高风险规则与官方文档一致。

## 输出位置

`context/DOCS_INDEX.md`, `context/CHANGELOG.md`
