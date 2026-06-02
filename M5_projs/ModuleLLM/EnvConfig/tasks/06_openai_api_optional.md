# 06 Optional OpenAI API — 可选标准 API 插件

状态：未开始  
风险等级：中风险到高风险

## 目标

仅在用户需要时，安装并验证 `llm-openai-api` 相关包。

## 前置条件

apt 源可用；空间充足；用户明确需要 OpenAI API 兼容访问。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## 步骤

1. 说明将安装的包和空间影响。
2. 安装前记录 `df -h`、已有 `llm-*` 包。
3. 执行官方依赖包安装。
4. 重启前等待用户确认。
5. 重启后验证服务。

## 验收标准

API 服务状态明确；测试方式记录；未泄露任何密钥。

## 输出位置

`inventory/after/openai-api.txt`, `context/PROJECT_STATE.md`
