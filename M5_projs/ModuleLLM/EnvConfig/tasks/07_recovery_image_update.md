# 07 Recovery Image Update — 镜像底包更新预案

状态：未开始  
风险等级：高风险 / 默认不执行

## 目标

仅制定恢复/刷机预案，不默认刷机。

## 前置条件

设备系统损坏或用户明确要求整机升级；Windows 烧录环境可用。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## 步骤

1. 阅读官方镜像底包更新文档。
2. 列出固件版本、烧录工具、驱动、进入下载模式步骤。
3. 再次强调禁止分区 `/dev/mmcblk0`。
4. 只有用户明确确认后才进入执行。

## 验收标准

有恢复预案；未执行刷机，除非用户逐条确认。

## 输出位置

`notes/recovery-plan.md`, `context/PROJECT_STATE.md`
