# PROJECT_STATE — M5Stack Module LLM Kit（AX630C）

最后更新：2026-06-12
维护者：用户 + Codex

## 1. 项目目标

用 Codex 本地任务协助完成 M5Stack Module LLM Kit（AX630C）的 Linux/ADB/UART/SSH/apt 配置、状态备份、软件包更新、可选 OpenAI API 插件配置，并确保换目录、换机器或任务意外丢失后仍能继续。

## 2. 当前状态总览

- 阶段：2 / 当前 Windows 主机 ADB 已连接；M5-Bus UART 登录已适配 Tab5 并通过重启验证；下一步设备 baseline 备份
- 当前任务文件：`tasks/03_device_baseline_backup.md`
- 最近一次跨机交接：2026-06-02 已保存交接上下文；每次换机仍按 `SESSION_HANDOFF.md` 重新做 Host/ADB 检查
- 设备连接：当前 Windows Host 的 `adb devices -l` 显示唯一 `device` 状态 serial `axera-ax620e`；该 serial 仅为本次观察值
- 主机环境：本次观察为 Windows 10 专业版 10.0.19045 + PowerShell 7.5.5；必须兼容 Windows 10 与 Linux 发行版
- 设备系统版本：Ubuntu 22.04 LTS；hostname `m5stack-LLM`；kernel `Linux 4.19.125` aarch64（当前 Linux Host ADB 只读验证）
- ADB：当前 Windows Host 使用 `C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe`，版本 `37.0.0-14910828`；通过显式 `-s axera-ax620e` 执行 shell 成功
- UART：Linux 默认系统 Log/调试登录终端仍为 `ttyS0`，参数 `115200n8`；M5-Bus UART `/dev/ttyS1`（`serial1` / `/soc/ax_uart@4881000`，对应 `TRM_TXD/TRM_RXD`）已改作额外运行期 root 登录 shell，当前为 `921600 8N1`、无硬件流控、`xterm-256color`、`COLORTERM=truecolor`、`32x64`；`serial-getty@ttyS1.service` enabled/active，`llm-sys.service` masked/inactive；ttyS1 使用 `--autologin root`
- SSH：未知
- 网络：当前无可用外网路由；`eth0` 存在但 `DOWN/NO-CARRIER`，无 `wlan*`，无 `usb0/rndis/ecm/ncm` USB 网络接口；USB gadget 仅配置 `ffs.adb`（2026-06-02 ADB 只读观察）
- apt 源：未知
- 已安装 llm 包：未知
- C/C++ 工具链：已确认 `build-essential`、`binutils`、`gcc/g++`、`make`、`libc6-dev`、`dpkg-dev` 存在；`gdb` 可执行文件存在但不是 dpkg 管理的 `gdb` 包；`gdbserver` 未安装（2026-06-02 ADB 只读观察）
- SoC 温度读取：已确认 `/sys/class/thermal/thermal_zone0/type=soc_thm`，`temp` 为毫摄氏度；已新增 `scripts/read_soc_temp_adb.sh`，默认每 5 秒持续读取
- Fan Module v1.1：用户已决定后续不再关注从 LLM 控制 Fan 模块；历史结论保留为 I2C 17/18 疑似反向和 `0x18` 未应答
- 当前风险：M5-Bus UART 已不再用于 StackFlow/JSON API 通信；物理接入 M5-Bus UART 的外部终端可免账号密码获得 root shell；未完成完整 baseline，下一步仍需做只读 baseline 和低风险备份；温度读取输出显示设备侧时间为 `2023-08-22`，后续 baseline 需复核系统时间

## 3. 已确认需求

- 主要通过 ADB 对设备进行 Linux 发行版设置和配置。
- 基本不进行软件开发。
- 需要让 Codex 能阅读硬件/软件文档。
- 本地任务上下文可能不持久，所以必须把上下文版本化保存到项目目录。
- 需要支持移动项目目录、意外丢失本地任务、换开发机器后的恢复。
- 工作可能分别在 Windows 10 和 Linux 发行版上进行。
- 多次工作不能保证模块对应的 USB 端口、串口名或 ADB serial 相同；每次都必须重新扫描 USB/ADB 设备。
- 官方操作手册使用 ADB，因此任务必须先确认 ADB 已安装并能正常工作。
- 在任何新主机/新环境中，至少先做 Host ADB 工具检查和 USB/ADB 设备扫描。
- 若没有发现 `adb`，必须先询问用户是否安装；若已安装但 PATH 未配置，优先设置推荐环境变量 `ADB_BIN=<adb 绝对路径>`；配置正确后记录版本并按可信来源检查/更新 Host 侧 Platform Tools。
- 所有配置类任务必须查 `context/CONFIG_REGISTRY.md`，用登记的检测命令判断是否已配置好，并按依赖关系执行缺失配置。
- 若发现未记录配置，必须先登记证据和影响范围，不直接覆盖；替代方案、折中采纳方案和阶段性整体最佳方案集中维护在 `CONFIG_REGISTRY.md`。
- 用户在 Linux 虚拟终端、串口终端、console、getty 等术语上可能不精确；Codex 应以用户描述的应用场景为准反推检查项。例如“带屏幕键盘的外部嵌入式开发板通过串口登录 Module LLM”应拆解为物理 UART、电气电平、串口参数、kernel console、systemd serial getty、登录认证、shell、业务串口占用和回滚路径。
- 如果用户明确说明当前开发机任务需要保存、下次任务更换开发机或要在另一台开发机继续，Codex 必须理解为通过 GitHub 保存当前工作并在新开发机拉取继续；应先更新 `PROJECT_STATE.md`、`PLAN_CURRENT.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md` 和相关 `CONFIG_REGISTRY.md` 内容，再检查 git 状态、敏感信息和提交/推送范围。

## 4. 已确认设备事实

初始锚点见 `context/DOCS_INDEX.md`。

- 2026-06-02 官方文档与原理图复核：M5-Bus 上的 Module LLM RX/TX 是模块与 M5 主控通信用 UART，走 StackFlow/JSON API，默认 115200bps 8N1；不是默认 Linux 登录终端。
- 2026-06-02 官方原理图复核：Module LLM 原理图中 `TRM_TXD/TRM_RXD` 与 `DBG_TXD/DBG_RXD` 是两组不同信号；`DBG_TXD/DBG_RXD` 对应系统 Log/调试串口路径。
- 2026-06-02 ADB 只读复核：当前 Linux 登录串口为 `ttyS0`。证据：`/proc/cmdline` 为 `console=ttyS0,115200n8 earlycon=uart8250,mmio32,0x4880000`；`/proc/consoles` 显示 `ttyS0`；`serial-getty@ttyS0.service` active/running；`/sys/class/tty/ttyS0/device` 指向 `4880000.ax_uart`；设备树 `serial0=/soc/ax_uart@4880000`。
- 2026-06-02 ADB 只读复核：当前启用的 SoC UART 只有 `ax_uart@4880000` 和 `ax_uart@4881000`；`ttyS1` 对应 `4881000.ax_uart`。首次复核时 `ttyS1` 未作为 console/getty，后续已按用户目标新增 `serial-getty@ttyS1.service`，但仍不迁移 kernel console/earlycon。
- 2026-06-02 用户明确要求：让外部带屏/键盘嵌入式开发板作为实体终端，通过 M5-Bus UART 与 Module LLM 登录和 shell 交互。已确认 `/dev/ttyS1` 被 `/opt/m5stack/bin/llm_sys` 占用；随后 mask `llm-sys.service`，启用 `serial-getty@ttyS1.service`。当前登录不需要账号/密码；注意：这会牺牲 M5-Bus StackFlow/JSON 通信，不提供 early boot log。
- 2026-06-08 已把 ttyS1 实例级 drop-in 从 115200 改为 `921600 8N1`；重启后复核 `ExecStart`、`stty`、getty 和 `llm-sys` 状态全部通过，未修改 ttyS0、bootargs、DTB、分区或认证方式。
- 2026-06-12 已将 ttyS1 的 `agetty` 终端类型从 `vt102` 改为 `xterm-256color`，新增 `/etc/profile.d/m5bus-ttyS1-tab5.sh`，仅在控制终端为 `/dev/ttyS1` 时设置 `TERM=xterm-256color`、`COLORTERM=truecolor` 和 `stty rows 32 cols 64`。当前会话及设备重启后均通过真实 ttyS1 shell 自检；ttyS0 和非 ttyS1 会话未受影响。
- 2026-06-02 用户意图澄清：串口登录相关任务按“外部实体终端设备通过 UART 获得 Module LLM Linux 登录 shell”的应用场景理解，不按 Linux `tty1` 虚拟控制台术语字面处理。
- 2026-06-02 ADB 只读复核：设备内置 Ubuntu 22.04 LTS 上已安装 C/C++ 基础构建链：`build-essential 12.9ubuntu3`、`binutils 2.38-4ubuntu2.6`、`binutils-aarch64-linux-gnu 2.38-4ubuntu2.6`、`gcc/g++` 元包 `4:11.2.0-1ubuntu1`，实际 `gcc/g++ 11.4.0`，目标 `aarch64-linux-gnu`。
- 2026-06-02 ADB 只读复核：`/usr/bin/gdb` 存在，版本 `GNU gdb 9.2`，但 `dpkg-query gdb` 显示 `unknown ok not-installed` 且 `dpkg -S /usr/bin/gdb` 无归属；`gdbserver` 未安装。
- 2026-06-02 ADB 只读复核：当前系统镜像没有显示出通过 ADB 原生获取互联网的能力。证据：USB gadget functions 只有 `ffs.adb`；设备侧没有 USB 网络接口；`ip route get 1.1.1.1` 返回 Network is unreachable；`adb reverse --list` 返回 `error: closed`；设备端 `ping/curl/wget` 访问公网和 M5Stack repo 均失败。

## 5. 主机环境

每次会话都重新填写或更新为“本次观察值”，不要作为永久事实跨机器套用。

```text
OS: Windows 10 专业版 10.0.19045（本次观察）
Shell: PowerShell 7.5.5；脚本语法检查使用 Git Bash
Codex version:
adb path: C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe
adb version: Android Debug Bridge version 1.0.41, Version 37.0.0-14910828
adb devices -l output file: inventory/before/tab5-ttyS1-preflight-20260612-100705.txt
USB permissions / driver state: Windows PnP `ax620e-adb`，Microsoft WinUSB
ssh client:
serial tool:
network access:
```

## 6. 设备连接信息

不要记录真实密码或私钥。  
ADB serial 和 USB 端口只记录为本次会话观察值；换线、换口、重启、换主机后必须重新扫描。

```text
连接方式: USB-C / ADB（本次观察；当前 Windows Host 已可用）
本次 ADB serial: axera-ax620e（本次观察；当前状态 `device`，换线/重启后必须重新扫描）
本次 UART port:
本次 SSH IP:
SSH user:
登录凭据保存方式:
```

## 7. 当前实施方案

见 `context/PLAN_CURRENT.md`。当前原则：先确认 Host OS 和 ADB 工具，再扫描当前 USB/ADB 设备，只读探测和备份，然后配置网络、apt 源、包更新，最后做可选插件/服务。

## 8. 已完成工作

- [ ] 建立任务目录
- [ ] 初始化 git
- [x] 阅读官方文档（已复核 M5-Bus UART / 调试串口用途）
- [x] 主机 OS 检查（本次观察：Ubuntu 24.04.4 LTS + zsh；早前观察为 Windows 10 + PowerShell 7.5.5）
- [x] ADB 安装/版本检查（当前 Linux `/usr/bin/adb` 可用，版本 34.0.4-debian；早前 Windows 官方 Platform Tools 版本 37.0.0-14910828）
- [x] 当前 USB 设备扫描（当前 Linux `lsusb` 已识别 `32c9:2003 axera ax620e-adb`；早前 Windows PnP 已识别 ax620e-adb）
- [x] 当前 ADB 设备扫描可用性（当前 Linux 显示唯一 `device` 状态设备：`axera-ax620e`）
- [x] ADB 连接确认（当前 Linux 通过显式 `-s axera-ax620e` 只读 shell 验证成功）
- [x] SoC 温度只读读取脚本（`scripts/read_soc_temp_adb.sh`，5 秒间隔）
- [x] Fan Module v1.1 控制资料复核（I2C `0x18`，pin 17/18；不与 M5-Bus UART 通信脚重叠）
- [x] Fan Module v1.1 I2C 实机探测（I2C adapter 启用，但 `0x18` 未应答；温控脚本已准备，未执行寄存器写入）
- [x] M5-Bus UART 实体终端登录（`ttyS1`/`TRM_TXD/TRM_RXD` 已启用 `serial-getty@ttyS1`；`llm-sys` 已禁用）
- [ ] 设备只读探测
- [ ] baseline 备份
- [ ] 网络/SSH 确认
- [ ] apt 源配置
- [ ] llm 包清单记录
- [ ] 可选 OpenAI API 插件
- [ ] 恢复方案验证

## 9. 当前卡点

当前 Linux Host 的 ADB 权限问题已修复。用户已授权执行 `scripts/fix_m5stack_adb_udev.sh`，Host 侧规则 `/etc/udev/rules.d/51-m5stack-axera-adb.rules` 已写入，设备节点变为 `root:plugdev 0660`。`adb devices -l` 显示唯一 `device` 状态设备 `axera-ax620e`，显式 serial 的只读 shell 验证成功。下一步是执行 `tasks/03_device_baseline_backup.md`，做只读 baseline 探测和 `/etc/apt` 备份；不要直接进入 apt/SSH 修改。

## 10. 关键文件与输出

- `inventory/before/`：改动前探测输出
- `inventory/after/`：改动后探测输出
- `backups/`：配置备份和项目备份
- `notes/`：人工观察记录
- `scripts/host_check.sh` / `scripts/host_check.ps1`：主机与 ADB 检查
- `scripts/device_probe_adb.sh` / `scripts/device_probe_adb.ps1`：ADB 只读探测
- `context/CONFIG_REGISTRY.md`：采纳配置、检测命令、依赖、替代方案、未记录配置和整体最佳方案
- `tasks/09_config_registry_audit.md`：配置登记审计和整体方案整理任务
- `inventory/before/host-tools-20260602-162913.txt`：本次 Host/ADB/USB 预检输出
- `inventory/before/usb-adb-device-20260602-163018.txt`：本次 USB ADB 设备与驱动详情
- `inventory/after/host-adb-install-20260602-163412.txt`：官方 Platform Tools 下载、签名、版本记录
- `inventory/before/host-tools-20260602-163454.txt`：安装后 ADB preflight 和 `adb devices -l`
- `inventory/before/adb-connect-readonly-20260602-163601.txt`：显式 serial 的 ADB 只读连接验证
- 2026-06-02 已更新脚本：Host/ADB 脚本会优先使用 `ADB_BIN`，并在 Windows/Linux 常见 SDK 路径中查找已安装 adb；未发现时提示先询问是否安装。
- `inventory/before/host-tools-20260602-164338.txt`：验证未设置 `ADB_BIN` 时，PowerShell 脚本可从常见路径发现官方 `adb.exe` 并完成 `adb devices -l`。
- 当前 Windows `bash.exe` 指向 WSL，WSL 启动失败；本机未能完成 Bash 脚本语法检查。
- `inventory/before/host-tools-20260602-164709.txt`：本轮重新执行 Host/ADB preflight，确认唯一 `device` 状态设备 `axera-ax620e`。
- `inventory/before/adb-connect-readonly-20260602-164730.txt`：本轮显式 serial 的 ADB 只读连接验证。
- `inventory/before/toolchain-check-adb-20260602-165625.txt`：C/C++ 工具链与 binutils/gdb 只读检查。
- `inventory/before/gdb-origin-check-adb-20260602-165651.txt`：`/usr/bin/gdb` 来源与配置只读检查。
- `inventory/before/adb-network-capability-20260602-170131.txt`：网络接口、路由、DNS、USB gadget 与 ADB forward/reverse 只读检查。
- `inventory/before/usb-gadget-adb-functions-20260602-170200.txt`：USB gadget functions/configs 只读检查。
- `inventory/before/device-internet-probe-adb-20260602-170221.txt`：设备端公网/M5 repo 连通性只读测试。
- `inventory/before/serial-login-adb-preflight-20260602-173920.txt`：本轮串口登录审计前 ADB preflight。
- `inventory/before/serial-login-config-adb-20260602-174529.txt`：Linux console/getty/tty 只读检查。
- `inventory/before/serial-login-devicetree-adb-20260602-174558.txt`：设备树 UART alias 和 `ttyS0/ttyS1` 映射只读检查。
- `inventory/before/serial-uart-status-adb-20260602-174644.txt`：SoC UART 节点启用状态只读检查。
- `inventory/before/host-tools-20260602-184723.txt`：当前 Linux Host 工具与 USB/ADB 预检；`adb` 可用，设备 `axera-ax620e` 为 `no permissions`。
- `inventory/before/adb-linux-permission-diagnosis-20260602-190543.txt`：当前 Linux Host ADB 权限诊断；确认缺少 `32c9:2003` udev 规则。
- `scripts/fix_m5stack_adb_udev.sh`：Host 侧 M5Stack/AXERA ADB udev 修复脚本，默认 dry-run，`APPLY=1` 时需要 sudo。
- `/etc/udev/rules.d/51-m5stack-axera-adb.rules`：当前 Linux Host 已应用的 Host 侧 udev 规则（不在仓库内）。
- `inventory/before/adb-reconnect-success-20260602-192018.txt`：当前 Linux Host ADB 权限修复后的 preflight 与只读 shell 验证输出。
- `scripts/read_soc_temp_adb.sh`：Host 侧通过 ADB 每 5 秒读取 Module LLM SoC 温度的只读脚本。
- `inventory/before/soc-temp-interface-scan-20260602-192732.txt`：SoC thermal zone 扫描输出。
- `inventory/before/soc-temp-read-20260602-192730.txt`：`COUNT=3 INTERVAL=5` 温度读取验证输出。
- `scripts/fan_temp_control_adb.sh`：Host 侧通过 ADB/I2C 按 SoC 温度控制 Fan Module v1.1 的脚本；未检测到 `0x18` 时不写寄存器；当前已不作为后续目标。
- `inventory/before/m5bus-i2c-fan-scan-20260602-215510.txt`：I2C adapter、Fan `0x18` 目标探测和读模式扫描输出。
- `inventory/before/fan-temp-control-attempt-20260602-215650.txt`：温控脚本短测输出；因 Fan `0x18` 未检测到，未写风扇寄存器。
- `scripts/enable_m5bus_uart_login_adb.sh`：Host 侧通过 ADB 启用/回滚 M5-Bus UART `/dev/ttyS1` 登录的脚本。
- `inventory/before/m5bus-uart-login-preflight-20260602-220954.txt`：M5-Bus UART 登录任务前 ADB preflight。
- `inventory/before/m5bus-uart-login-readonly-20260602-221041.txt`：UART、getty、进程和 `/dev/ttyS1` 占用只读探测。
- `inventory/before/m5bus-uart-login-llm-services-20260602-221203.txt`：`llm-sys.service` 与 `/dev/ttyS1` 占用证据。
- `inventory/before/m5bus-uart-login-dryrun-20260602-221426.txt`：M5-Bus UART 登录脚本 dry-run。
- `inventory/after/m5bus-uart-login-apply-20260602-221439.txt`：实际应用输出；设备侧备份目录 `/root/m5bus-uart-login-backup-20260602-221439`。
- `inventory/after/m5bus-uart-login-verify-20260602-221559.txt`：初次后验验证输出；`serial-getty@ttyS1` active/running，`llm-sys` disabled/inactive，`ttyS1` 为 115200。
- `inventory/after/m5bus-uart-login-auth-check-20260602-221842.txt`：登录认证确认；ttyS1 使用 `--autologin root`，当前 `/dev/ttyS1` 上为 `/bin/login -f` 和 `-bash`，不需要账号/密码。
- `inventory/after/llm-reboot-for-lcd-terminal-20260602-223436.txt`：用户要求重启后的首次验证；发现 `llm-sys.service` 虽 disabled 仍被其他 `llm-*` 服务依赖拉起并重新占用 `/dev/ttyS1`。
- `inventory/before/llm-sys-post-reboot-autostart-cause-20260602-223543.txt`：确认其他 `llm-*` 服务 `RequiredBy=llm-sys.service`，单纯 disable 不足。
- `inventory/after/m5bus-uart-login-mask-llm-sys-20260602-223610.txt`：补充 mask `llm-sys.service` 后，`ttyS1` 只剩 login/bash 占用。
- `inventory/after/llm-reboot-after-mask-verify-20260602-223640.txt`：第二次重启验证；`llm-sys.service` masked/inactive，`serial-getty@ttyS1` active/running，持久化生效。
- `inventory/before/m5bus-uart-921600-preflight-20260608-142242.txt`：本轮 Windows Host ADB preflight。
- `inventory/before/m5bus-uart-921600-readonly-20260608-142430.txt`：改动前 ttyS1、getty、llm-sys、串口参数和占用进程快照。
- `backups/m5bus-uart-login-backup-20260608-142700/`：改动前 drop-in、systemd、stty 和 cmdline 的 Host 侧备份；设备侧对应 `/root/m5bus-uart-login-backup-20260608-142700`。
- `inventory/after/m5bus-uart-921600-apply-20260608-142718.txt`：实时应用 921600 后的验证。
- `inventory/after/m5bus-uart-921600-reboot-preflight-20260608-142754.txt`：重启后重新扫描 ADB 设备的记录。
- `inventory/after/m5bus-uart-921600-reboot-verify-20260608-142828.txt`：重启后 921600 8N1 持久化强校验，结果 `VERIFY=PASS`。
- `inventory/before/tab5-ttyS1-preflight-20260612-100705.txt`：Tab5 适配任务的 Windows Host ADB/USB preflight。
- `inventory/before/tab5-ttyS1-readonly-20260612-100838.txt`：改动前 getty、ttyS0/ttyS1、terminfo、htop 和 profile 启动链审计。
- `backups/m5bus-ttyS1-tab5-backup-20260612-101143/`：改动前 Host 侧配置备份；设备侧对应 `/root/m5bus-ttyS1-tab5-backup-20260612-101143`。
- `inventory/after/tab5-ttyS1-current-verify-20260612-101533.txt`：当前 ttyS1 真实登录 shell 与 htop 初始化验证，结果 `CURRENT_VERIFY=PASS`。
- `inventory/after/tab5-ttyS1-reboot-preflight-20260612-101641.txt`：重启后重新执行 ADB 设备扫描与版本检查。
- `inventory/after/tab5-ttyS1-reboot-verify-20260612-101722.txt`：重启后 ttyS1 Tab5 配置持久化验证，结果 `REBOOT_VERIFY=PASS`。

## 11. 下一步

继续时先读取 `AGENTS.md` 和 `context/SESSION_HANDOFF.md`。随后重新执行 `adb version`、`adb start-server`、`adb devices -l`；若仍只有一个 `device` 状态设备 `axera-ax620e`，执行 `tasks/03_device_baseline_backup.md` 做只读探测与备份。若要恢复 M5-Bus StackFlow/JSON 通信，执行 `MODE=rollback APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>`。若换线/重启/换主机后再次出现 `no permissions`，再按 `scripts/fix_m5stack_adb_udev.sh` 的 Host 侧 udev 规则处理。
