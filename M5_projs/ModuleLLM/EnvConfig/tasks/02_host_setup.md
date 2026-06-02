# 02 Host Setup — 主机工具与 ADB 预检查

状态：未开始  
风险等级：只读 / 本机低风险

## 目标

确认当前主机 OS、shell、adb、ssh、串口工具可用；通过 `adb devices -l` 扫描当前 USB-C 数据线连接的设备。  
这是所有 ADB 操作的前置门禁。

在任何新主机/新环境中，至少先完成两项检查任务：

1. Host ADB 工具检查。
2. USB/ADB 设备扫描。

## 前置条件

主机已安装或准备安装 platform-tools/adb、ssh 客户端、串口工具。  
若未安装，Codex 只能给安装建议，不能擅自安装系统软件，除非用户确认。

## Codex 执行要求

- 先阅读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/PLAN_CURRENT.md`、`context/RUNBOOK.md`。
- 明确标注每条命令运行端：Host / ADB shell / UART shell / SSH shell。
- 先判断当前主机是 Windows 10 还是 Linux，并选择对应脚本。
- 若未发现 `adb`，必须停止并询问用户是否安装，不能擅自安装。
- 若发现已安装的 `adb` 但 PATH 未配置，优先设置当前会话 `ADB_BIN=<adb 绝对路径>` 后继续；持久写入环境变量前必须等用户确认。
- 必须确认 `adb version` 成功后，才允许进入设备探测任务。
- 配置正确后必须记录 `adb version`，并按可信来源检查/更新 Host 侧 Android SDK Platform Tools。
- 必须执行 `adb start-server` 和 `adb devices -l`。
- 多个 ADB 设备时，停止并要求用户选择 serial；不要裸执行 `adb shell/push/pull`。
- 会修改设备的命令必须先给计划并等待用户确认。
- 每轮结束更新 `PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`。

## Linux 步骤

Host：

```bash
bash scripts/host_check.sh
```

或手工：

```bash
uname -a
command -v adb || true
adb version
adb start-server
adb devices -l
ssh -V
python3 --version || true
lsusb || true
```

## Windows 10 PowerShell 步骤

Host：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\host_check.ps1
```

或手工：

```powershell
$PSVersionTable
Get-Command adb -ErrorAction SilentlyContinue
adb version
adb start-server
adb devices -l
ssh -V
Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match 'USB' } | Select-Object -First 30
```

## 判定

- `adb` 不存在：记录状态，询问用户是否安装 Android SDK Platform Tools，停止设备操作。
- `adb` 已安装但不在 PATH：设置当前会话推荐环境变量 `ADB_BIN=<adb 绝对路径>`，再执行 `adb version`。
- `adb version` 失败：记录错误，停止设备操作。
- `adb devices -l` 无设备：提示检查 Type-C 是否为数据线、是否接到 Module LLM 的 Type-C、是否需要驱动/权限，停止设备操作。
- 只有一个 `device` 状态设备：可进入 `tasks/03_device_baseline_backup.md`，但仍记录本次 serial。
- 多个 `device` 状态设备：要求用户指定 `ADB_SERIAL` 或脚本参数，不能自动选择。

## 验收标准

主机工具状态、ADB 版本、`adb devices -l` 输出路径写入 `PROJECT_STATE.md`。

## 输出位置

`inventory/before/host-tools-*.txt`, `context/PROJECT_STATE.md`
