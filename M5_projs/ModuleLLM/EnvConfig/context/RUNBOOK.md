# RUNBOOK — Module LLM Kit 安全配置操作手册

所有命令执行前必须确认运行端：Host / ADB shell / UART shell / SSH shell。  
Host 可能是 Windows 10，也可能是 Linux 发行版；每次会话都要重新确认。

## 1. 风险等级

- 只读：不会改变设备状态。例如 `uname -a`、`df -h`、`ip addr`。
- 低风险：复制文件、读取日志、创建本地目录。
- 中风险：修改 `/etc/apt`、安装/卸载包、重启服务。
- 高风险：重启设备、整机刷机、系统镜像更新、账户/SSH/网络核心配置。
- 禁止默认执行：对 `/dev/mmcblk0` 分区/格式化/写镜像。

## 1.1 配置登记与审计

所有配置类任务必须先查 `context/CONFIG_REGISTRY.md`。

固定流程：

1. 找到相关配置项 ID。
2. 运行登记的检测命令，输出保存到 `inventory/before/` 或 `inventory/after/`。
3. 判断状态：`compliant`、`missing`、`drift`、`unrecorded`、`unknown`、`deferred`。
4. 若缺失，按依赖关系和正确配置步骤推进。
5. 若发现未记录配置，不覆盖；先登记证据和影响范围。
6. 若有多个方案，查替代方案表；改变采纳方案前记录取舍和回滚路径。
7. 任务结束时更新 `CONFIG_REGISTRY.md`、`PROJECT_STATE.md`、`CHANGELOG.md`、`SESSION_HANDOFF.md`。

## 1.2 跨开发机 GitHub 交接

触发条件：用户明确说明当前开发机任务需要保存、下次任务更换开发机、要在另一台机器继续，或等价表达。

解释：这表示用户要通过 GitHub 保存当前工作，并在下一台开发机拉取完整项目继续。不要只回复聊天总结。

必须先更新：

- `context/PROJECT_STATE.md`：当前阶段、事实、风险、输出路径、下一步。
- `context/PLAN_CURRENT.md`：实施方案和依赖关系。
- `context/SESSION_HANDOFF.md`：新开发机恢复步骤，必须可独立阅读。
- `context/CHANGELOG.md`：本轮动作和证据文件。
- `context/CONFIG_REGISTRY.md`：若本轮涉及配置。

GitHub 保存前检查：

```bash
git status --short
git remote -v
```

Windows PowerShell 同样使用：

```powershell
git status --short
git remote -v
```

处理规则：

1. 不提交真实密码、token、私钥、Wi-Fi 密码。
2. 不把其他项目或无关工作树改动混入本项目交接提交。
3. 若看到无关改动，列出并让用户确认范围；不要擅自 revert。
4. 若 remote/auth 可用，按用户“保存/换机继续”的明确意图提交并推送。
5. 若不能推送，说明原因，并确保本地 `SESSION_HANDOFF.md` 已足够恢复。
6. 最终回复要写明新开发机续接步骤：clone/pull 后先读 `AGENTS.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/RUNBOOK.md`、`context/SESSION_HANDOFF.md`，再执行 Host ADB 工具检查和 USB/ADB 设备扫描。

## 2. 每次会话的 Host / ADB 门禁

目标：先确认当前主机和当前连接的 USB/ADB 设备。不要沿用上次端口或 serial。

最低前置检查任务：

1. Host ADB 工具检查：确认 `adb` 是否在 PATH、`ADB_BIN` 或常见安装路径中可用。
2. USB/ADB 设备扫描：确认 USB/PnP/`adb devices -l` 当前看到的设备。

判定规则：

- 未发现 `adb`：记录状态，停止设备操作，并询问用户是否安装 Android SDK Platform Tools。
- 已发现 `adb` 但环境变量未配置：设置当前会话推荐环境变量 `ADB_BIN=<adb 绝对路径>` 后继续；持久写入用户/系统环境变量前需用户确认。
- `adb` 配置正确后：执行 `adb version`，记录版本；按可信来源检查/更新 Host 侧 Platform Tools，优先使用系统可信包管理器或 Android 官方 ZIP。
- 更新或安装 `adb` 只属于 Host 侧动作；仍不得跳过后续 `adb start-server` 和 `adb devices -l`。

### Linux Host

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

也可直接运行：

```bash
bash scripts/host_check.sh
```

### Windows 10 Host / PowerShell

```powershell
$PSVersionTable
Get-Command adb -ErrorAction SilentlyContinue
adb version
adb start-server
adb devices -l
ssh -V
Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match 'USB' } | Select-Object -First 30
```

也可直接运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\host_check.ps1
```

验收：

- `adb version` 能正常输出。
- `adb devices -l` 能运行。
- 至少一个目标设备显示为 `device` 状态；若没有，则停止。
- 多个设备时必须显式选择 serial。

Linux 若出现 `no permissions`：

```bash
id
lsusb -d 32c9:2003
adb devices -l
stat -c '%A %U %G %a %n' /dev/bus/usb/<BUS>/<DEV>
grep -RIn '32c9\|axera\|2003' /etc/udev/rules.d /usr/lib/udev/rules.d
```

本项目为 M5Stack/AXERA `32c9:2003` ADB 设备准备了 Host 侧修复脚本：

```bash
bash scripts/fix_m5stack_adb_udev.sh
APPLY=1 bash scripts/fix_m5stack_adb_udev.sh
adb kill-server
adb start-server
adb devices -l
```

该脚本默认 dry-run；`APPLY=1` 会写入 Host 侧 `/etc/udev/rules.d/51-m5stack-axera-adb.rules`，需要 sudo。它不修改 Module LLM 设备系统。

## 3. ADB 设备选择规则

### 单设备

若 `adb devices -l` 只有一个 `device` 状态设备，可以用该 serial 执行命令，但仍要把 serial 写入输出文件。

### 多设备

禁止直接执行：

```bash
adb shell
adb push local remote
adb pull remote local
```

必须改为：

```bash
adb -s <SERIAL> shell
adb -s <SERIAL> push local remote
adb -s <SERIAL> pull remote local
```

脚本调用：

Linux：

```bash
ADB_SERIAL=<SERIAL> bash scripts/device_probe_adb.sh
ADB_SERIAL=<SERIAL> bash scripts/pull_apt_backup_adb.sh
```

Windows PowerShell：

```powershell
$env:ADB_SERIAL="<SERIAL>"
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\pull_apt_backup_adb.ps1
```

## 4. ADB 接入

Host：

```bash
adb devices -l
adb -s <SERIAL> shell
```

如果 `adb` 已安装但不在 PATH，使用推荐环境变量：

Linux：

```bash
export ADB_BIN=/path/to/platform-tools/adb
$ADB_BIN devices -l
```

Windows PowerShell：

```powershell
$env:ADB_BIN="C:\path\to\platform-tools\adb.exe"
& $env:ADB_BIN devices -l
```

文件推送示例：

```bash
adb -s <SERIAL> push local-file /opt/
```

若出现权限/连接异常，先不要改设备。可重启 Host 侧 ADB server：

```bash
adb kill-server
adb start-server
adb devices -l
```

## 5. UART 接入

用户描述串口终端任务时，以应用场景为准：目标通常是让外部带屏幕/键盘的设备通过 UART 获得 Module LLM 的 Linux 登录 shell，而不是配置 Linux 本机 `tty1` 虚拟控制台。

参数：

```text
ttyS0 / DBG 调试登录：115200 8N1
ttyS1 / M5-Bus Tab5 登录：921600 8N1，xterm-256color，truecolor，32x64
用户：root
默认密码：123456
```

官方默认密码信息只用于需要密码登录的路径；不要把新密码写入仓库。

当前现场状态（2026-06-12）：

- `ttyS0` / `DBG_TXD/DBG_RXD` 保留为默认系统 Log/调试登录路径。
- M5-Bus UART `/dev/ttyS1` 已按用户目标改作 `921600 8N1` 的额外运行期实体终端登录口；`agetty` 类型为 `xterm-256color`。
- `ttyS1` 当前使用 `--autologin root`，不需要输入账号/密码；外部终端接入后应直接得到 root shell。
- `/etc/profile.d/m5bus-ttyS1-tab5.sh` 仅在控制终端为 `/dev/ttyS1` 时设置 `TERM=xterm-256color`、`COLORTERM=truecolor` 和 `stty rows 32 cols 64`。
- 为释放 `ttyS1`，`llm-sys.service` 已 mask/inactive；M5-Bus StackFlow/JSON API 通信不可用，回滚见 `scripts/enable_m5bus_uart_login_adb.sh`。

针对“外部独立终端设备”的检查顺序：

1. 确认 Module LLM 侧登录 UART：当前采纳配置为 `ttyS0` / `DBG_TXD/DBG_RXD` / 系统 Log 调试路径。
2. 确认外部终端板侧 UART：连接 ttyS1 时配置 `921600 8N1`、`xterm-256color`、32 行 64 列；3.3V TTL、电平兼容、TX/RX 交叉、公共 GND、无硬件流控。
3. 确认外部终端板的软件：能运行串口终端程序，把键盘输入发到 UART，把 UART 输出显示到屏幕。
4. 确认 Module LLM 侧服务：`console=ttyS0,115200n8`、`serial-getty@ttyS0.service` active/running。
5. 若目标是 M5-Bus UART 实体终端，查 `CFG-M5BUS-UART-LOGIN-001`，确认 `serial-getty@ttyS1.service` active/running 且 `llm-sys.service` masked/inactive。
6. 若用户想把 M5-Bus UART 改成账号/密码登录，不要记录密码；先把 ttyS1 drop-in 中的 `--autologin root` 去掉，再验证 root 密码和恢复路径。

ttyS1 / Tab5 验证：

```sh
echo "$TERM"       # xterm-256color
echo "$COLORTERM"  # truecolor
stty size          # 32 64
tput colors        # 256
htop
```

若通过 ADB 审计，必须同时确认 ttyS0 仍为 115200 且非 ttyS1 的 `/dev/pts/*` 会话 source `/etc/profile.d/m5bus-ttyS1-tab5.sh` 后环境不变。

## 6. SSH 接入

先在设备端确认 IP。

ADB shell / UART shell：

```sh
ip addr
ip route
```

Host：

```bash
ssh root@<DEVICE_IP>
```

## 7. 设备只读探测

推荐先通过脚本完成，因为脚本会强制做 ADB preflight 和目标 serial 选择。

Linux：

```bash
bash scripts/device_probe_adb.sh
```

Windows PowerShell：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\device_probe_adb.ps1
```

ADB shell / SSH shell 原始命令骨架：

```sh
set -eu
date
hostname || true
uname -a
cat /etc/os-release || true
df -h
free -h || true
ip addr
ip route || true
mount || true
lsblk || true
cat /proc/cmdline || true
apt list --installed 2>/dev/null | grep -E '(^lib-llm|^llm-)' || true
dpkg -l | grep -E ' lib-llm| llm-' || true
ls -la /opt || true
find /etc/apt -maxdepth 2 -type f -print -exec sed -n '1,120p' {} \; || true
```

## 8. Baseline 备份

Host 创建目录：

```bash
mkdir -p inventory/before backups
```

Windows PowerShell：

```powershell
New-Item -ItemType Directory -Force inventory\before, backups | Out-Null
```

ADB 备份脚本：

Linux：

```bash
bash scripts/pull_apt_backup_adb.sh
```

Windows PowerShell：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\pull_apt_backup_adb.ps1
```

手工命令示例，必须带 `-s <SERIAL>`：

```bash
adb -s <SERIAL> shell 'uname -a; cat /etc/os-release; df -h; ip addr; ip route' > inventory/before/device-basic.txt
adb -s <SERIAL> shell 'dpkg -l | grep -E " lib-llm| llm-" || true' > inventory/before/llm-packages.txt
adb -s <SERIAL> shell 'tar -czf /tmp/etc-apt-backup.tgz /etc/apt 2>/dev/null || true'
adb -s <SERIAL> pull /tmp/etc-apt-backup.tgz backups/
adb -s <SERIAL> shell 'rm -f /tmp/etc-apt-backup.tgz'
```

## 9. 配置 M5Stack apt 源

前提：网络可用，已备份 `/etc/apt`。  
这属于中风险操作，必须先让用户确认。

ADB shell / SSH shell：

```sh
set -eu
install -d -m 0755 /etc/apt/keyrings
cp -a /etc/apt "/tmp/etc-apt-before-$(date +%Y%m%d-%H%M%S)"
wget -qO /etc/apt/keyrings/StackFlow.gpg https://repo.llm.m5stack.com/m5stack-apt-repo/key/StackFlow.gpg
printf '%s\n' 'deb [arch=arm64 signed-by=/etc/apt/keyrings/StackFlow.gpg] https://repo.llm.m5stack.com/m5stack-apt-repo jammy ax630c' > /etc/apt/sources.list.d/StackFlow.list
apt update
apt list | grep llm
```

安装最小基础包前先检查空间：

```sh
df -h
apt install lib-llm llm-sys
```

## 10. 可选 OpenAI API 插件

仅在用户确认需要时执行。

ADB shell / SSH shell：

```sh
apt update
apt install lib-llm
apt install llm-sys llm-llm llm-vlm llm-whisper llm-melotts llm-openai-api
reboot
```

重启后重新通过 ADB/UART/SSH 验证。重启后 USB/ADB 设备必须重新扫描，不能沿用旧 serial。

## 11. 严禁事项复核

不要执行：

```sh
fdisk /dev/mmcblk0
parted /dev/mmcblk0
mkfs.* /dev/mmcblk0*
dd if=* of=/dev/mmcblk0*
```

若必须做镜像底包更新，另开高风险任务，使用官方 Windows 烧录工具路径，并先写恢复计划。

## 12. SoC 温度读取

风险等级：只读。

当前已验证温度节点：

```text
/sys/class/thermal/thermal_zone0/type = soc_thm
/sys/class/thermal/thermal_zone0/temp = 毫摄氏度
```

Host 侧通过 ADB 持续读取：

```bash
bash scripts/read_soc_temp_adb.sh
```

默认每 5 秒读取一次并持续运行。短测三次：

```bash
COUNT=3 INTERVAL=5 bash scripts/read_soc_temp_adb.sh
```

输出示例：

```text
2023-08-22 06:00:06 zone=/sys/class/thermal/thermal_zone0 type=soc_thm raw=42350 temp=42.350 C
```
