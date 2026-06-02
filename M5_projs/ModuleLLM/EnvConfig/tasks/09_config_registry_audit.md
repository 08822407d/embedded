# 09 Config Registry Audit — 配置登记审计与整体方案整理

状态：未开始  
风险等级：只读为主；文档更新低风险

## 目标

检查连接的 Module LLM 是否满足 `context/CONFIG_REGISTRY.md` 中已采纳的配置要求；发现缺失、漂移或未记录配置时，按规则登记证据、依赖和处理方案。  
在需求变化后，基于当前采纳方案和替代方案，整理一套阶段性“整体最佳方案”。

## 前置条件

- 已阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/RUNBOOK.md`。
- 若接触设备，必须完成 ADB preflight 并选定当前 serial。

## Codex 执行要求

- 明确标注命令运行端：Host / ADB shell / UART shell / SSH shell。
- 默认只读检测，不修改设备。
- 中风险或高风险配置动作必须先列计划并等待用户确认。
- 不把真实密码、token、私钥、Wi-Fi 密码写入仓库。

## 步骤

1. 列出本轮涉及的配置项 ID。
2. 逐项运行检测命令并保存输出。
3. 标记状态：`compliant`、`missing`、`drift`、`unrecorded`、`unknown`、`deferred`。
4. 对 `missing` / `drift` 项列依赖关系、正确配置步骤、验证命令和回滚方法。
5. 对 `unrecorded` 项记录证据、来源推断、风险和建议处理方式。
6. 若有多个方案，更新替代方案与建议。
7. 根据当前采纳方案和替代方案，必要时更新“当前整体最佳方案”。

## 验收标准

- `CONFIG_REGISTRY.md` 反映最新采纳配置、检测方法、依赖关系和方案取舍。
- `PROJECT_STATE.md`、`CHANGELOG.md`、`SESSION_HANDOFF.md` 已同步关键结论。
- 所有设备侧输出都能在 `inventory/` 或 `backups/` 中追溯。

## 输出位置

`context/CONFIG_REGISTRY.md`, `context/PROJECT_STATE.md`, `context/CHANGELOG.md`, `context/SESSION_HANDOFF.md`, `inventory/*`
