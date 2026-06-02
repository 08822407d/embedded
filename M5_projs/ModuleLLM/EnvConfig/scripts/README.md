# scripts 使用说明

这些脚本只做主机检查、ADB 设备扫描、只读探测和 `/etc/apt` 备份。默认不修改 Module LLM 配置。

## Linux

```bash
bash scripts/host_check.sh
bash scripts/device_probe_adb.sh
bash scripts/pull_apt_backup_adb.sh
```

多个 ADB 设备时：

```bash
ADB_SERIAL=<serial-from-adb-devices> bash scripts/device_probe_adb.sh
ADB_SERIAL=<serial-from-adb-devices> bash scripts/pull_apt_backup_adb.sh
```

也可以把 serial 作为第一个参数传入：

```bash
bash scripts/device_probe_adb.sh <serial>
```

## Windows 10 PowerShell

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\host_check.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\pull_apt_backup_adb.ps1
```

多个 ADB 设备时：

```powershell
$env:ADB_SERIAL="<serial-from-adb-devices>"
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
```

或：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1 -Serial <serial>
```

## ADB 工具路径

推荐环境变量为 `ADB_BIN`。如果 `adb` 不在 PATH，可以指定：

Linux：

```bash
ADB_BIN=/path/to/platform-tools/adb bash scripts/host_check.sh
```

Windows PowerShell：

```powershell
$env:ADB_BIN="C:\path\to\platform-tools\adb.exe"
powershell -ExecutionPolicy Bypass -File .\scripts\host_check.ps1
```

规则：

- 新主机至少先运行 Host ADB 工具检查和 USB/ADB 设备扫描。
- 如果脚本没有发现 `adb`，先询问用户是否安装，不要直接进入设备操作。
- 如果脚本在常见位置发现已安装的 `adb`，但 PATH/`ADB_BIN` 未配置，应使用 `ADB_BIN` 指向该绝对路径后继续。
- 配置正确后记录 `adb version`，再按可信来源检查/更新 Platform Tools。
