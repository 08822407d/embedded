# CONFIG_REGISTRY — 配置登记、检测与方案台账

最后更新：2026-06-12
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
- 当前已按用户目标把 M5-Bus UART `/dev/ttyS1` 由 StackFlow 通信口改作额外 Linux 登录口；该配置依赖 `llm-sys.service` 停用。
- 任何换线、换 USB 口、设备重启、换主机后，ADB serial 必须重新扫描。
- 镜像底包更新独立为高风险恢复路径，不是常规配置依赖链的一部分。

## 3. 当前采纳配置项

### CFG-HOST-ADB-001 — Host 侧 ADB 工具

状态：`compliant`（当前 Linux 主机工具可用；早前 Windows 主机也可用）

采纳配置：

- 当前 Linux 主机使用 `/usr/bin/adb`，实际安装路径 `/usr/lib/android-sdk/platform-tools/adb`，版本 `34.0.4-debian`。
- 早前 Windows 主机使用 Android 官方 Platform Tools，路径 `C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe`。
- 如需显式指定，Linux 推荐当前会话 `export ADB_BIN="/usr/bin/adb"`；Windows 推荐当前会话设置 `ADB_BIN` 到 `adb.exe` 绝对路径。

检测命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\host_check.ps1
```

```bash
bash scripts/host_check.sh
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
- `inventory/before/host-tools-20260602-184723.txt`
- `inventory/before/adb-linux-permission-diagnosis-20260602-190543.txt`

### CFG-ADB-TARGET-001 — ADB 目标设备选择

状态：`compliant`（当前 Linux Host 权限已修复；唯一 `device` 状态设备为 `axera-ax620e`）

采纳配置：

- 每轮使用前重新执行 `adb devices -l`。
- 多设备时必须指定 `ADB_SERIAL` 或 `adb -s <serial>`。
- 本次观察 serial：`axera-ax620e`，当前状态 `device`，不可跨会话视为永久事实。

检测命令：

```powershell
$env:ADB_BIN="C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe"
& $env:ADB_BIN devices -l
```

```bash
export ADB_BIN=/usr/bin/adb
$ADB_BIN devices -l
```

合规判定：

- 若只有一个 `device` 状态设备，可继续但仍记录 serial。
- 若多个 `device` 状态设备，停止并要求用户选择。
- 若无 `device` 状态设备，或设备为 `no permissions` / `unauthorized` / `offline`，停止设备操作。

缺失时正确处理：

- Linux `no permissions`：不要执行 `adb shell/push/pull`；先诊断用户组、USB 节点权限和 udev 规则。
- 此前 Linux Host `no permissions` 根因：用户 `cheyh` 已在 `plugdev` 组；修复前设备节点 `/dev/bus/usb/001/015` 为 `root:root 0664`；`android-sdk-platform-tools-common` 已安装但现有 `51-android.rules` 未包含 M5Stack/AXERA `32c9:2003`。
- 正确修复路径：在 Host 侧新增 udev 规则 `SUBSYSTEM=="usb", ATTR{idVendor}=="32c9", ATTR{idProduct}=="2003", MODE="0660", GROUP="plugdev", TAG+="uaccess"`，重载 udev，修正当前节点或重新插拔设备，再重启 ADB server。
- 本项目已准备脚本：`scripts/fix_m5stack_adb_udev.sh`。默认 dry-run；应用需 sudo：`APPLY=1 bash scripts/fix_m5stack_adb_udev.sh`。
- Windows 驱动异常：先记录 PnP/驱动状态，再按用户确认处理驱动或 Platform Tools。

已知证据：

- `inventory/before/host-tools-20260602-164709.txt`
- `inventory/before/adb-connect-readonly-20260602-164730.txt`
- `inventory/before/host-tools-20260602-184723.txt`
- `inventory/before/adb-linux-permission-diagnosis-20260602-190543.txt`
- `inventory/before/adb-reconnect-success-20260602-192018.txt`

已知 Host 侧修复：

- 当前 Linux Host 已写入 `/etc/udev/rules.d/51-m5stack-axera-adb.rules`。
- 规则内容匹配 M5Stack/AXERA `32c9:2003`，设置 `MODE="0660"`、`GROUP="plugdev"`、`TAG+="uaccess"`。
- 该规则是 Host 侧配置，不修改 Module LLM 设备系统。

### CFG-SERIAL-LOGIN-001 — Linux 串口登录终端

状态：`compliant`（默认 `ttyS0` 登录保留；另见 `CFG-M5BUS-UART-LOGIN-001`）

用户意图解释：

- 用户目标不是研究 Linux 虚拟终端框架本身，而是让“带屏幕和键盘的外部嵌入式开发板”通过串口成为 Module LLM 的独立字符终端。
- 遇到用户说“串口登录”“终端”“纯字符界面”“init 3 那种界面”等表述时，应按该场景反推检查项：物理 UART 引脚、电气电平、串口参数、kernel console、systemd serial getty、登录认证、shell、业务串口占用和回滚路径。
- 不应把用户意图窄化为 Linux `tty1`/虚拟控制台，也不应默认把 M5-Bus 通信 UART 当成登录 UART。

采纳配置：

- 保留默认 Linux 登录串口：`ttyS0`，`115200n8`。
- `ttyS0` 同时是 kernel console 和 systemd serial getty。
- 当前运行时映射：`ttyS0 -> /soc/ax_uart@4880000`（`serial0`，MMIO `0x04880000`）；`ttyS1 -> /soc/ax_uart@4881000`（`serial1`），当前已按 `CFG-M5BUS-UART-LOGIN-001` 绑定 `serial-getty@ttyS1.service`。
- 硬件路径判断：`ttyS0`/`serial0` 对应官方系统 Log/调试串口路径，即 `DBG_TXD/DBG_RXD`；M5-Bus 上 `TRM_TXD/TRM_RXD` 是 StackFlow/JSON API 通信 UART，不作为默认 Linux 登录终端。
- 2026-06-02 用户明确要把外部带屏嵌入式开发板作为实体串口终端，通过 M5-Bus UART 登录 Module LLM。已新增 `CFG-M5BUS-UART-LOGIN-001` 记录该改配；`ttyS0` 仍保留为系统 Log/调试登录口。

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

### CFG-M5BUS-UART-LOGIN-001 — M5-Bus UART 实体终端登录

状态：`compliant`（已按用户要求启用）

目标场景：

- 外部带屏/键盘的嵌入式开发板作为实体串口终端，通过 M5-Bus UART 与 Module LLM 的 Linux shell 交互。
- 外部开发板侧需要运行串口终端程序，参数 `921600 8N1`，无硬件流控，3.3V TTL，TX/RX 交叉，公共 GND。
- 当前 Tab5 终端契约：`TERM=xterm-256color`、`COLORTERM=truecolor`、终端尺寸 `32x64`、`tput colors=256`。

采纳配置：

- Module LLM Linux 侧使用 `/dev/ttyS1`，映射为 `/soc/ax_uart@4881000` / `serial1`。
- 该 UART 在官方资料中对应 M5-Bus `TRM_TXD(NT)` / `TRM_RXD(NT)` 通信口，M5-Bus pin 15/16。
- 已停止并 mask `llm-sys.service`，释放 `/dev/ttyS1` 并阻止它在开机时被其他 `llm-*` 服务依赖拉起。代价：M5-Bus 上原 StackFlow/JSON API 通信口不可用，M5 主控无法再通过该 UART 与 `llm_sys` 交互。
- 已启用 `serial-getty@ttyS1.service`。
- 已新增实例级 drop-in：`/etc/systemd/system/serial-getty@ttyS1.service.d/override.conf`。
- 当前认证方式：不需要输入账号密码。`agetty` 使用 `--autologin root`，当前 `/dev/ttyS1` 上可见 `/bin/login -f` 和 `-bash`，表示已自动登录 root。
- drop-in 内容：

```ini
[Service]
ExecStart=
ExecStart=-/sbin/agetty -L --autologin root --noclear 921600 %I xterm-256color
```

- ttyS1 专属登录 shell 配置文件：`/etc/profile.d/m5bus-ttyS1-tab5.sh`。

```sh
if _m5bus_tty=$(tty 2>/dev/null) && [ "$_m5bus_tty" = /dev/ttyS1 ]; then
  export TERM=xterm-256color
  export COLORTERM=truecolor
  stty rows 32 cols 64 2>/dev/null || true
fi
unset _m5bus_tty
```

重要边界：

- `ttyS1` 不是 kernel console，也没有 earlycon；通过 M5-Bus UART 只能获得运行期登录 shell，看不到早期 boot log。
- `ttyS0` / `DBG_TXD/DBG_RXD` 仍保留为系统 Log/调试登录路径。
- `/etc/profile.d/m5bus-ttyS1-tab5.sh` 以 `tty` 的精确结果 `/dev/ttyS1` 为条件；ttyS0、SSH、ADB shell 和其他 `/dev/pts/*` 会话不会设置这些变量或尺寸。
- 当前 `serial-getty@.service` 模板本身带 `--autologin root`；本次 ttyS1 drop-in 也沿用 autologin root。物理接触到 M5-Bus UART 的设备可直接获得 root shell，属于安全取舍。
- 若需要账号/密码登录，可把 ttyS1 drop-in 改为不带 `--autologin root` 的 `agetty` 命令，例如 `ExecStart=-/sbin/agetty -L --noclear 921600 %I xterm-256color`，然后 `systemctl daemon-reload && systemctl restart serial-getty@ttyS1.service`。执行前应确认 root 密码策略和恢复路径，不把密码写入仓库。

检测命令：

```sh
systemctl show llm-sys.service serial-getty@ttyS1.service -p LoadState -p ActiveState -p SubState -p UnitFileState -p FragmentPath --no-pager
for fd in /proc/[0-9]*/fd/*; do target=$(readlink "$fd" 2>/dev/null) || continue; [ "$target" = /dev/ttyS1 ] || continue; pid=${fd#/proc/}; pid=${pid%%/*}; cmd=$(tr "\000" " " < /proc/$pid/cmdline 2>/dev/null); echo pid=$pid fd=${fd##*/} target=$target cmd=$cmd; done
stty -F /dev/ttyS1 -a
cat /etc/systemd/system/serial-getty@ttyS1.service.d/override.conf
cat /etc/profile.d/m5bus-ttyS1-tab5.sh
infocmp xterm-256color
TERM=xterm-256color tput colors
```

合规判定：

- `llm-sys.service` 为 `inactive/dead` 且 `masked`。
- `serial-getty@ttyS1.service` 为 `active/running` 且 `enabled`。
- `/dev/ttyS1` 当前由 `/bin/login -f` / `-bash` 占用，而不是 `/opt/m5stack/bin/llm_sys`。
- `agetty` 的最后一个参数为 `xterm-256color`。
- ttyS1 实际登录 shell 中 `TERM=xterm-256color`、`COLORTERM=truecolor`。
- `stty -F /dev/ttyS1 -a` 显示 `speed 921600 baud`、`rows 32`、`columns 64`、`cs8`、`-cstopb`、`-parenb`、`-crtscts`。
- `infocmp xterm-256color` 成功，ttyS1 shell 中 `tput colors` 返回 `256`。
- `htop` 能在 `xterm-256color`、`32x64` 伪终端中完成 ncurses 彩色界面初始化。
- ttyS0 仍为 `115200`、`24x80`；非 ttyS1 会话 source 该 profile 后环境保持不变。

执行脚本：

```bash
BAUD=921600 TERM_TYPE=xterm-256color COLORTERM_VALUE=truecolor ROWS=32 COLS=64 \
  APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>
```

脚本当前默认值即为 `921600`、`xterm-256color`、`truecolor`、`32x64`。若只想保留 ttyS1 登录但回退到旧波特率，执行：

```bash
BAUD=115200 APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>
```

回滚脚本：

```bash
MODE=rollback APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>
```

回滚效果：

- 停止并禁用 `serial-getty@ttyS1.service`。
- 删除脚本管理的 `/etc/profile.d/m5bus-ttyS1-tab5.sh`。
- 解除 mask、重新启用并启动 `llm-sys.service`。
- 保留 ttyS1 drop-in 文件，便于以后再次启用；若要彻底清理，可另行删除 `/etc/systemd/system/serial-getty@ttyS1.service.d/` 并 `systemctl daemon-reload`。

已知证据：

- `inventory/before/m5bus-uart-login-preflight-20260602-220954.txt`
- `inventory/before/m5bus-uart-login-readonly-20260602-221041.txt`
- `inventory/before/m5bus-uart-login-llm-services-20260602-221203.txt`
- `inventory/before/m5bus-uart-login-dryrun-20260602-221426.txt`
- `inventory/after/m5bus-uart-login-apply-20260602-221439.txt`
- `inventory/after/m5bus-uart-login-verify-20260602-221559.txt`
- `inventory/after/m5bus-uart-login-auth-check-20260602-221842.txt`
- `inventory/after/llm-reboot-for-lcd-terminal-20260602-223436.txt`
- `inventory/before/llm-sys-post-reboot-autostart-cause-20260602-223543.txt`
- `inventory/after/m5bus-uart-login-mask-llm-sys-20260602-223610.txt`
- `inventory/after/llm-reboot-after-mask-verify-20260602-223640.txt`
- `inventory/before/m5bus-uart-921600-preflight-20260608-142242.txt`
- `inventory/before/m5bus-uart-921600-readonly-20260608-142430.txt`
- `inventory/before/m5bus-uart-921600-backup-20260608-142700.txt`
- `backups/m5bus-uart-login-backup-20260608-142700/`
- `inventory/after/m5bus-uart-921600-apply-20260608-142718.txt`
- `inventory/after/m5bus-uart-921600-reboot-preflight-20260608-142754.txt`
- `inventory/after/m5bus-uart-921600-reboot-verify-20260608-142828.txt`
- `inventory/before/tab5-ttyS1-preflight-20260612-100705.txt`
- `inventory/before/tab5-ttyS1-readonly-20260612-100838.txt`
- `inventory/before/tab5-ttyS1-backup-20260612-101143.txt`
- `backups/m5bus-ttyS1-tab5-backup-20260612-101143/`
- `inventory/after/tab5-ttyS1-apply-20260612-101219.txt`
- `inventory/after/tab5-ttyS1-current-verify-20260612-101533.txt`
- `inventory/after/tab5-ttyS1-reboot-preflight-20260612-101641.txt`
- `inventory/after/tab5-ttyS1-reboot-verify-20260612-101722.txt`

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

### CFG-SOC-TEMP-001 — SoC 温度读取工具

状态：`compliant`

采纳配置：

- 不修改设备配置，通过 ADB 只读读取 Linux thermal sysfs。
- 当前 SoC 温度节点：`/sys/class/thermal/thermal_zone0/temp`。
- 当前 thermal zone 类型：`soc_thm`。
- `temp` 数值单位按 Linux thermal 约定为毫摄氏度，例如 `42350` 表示 `42.350 C`。
- Host 侧脚本：`scripts/read_soc_temp_adb.sh`，默认 `INTERVAL=5`、`COUNT=0` 持续运行。

检测命令：

```bash
adb -s <serial> shell 'cat /sys/class/thermal/thermal_zone0/type; cat /sys/class/thermal/thermal_zone0/temp'
COUNT=3 INTERVAL=5 bash scripts/read_soc_temp_adb.sh <serial>
```

合规判定：

- `type` 为 `soc_thm`。
- `temp` 可读且为整数。
- 脚本能每隔 5 秒输出 `raw=<毫摄氏度>` 和换算后的 `temp=<摄氏度> C`。

已知证据：

- `inventory/before/soc-temp-interface-scan-20260602-192732.txt`
- `inventory/before/soc-temp-read-20260602-192730.txt`

### CFG-FAN-MODULE-001 — M5Stack Fan Module v1.1 控制

状态：`deferred`（用户已决定后续不再关注由 LLM 控制 Fan 模块）

资料结论：

- Fan Module v1.1 通过 I2C 控制，默认地址 `0x18`。
- M5-Bus 控制引脚为 pin 17 `SDA`、pin 18 `SCL`。
- 标准 M5-Bus / CoreS3 也是 pin 17 `SDA`、pin 18 `SCL`。
- Module LLM Kit 产品页显示 Module LLM 为 pin 17 `SoC_SCL`、pin 18 `SoC_SDA`；Module13.2 LLM Mate 为 pin 17 `SCL`、pin 18 `SDA`，与 Fan/标准 M5-Bus 正好相反。
- 注意：M5Stack 官方资料存在不一致，Module LLM 单品页和部分原理图文本提取显示过标准方向；但用户肉眼观察和当前叠插实机 `0x18` 不应答均支持“当前组合默认 I2C 不通”。
- 结论：在未改焊盘/未做 SDA/SCL 交叉转接情况下，Fan v1.1 叠插在 LLM Kit 上很可能无法通过默认 M5-Bus I2C 控制。该问题不是 UART 脚冲突，而是 I2C 的 SDA/SCL 位置疑似相反。
- 实机观察：当前 Linux 有 `/dev/i2c-0`、`/dev/i2c-2`、`/dev/i2c-4`，device-tree 中对应 `i2c@4850000`、`i2c@4852000`、`i2c@4854000` 为 `okay`。
- 实机观察：`i2c-0/2/4` 对 Fan 默认地址 `0x18` 均无应答；读模式全地址扫描只看到 `i2c-4` 上 `0x30` / `lp5562` 和 `0x47` / `sgm7220` 已由内核占用。
- 因 Fan `0x18` 未检测到，本轮未执行风扇启停/占空比寄存器写入。
- 当前建议：先不要继续软件写寄存器；需要确认物理叠插路径。可用交叉转接/飞线方式把 LLM 侧 SCL/SDA 与 Fan 侧 SCL/SDA 对齐后再重新扫描 `0x18`。
- 2026-06-02 用户决定：后续不用再关心从 LLM 控制 Fan 模块的事。本配置项保留为历史证据，不作为当前实施目标。

控制寄存器：

```text
I2C address: 0x18
0x00 R/W  fan status: 0 stop, 1 run
0x10 R/W  PWM frequency: 0=1kHz, 1=12kHz, 2=24kHz, 3=48kHz
0x20 R/W  PWM duty cycle: 0..100
0x30 R    RPM low/high byte
0x40 R    fan signal frequency low/high byte
```

后续若要实机控制：

1. 先只读确认 Module LLM Linux 是否有 `/dev/i2c-*`，并判断哪一路连接 M5-Bus `SoC_SDA/SoC_SCL`。
2. 再读探测 `0x18` 是否应答。
3. 若 `0x18` 应答，可用 `scripts/fan_temp_control_adb.sh` 按 SoC 温度控制风扇。
4. 写 `0x10`/`0x20`/`0x00` 前先说明风险；写 `0xF0` Flash 回写寄存器默认不做。

Host 侧脚本：

- `scripts/fan_temp_control_adb.sh`
- 默认 `INTERVAL=5`，`COUNT=0` 持续运行。
- 使用近似 Raspberry Pi 5 官方风扇曲线：`<50C=0%`、`50C=30%`、`60C=50%`、`67.5C=70%`、`75C=100%`，降速使用 5C 回差。
- 脚本先探测 Fan `0x18`，未检测到时退出且不写寄存器。

已知证据：

- `inventory/before/m5bus-i2c-fan-scan-20260602-215510.txt`
- `inventory/before/fan-temp-control-attempt-20260602-215650.txt`

## 4. 未记录配置观察

| 日期 | 范围 | 观察 | 证据 | 处理状态 |
| --- | --- | --- | --- | --- |
| 2026-06-02 | Device gdb | `/usr/bin/gdb` 存在且可运行，但不是 dpkg 管理的 `gdb` 包 | `inventory/before/gdb-origin-check-adb-20260602-165651.txt` | 记录为现场事实；不修改 |
| 2026-06-02 | Device time | 温度读取输出中的设备侧时间戳为 `2023-08-22`，与当前 Host 日期不一致；可能影响后续 apt/TLS | `inventory/before/soc-temp-read-20260602-192730.txt` | 记录为现场事实；baseline 阶段复核，不在本轮修改 |
| 2026-06-02 | Device M5-Bus UART | `/opt/m5stack/bin/llm_sys` 默认持有 `/dev/ttyS1`，这是把 M5-Bus UART 改作登录口前必须释放的占用 | `inventory/before/m5bus-uart-login-readonly-20260602-221041.txt`、`inventory/before/m5bus-uart-login-llm-services-20260602-221203.txt` | 已按用户目标 mask `llm-sys.service` 并启用 `serial-getty@ttyS1` |

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
| 额外启用 `ttyS1` getty | 已采纳 | 满足 Tab5 等外部实体终端通过 M5-Bus UART 登录 shell；当前支持 256 色和固定 32x64 | 已 mask `llm-sys.service`，M5-Bus StackFlow/JSON 通信不可用；无 early boot log | 当前按用户目标实施 |
| 把 M5-Bus `TRM_TXD/TRM_RXD` 改成 Linux 登录串口 | 已采纳 | 可用 M5-Bus 位置接外部终端 | 破坏主控 JSON 通信；物理访问者可获得 root shell | 当前按用户目标实施，保留回滚脚本 |

## 6. 当前整体最佳方案

生成时间：2026-06-12

在当前采纳配置和已知约束下，整体最佳实施顺序为：

1. Host 使用官方 Android Platform Tools，通过 `ADB_BIN` 指向 `adb.exe`。
2. 每次会话先执行 Host ADB 工具检查和 `adb devices -l`。
3. 通过 ADB 完成完整 baseline：系统、磁盘、网络、apt、包、服务状态。
4. 串口登录保留默认 `ttyS0` / `DBG_TXD/DBG_RXD` / 系统 Log 调试路径，同时把 M5-Bus UART `/dev/ttyS1` 作为 Tab5 适配的 `921600 8N1`、`xterm-256color`、`truecolor`、`32x64` 运行期 root 登录 shell。
5. 若需要恢复 M5-Bus StackFlow/JSON 通信，先回滚 `CFG-M5BUS-UART-LOGIN-001`。
6. 使用扩展底座 RJ45 建立设备网络。
7. 网络可用后备份 `/etc/apt`。
8. 按官方 StackFlow apt 源配置软件源并执行 `apt update`。
9. 只安装当前目标需要的 `llm-*` 包。
9. OpenAI API 插件保持可选，只有明确需求和空间确认后再安装。
10. 每轮结束更新本文件、状态文件、变更日志和交接文件。

注意：整体最佳方案是阶段性建议，不会自动覆盖已采纳配置；改变采纳方案必须记录原因并经用户确认。
