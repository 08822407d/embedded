# 03 Device Baseline Backup — 设备只读探测和备份

状态：未开始  
风险等级：只读为主；pull 文件低风险

## 目标

在任何修改前保存设备系统、网络、磁盘、apt、llm 包状态。  
通过 ADB 执行时，必须先扫描并选定当前目标设备，不能假设 USB 端口或 ADB serial 固定。

## 前置条件

- `tasks/02_host_setup.md` 已完成。
- `adb version` 成功。
- `adb devices -l` 至少显示一个 `device` 状态设备。
- 若多个设备，用户已明确指定 `ADB_SERIAL`。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 通过 ADB 时必须先执行 `adb devices -l` 并保存输出。
- 多个 ADB 设备时必须使用 `adb -s <serial>` 或脚本的 `ADB_SERIAL`。
- 不确定时只读探测，不猜测。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## Linux 步骤

Host：

```bash
bash scripts/device_probe_adb.sh
bash scripts/pull_apt_backup_adb.sh
```

多个设备时：

```bash
ADB_SERIAL=<serial> bash scripts/device_probe_adb.sh
ADB_SERIAL=<serial> bash scripts/pull_apt_backup_adb.sh
```

## Windows 10 PowerShell 步骤

Host：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\pull_apt_backup_adb.ps1
```

多个设备时：

```powershell
$env:ADB_SERIAL="<serial>"
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\pull_apt_backup_adb.ps1
```

## 手工步骤

1. Host 执行 `adb devices -l`。
2. 选定 serial。
3. 优先通过 ADB shell 执行只读探测。
4. 保存 `uname`、`os-release`、`df`、`ip`、`apt`、`dpkg` 输出。
5. 备份 `/etc/apt` 到 `backups/`。
6. 不修改设备。

## 验收标准

`inventory/before/` 和 `backups/` 中有 baseline；`PROJECT_STATE.md` 更新本次 serial、设备系统摘要、输出文件路径。

## 输出位置

`inventory/before/*`, `backups/*`, `context/PROJECT_STATE.md`
