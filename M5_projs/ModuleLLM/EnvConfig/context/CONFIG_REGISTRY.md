# CONFIG_REGISTRY — 配置登记、检测与方案台账

最后更新：2026-06-02  
用途：集中记录本项目已采纳配置、检测方法、配置依赖、修复步骤、替代方案和阶段性最佳方案，避免每轮任务重复联网查找原因和方案。

## 1. 使用规则

每个 Codex 任务在接触设备或修改项目配置前，都应先检查本文件中相关条目。

### 1.1 每轮最小流程

1. 识别当前任务涉及的配置项 ID。
2. 先运行该配置项的“检测命令”，并把输出保存到 `inventory/before/` 或 `inventory/after/`。
3. 对照“合规判定”确认状态：`compliant`、`missing`、`drift`、`unrecorded`、`unknown`。
4. 若配置缺失，按“依赖关系”和“正确配置步骤”推进；中风险或高风险步骤必须等待用户确认。
5. 若检测到未记录配置，不直接覆盖；先记录到“未记录配置观察”，判断它是用户手工配置、官方镜像默认、临时状态，还是异常漂移。
6. 若存在多个方案，先查“替代方案与建议”；若需要改变采纳方案，记录原因、取舍、回滚路径和新的验证结果。
7. 任务结束时更新本文件、`PROJECT_STATE.md`、`CHANGELOG.md`、`SESSION_HANDOFF.md`。

### 1.2 状态定义

- `compliant`：现场状态与采纳配置一致。
- `missing`：目标配置未完成。
- `drift`：目标配置存在但与采纳记录不一致。
- `unrecorded`：现场存在未登记配置，可能是用户手工修改或官方默认。
- `unknown`：检测不足，不能判断。
- `deferred`：已讨论但当前不实施。

### 1.3 未记录配置处理

发现未登记配置时：

1. 只读记录证据：命令、输出路径、文件 hash 或关键摘要。
2. 不删除、不覆盖、不“修正”。
3. 判断影响范围：Host / ADB / UART / SSH / 网络 / apt / 包 / 服务 / 系统启动。
4. 若该配置有用且安全，建议登记为采纳配置或已知现场事实。
5. 若该配置有风险，列出备份点、回滚方案和替代方案，等待用户确认。

## 2. 配置依赖图

当前整体顺序：

```text
Host ADB 工具
  -> USB/ADB 设备扫描
  -> ADB 目标 serial 选择
  -> 设备只读 baseline
  -> 串口登录路径确认（独立审计项，不阻塞 apt）
  -> 网络路径确认
  -> /etc/apt 备份
  -> M5Stack apt 源
  -> 基础 llm 包
  -> 可选 OpenAI API 插件
  -> 迁移/恢复包
```

关键约束：

- `apt update/install` 依赖网络可用和 `/etc/apt` 已备份。
- SSH/密码/网络配置变更依赖对应配置文件备份。
- 串口登录路径变更涉及 systemd getty、kernel bootargs、设备树/pinmux 或硬件 Net-Tie；默认只确认，不重配置。
- 任何换线、换 USB 口、设备重启、换主机后，ADB serial 必须重新扫描。
- 镜像底包更新独立为高风险恢复路径，不是常规配置依赖链的一部分。

## 3. 当前采纳配置项

### CFG-HOST-ADB-001 — Host 侧 ADB 工具

状态：`compliant`（本次 Windows 主机）

采纳配置：

- Windows 当前使用 Android 官方 Platform Tools。
- 当前路径：`C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe`
- 未写入全局 PATH；推荐当前会话使用 `ADB_BIN`。

检测命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\host_check.ps1
```

合规判定：

- `adb version` 成功。
- `adb devices -l` 能运行。
- 输出保存到 `inventory/before/host-tools-*.txt`。

缺失时正确处理：

- 未发现 `adb`：停止，询问用户是否安装。
- 已安装但 PATH 未配置：设置 `$env:ADB_BIN="<adb.exe 绝对路径>"` 后继续。
- 安装优先级：可信系统包管理器或 Android 官方 ZIP；安装记录必须保存下载来源、hash、签名和版本。

已知证据：

- `inventory/after/host-adb-install-20260602-163412.txt`
- `inventory/before/host-tools-20260602-164338.txt`

### CFG-ADB-TARGET-001 — ADB 目标设备选择

状态：`compliant`（本次连接）

采纳配置：

- 每轮使用前重新执行 `adb devices -l`。
- 多设备时必须指定 `ADB_SERIAL` 或 `adb -s <serial>`。
- 本次观察 serial：`axera-ax620e`，不可跨会话视为永久事实。

检测命令：

```powershell
$env:ADB_BIN="C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe"
& $env:ADB_BIN devices -l
```

合规判定：

- 若只有一个 `device` 状态设备，可继续但仍记录 serial。
- 若多个 `device` 状态设备，停止并要求用户选择。
- 若无 `device` 状态设备，停止设备操作。

已知证据：

- `inventory/before/host-tools-20260602-164709.txt`
- `inventory/before/adb-connect-readonly-20260602-164730.txt`

### CFG-SERIAL-LOGIN-001 — Linux 串口登录终端

状态：`compliant`（当前官方默认镜像；本轮只读验证）

用户意图解释：

- 用户目标不是研究 Linux 虚拟终端框架本身，而是让“带屏幕和键盘的外部嵌入式开发板”通过串口成为 Module LLM 的独立字符终端。
- 遇到用户说“串口登录”“终端”“纯字符界面”“init 3 那种界面”等表述时，应按该场景反推检查项：物理 UART 引脚、电气电平、串口参数、kernel console、systemd serial getty、登录认证、shell、业务串口占用和回滚路径。
- 不应把用户意图窄化为 Linux `tty1`/虚拟控制台，也不应默认把 M5-Bus 通信 UART 当成登录 UART。

采纳配置：

- 保留默认 Linux 登录串口：`ttyS0`，`115200n8`。
- `ttyS0` 同时是 kernel console 和 systemd serial getty。
- 当前运行时映射：`ttyS0 -> /soc/ax_uart@4880000`（`serial0`，MMIO `0x04880000`）；`ttyS1 -> /soc/ax_uart@4881000`（`serial1`），当前未绑定 getty。
- 硬件路径判断：`ttyS0`/`serial0` 对应官方系统 Log/调试串口路径，即 `DBG_TXD/DBG_RXD`；M5-Bus 上 `TRM_TXD/TRM_RXD` 是 StackFlow/JSON API 通信 UART，不作为默认 Linux 登录终端。

检测命令：

```sh
cat /proc/cmdline
cat /proc/consoles
cat /proc/tty/driver/serial
readlink -f /sys/class/tty/ttyS0/device
readlink -f /sys/class/tty/ttyS1/device
systemctl list-units --all "serial-getty*" "getty*" --no-pager
systemctl show serial-getty@ttyS0.service -p LoadState -p ActiveState -p SubState -p FragmentPath -p UnitFileState --no-pager
for p in /proc/device-tree/chosen/bootargs /proc/device-tree/aliases/serial0 /proc/device-tree/aliases/serial1; do [ -e "$p" ] && { echo "$p"; tr "\000" "\n" < "$p"; }; done
```

合规判定：

- `/proc/cmdline` 包含 `console=ttyS0,115200n8` 和 `earlycon=uart8250,mmio32,0x4880000`。
- `/proc/consoles` 显示 `ttyS0` 为内核 console。
- `serial-getty@ttyS0.service` 为 `active/running`。
- `/proc/device-tree/aliases/serial0` 指向 `/soc/ax_uart@4880000`。
- `/proc/tty/driver/serial` 中 `0:` 为 `uart:16550A mmio:0x04880000`。

正确使用步骤：

1. 优先使用 Module13.2 LLM Mate 或官方调试板的系统 Log/调试串口。
2. 串口参数：`115200 bps`、`8N1`、无硬件流控，按 Mate/调试接口确认 3.3V TTL 电平；连接 TX/RX 交叉和公共 GND，不能直接接 RS-232 电平。
3. 外部嵌入式开发板作为终端时，应把该板 UART 接到 Module LLM 的 `DBG_TXD/DBG_RXD` 路径，而不是 M5-Bus `TRM_TXD/TRM_RXD`。
4. 默认账户信息只按官方文档现场使用；修改密码前必须先制定备份和恢复方案，且不得把新密码写入仓库。

外部独立终端需要额外确认：

- 外部开发板自身需要有串口终端程序或等价登录终端应用，负责把键盘输入发到 UART、把 UART 输出显示到屏幕。
- 外部开发板的 UART 不能在启动日志、REPL、调试器或其他服务中被占用；其串口参数必须与 Module LLM 一致。
- 若希望看到 Module LLM 从上电开始的日志，必须接 `ttyS0`/DBG 路径；只额外启用某个非 console UART getty 通常看不到 early boot log。

可重配置边界：

- 低到中风险：在确认物理引脚和业务占用后，额外启用 `serial-getty@ttyS1.service` 作为第二登录串口；执行前必须备份 systemd 配置并确认不会占用 StackFlow 通信 UART。
- 高风险：把 kernel console/earlycon 从 `ttyS0` 改到其他 UART。该动作涉及 bootargs、bootloader env、dtb 或启动分区；错误配置可能丢失早期日志和串口登录恢复路径。
- 高风险或硬件改造：把 M5-Bus `TRM_TXD/TRM_RXD` 改作 Linux 登录串口。该路径默认属于主控通信 UART，且可能涉及设备树 pinmux、服务占用和 PCB Net-Tie/焊盘切换。

已知证据：

- `inventory/before/serial-login-adb-preflight-20260602-173920.txt`
- `inventory/before/serial-login-config-adb-20260602-174529.txt`
- `inventory/before/serial-login-devicetree-adb-20260602-174558.txt`
- `inventory/before/serial-uart-status-adb-20260602-174644.txt`

### CFG-DEVICE-NET-001 — 设备联网路径

状态：`missing`（当前无互联网）

采纳配置：

- 尚未采纳联网配置。
- 当前事实：设备无默认路由；`eth0` 存在但 `DOWN/NO-CARRIER`；未发现无线网卡或 USB Ethernet；USB gadget 仅 ADB function。

检测命令：

```sh
ip -br addr
ip route
ip route get 1.1.1.1
cat /etc/resolv.conf
find /sys/kernel/config/usb_gadget/usb_adb -maxdepth 4 -printf '%y %p -> %l\n'
```

合规判定：

- 有默认路由，并能访问 M5Stack apt repo，才可进入 apt 配置。
- 当前不合规：公网 IP 不可达，DNS 解析 M5Stack repo 失败。

正确配置步骤：

1. 首选方案：连接 Module13.2 LLM Mate / 扩展底座 RJ45。
2. 通过 ADB shell 只读确认 `eth0` 获得地址、默认路由和 DNS。
3. 保存网络检测输出到 `inventory/before/` 或 `inventory/after/`。
4. 网络可用后再进入 `/etc/apt` 备份和 apt 源配置。

替代方案：

- USB gadget Ethernet + Host NAT：可能可行，但会改变 USB gadget 和 Host 网络配置，属于中风险到高风险，需备份和确认。
- Host 侧代理/端口转发：当前 `adb reverse --list` 返回 `error: closed`，不能假设可用；需要单独研究和验证。
- 离线包传输：适合少量固定包，但依赖完整依赖解析，易漏包，不作为常规首选。

当前建议：

- 阶段性最佳方案是使用扩展底座 RJ45，让设备走正常 Linux 网络路径。

已知证据：

- `inventory/before/adb-network-capability-20260602-170131.txt`
- `inventory/before/usb-gadget-adb-functions-20260602-170200.txt`
- `inventory/before/device-internet-probe-adb-20260602-170221.txt`

### CFG-APT-001 — M5Stack StackFlow apt 源

状态：`missing`

采纳配置：

- 尚未修改设备 apt 配置。

依赖：

- `CFG-ADB-TARGET-001` compliant。
- `CFG-DEVICE-NET-001` compliant。
- `/etc/apt` 已备份。

检测命令：

```sh
find /etc/apt -maxdepth 2 -type f -print -exec sed -n '1,120p' {} \;
apt-cache policy 2>/dev/null | sed -n '1,120p'
```

正确配置步骤：

1. 备份 `/etc/apt`。
2. 添加 StackFlow GPG key。
3. 添加 StackFlow apt source。
4. 执行 `apt update`。
5. 保存 `apt update` 和 `apt list | grep llm` 输出。

风险：

- 修改 `/etc/apt` 属于中风险，必须等待用户确认。

### CFG-TOOLCHAIN-001 — C/C++ 基础工具链

状态：`compliant`

采纳配置：

- 设备保留当前已安装 C/C++ 基础工具链，不主动修改。

检测命令：

```sh
dpkg-query -W build-essential binutils binutils-aarch64-linux-gnu gcc g++ make libc6-dev dpkg-dev
command -v gcc g++ make ld as objdump readelf
gcc -dumpmachine
```

已知事实：

- `build-essential 12.9ubuntu3`
- `binutils 2.38-4ubuntu2.6`
- `gcc/g++ 11.4.0`
- 默认目标 `aarch64-linux-gnu`
- `/usr/bin/gdb` 可运行但不是 dpkg 管理包；`gdbserver` 未安装。

已知证据：

- `inventory/before/toolchain-check-adb-20260602-165625.txt`
- `inventory/before/gdb-origin-check-adb-20260602-165651.txt`

### CFG-OPENAI-API-001 — 可选 OpenAI API 插件

状态：`deferred`

采纳配置：

- 未启用。

依赖：

- `CFG-DEVICE-NET-001` compliant。
- `CFG-APT-001` compliant。
- 磁盘空间检查通过。
- 用户明确需要 OpenAI API 兼容访问。

检测命令：

```sh
dpkg -l | grep -E ' llm-openai-api| llm-llm| llm-vlm| llm-whisper| llm-melotts'
df -h
```

风险：

- 安装多个功能/模型包可能占用较多空间。
- 安装后可能需要重启设备；重启后必须重新扫描 ADB serial。

## 4. 未记录配置观察

| 日期 | 范围 | 观察 | 证据 | 处理状态 |
| --- | --- | --- | --- | --- |
| 2026-06-02 | Device gdb | `/usr/bin/gdb` 存在且可运行，但不是 dpkg 管理的 `gdb` 包 | `inventory/before/gdb-origin-check-adb-20260602-165651.txt` | 记录为现场事实；不修改 |

## 5. 替代方案与建议

### 网络方案

| 方案 | 状态 | 优点 | 代价/风险 | 当前建议 |
| --- | --- | --- | --- | --- |
| 扩展底座 RJ45 | 建议 | 官方/常规 Linux 网络路径，适合 apt 和 SSH | 需要底座和网线 | 当前最佳 |
| USB gadget Ethernet + Host NAT | 未采纳 | 不依赖 RJ45 | 需改 USB gadget 和 Host 网络，风险较高 | 暂不实施 |
| Host 代理/ADB 端口隧道 | 未采纳 | 理论上改动较少 | 当前 `adb reverse` 不可用；不等同通用互联网 | 仅单独研究 |
| 离线包传输 | 备用 | 无需设备联网 | 依赖解析复杂，维护成本高 | 仅应急 |

### ADB 工具方案

| 方案 | 状态 | 优点 | 代价/风险 | 当前建议 |
| --- | --- | --- | --- | --- |
| Android 官方 Platform Tools ZIP | 已采纳 | 来源清晰，可校验签名和 hash | 需手工更新或脚本检查 | 当前最佳 |
| winget 安装 | 条件采纳 | 系统包管理器方便更新 | 当前主机无 winget | 有 winget 时可优先 |
| Chocolatey 安装 | 未采纳 | 本机可用 | 包维护链路非 Google 官方 | 不作为首选 |

### 串口登录方案

| 方案 | 状态 | 优点 | 代价/风险 | 当前建议 |
| --- | --- | --- | --- | --- |
| 使用 `DBG_TXD/DBG_RXD` / 系统 Log 调试串口 | 已采纳 | 官方登录路径；当前 `ttyS0`/getty 已就绪；不影响 StackFlow 通信 UART | 需要接到正确的调试接口/FPC/Mate 路径 | 当前最佳 |
| 额外启用 `ttyS1` getty | 条件备用 | 可能增加第二个登录串口 | 需确认 `ttyS1` 物理连线和是否被 StackFlow/TRM 占用 | 仅单独验证后实施 |
| 把 M5-Bus `TRM_TXD/TRM_RXD` 改成 Linux 登录串口 | 未采纳 | 可用 M5-Bus 位置接外部终端 | 可能破坏主控 JSON 通信；涉及 bootargs/DT/pinmux/硬件 Net-Tie | 不作为默认方案 |

## 6. 当前整体最佳方案

生成时间：2026-06-02

在当前采纳配置和已知约束下，整体最佳实施顺序为：

1. Host 使用官方 Android Platform Tools，通过 `ADB_BIN` 指向 `adb.exe`。
2. 每次会话先执行 Host ADB 工具检查和 `adb devices -l`。
3. 通过 ADB 完成完整 baseline：系统、磁盘、网络、apt、包、服务状态。
4. 串口登录保留默认 `ttyS0` / `DBG_TXD/DBG_RXD` / 系统 Log 调试路径；外部独立终端设备接这一路。
5. 使用扩展底座 RJ45 建立设备网络。
6. 网络可用后备份 `/etc/apt`。
7. 按官方 StackFlow apt 源配置软件源并执行 `apt update`。
8. 只安装当前目标需要的 `llm-*` 包。
9. OpenAI API 插件保持可选，只有明确需求和空间确认后再安装。
10. 每轮结束更新本文件、状态文件、变更日志和交接文件。

注意：整体最佳方案是阶段性建议，不会自动覆盖已采纳配置；改变采纳方案必须记录原因并经用户确认。
