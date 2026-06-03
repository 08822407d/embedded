# scripts 使用说明

这些脚本只做主机检查、ADB 设备扫描、只读探测和 `/etc/apt` 备份。默认不修改 Module LLM 配置。

## Linux

```bash
bash scripts/host_check.sh
bash scripts/device_probe_adb.sh
bash scripts/pull_apt_backup_adb.sh
```

持续读取 Module LLM SoC 温度：

```bash
bash scripts/read_soc_temp_adb.sh
```

默认每 5 秒持续读取一次。测试固定次数：

```bash
COUNT=3 INTERVAL=5 bash scripts/read_soc_temp_adb.sh
```

按 SoC 温度控制 Fan Module v1.1：

```bash
COUNT=3 INTERVAL=5 bash scripts/fan_temp_control_adb.sh
```

该脚本先探测 `/dev/i2c-*` 上的 Fan 默认地址 `0x18`；未检测到时会退出且不写风扇寄存器。检测到后按近似 Raspberry Pi 5 的温度曲线写 Fan v1.1 I2C 寄存器：低于 50C 关闭，50C/60C/67.5C/75C 对应 30/50/70/100% 占空比，并使用 5C 回差。

启用 M5-Bus UART `/dev/ttyS1` 实体终端登录：

```bash
bash scripts/enable_m5bus_uart_login_adb.sh <serial>
APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>
```

当前配置会 mask `llm-sys.service` 以释放 `/dev/ttyS1` 并阻止它开机被其他 `llm-*` 服务依赖拉起，然后启用 `serial-getty@ttyS1.service`。登录方式为 `--autologin root`，外部串口终端接入后不需要账号/密码。回滚到 M5-Bus StackFlow/JSON 通信：

```bash
MODE=rollback APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>
```

若 `adb devices -l` 显示 M5Stack/AXERA 设备 `no permissions`，先运行 dry-run：

```bash
bash scripts/fix_m5stack_adb_udev.sh
```

确认只匹配 `32c9:2003` 后，可在 Host 侧授权应用：

```bash
APPLY=1 bash scripts/fix_m5stack_adb_udev.sh
adb kill-server
adb start-server
adb devices -l
```

该脚本只修改 Linux Host 侧 udev/USB 节点权限，不修改 Module LLM 设备系统。

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
