# PROJECT_STATE — M5Stack Module LLM Kit（AX630C）

最后更新：2026-06-02  
维护者：用户 + Codex

## 1. 项目目标

用 Codex 本地任务协助完成 M5Stack Module LLM Kit（AX630C）的 Linux/ADB/UART/SSH/apt 配置、状态备份、软件包更新、可选 OpenAI API 插件配置，并确保换目录、换机器或任务意外丢失后仍能继续。

## 2. 当前状态总览

- 阶段：2 / Host 工具与 ADB 连接确认完成；下一步设备 baseline 备份
- 当前任务文件：`tasks/03_device_baseline_backup.md`
- 当前交接：2026-06-02 下班前触发跨开发机保存；本项目目录将通过 GitHub 保存，回家后在新开发机拉取继续
- 设备连接：ADB 已重新连接成功，本次 serial 为 `axera-ax620e`；Host USB/PnP 层识别 `ax620e-adb`，实例 `USB\VID_32C9&PID_2003\AXERA-AX620E`
- 主机环境：本次观察为 Windows 10 + PowerShell 7.5.5；必须兼容 Windows 10 与 Linux 发行版
- 设备系统版本：Ubuntu 22.04 LTS；hostname `m5stack-LLM`；kernel `Linux 4.19.125` aarch64（本次 ADB 只读观察）
- ADB：已安装官方 Android SDK Platform Tools 到 `C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe`；未改全局 PATH；通过 `ADB_BIN`/完整路径执行成功
- UART：Linux 默认登录终端已确认在 `ttyS0`，参数 `115200n8`；运行时映射为 `serial0` / `/soc/ax_uart@4880000` / MMIO `0x04880000`，对应系统 Log/调试串口 `DBG_TXD/DBG_RXD` 路径；M5-Bus `TRM_TXD/TRM_RXD` 仍视为 StackFlow/JSON 通信 UART，不作为默认登录终端
- SSH：未知
- 网络：当前无可用外网路由；`eth0` 存在但 `DOWN/NO-CARRIER`，无 `wlan*`，无 `usb0/rndis/ecm/ncm` USB 网络接口；USB gadget 仅配置 `ffs.adb`（2026-06-02 ADB 只读观察）
- apt 源：未知
- 已安装 llm 包：未知
- C/C++ 工具链：已确认 `build-essential`、`binutils`、`gcc/g++`、`make`、`libc6-dev`、`dpkg-dev` 存在；`gdb` 可执行文件存在但不是 dpkg 管理的 `gdb` 包；`gdbserver` 未安装（2026-06-02 ADB 只读观察）
- 当前风险：未备份现场状态前，不做任何会修改设备的操作

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
- 2026-06-02 ADB 只读复核：当前启用的 SoC UART 只有 `ax_uart@4880000` 和 `ax_uart@4881000`；`ttyS1` 对应 `4881000.ax_uart` 但未作为 console/getty。把登录终端移动到其他 UART 或 M5-Bus `TRM_TXD/TRM_RXD` 涉及 bootargs、systemd、设备树/pinmux 或硬件 Net-Tie，默认不实施。
- 2026-06-02 用户意图澄清：串口登录相关任务按“外部实体终端设备通过 UART 获得 Module LLM Linux 登录 shell”的应用场景理解，不按 Linux `tty1` 虚拟控制台术语字面处理。
- 2026-06-02 ADB 只读复核：设备内置 Ubuntu 22.04 LTS 上已安装 C/C++ 基础构建链：`build-essential 12.9ubuntu3`、`binutils 2.38-4ubuntu2.6`、`binutils-aarch64-linux-gnu 2.38-4ubuntu2.6`、`gcc/g++` 元包 `4:11.2.0-1ubuntu1`，实际 `gcc/g++ 11.4.0`，目标 `aarch64-linux-gnu`。
- 2026-06-02 ADB 只读复核：`/usr/bin/gdb` 存在，版本 `GNU gdb 9.2`，但 `dpkg-query gdb` 显示 `unknown ok not-installed` 且 `dpkg -S /usr/bin/gdb` 无归属；`gdbserver` 未安装。
- 2026-06-02 ADB 只读复核：当前系统镜像没有显示出通过 ADB 原生获取互联网的能力。证据：USB gadget functions 只有 `ffs.adb`；设备侧没有 USB 网络接口；`ip route get 1.1.1.1` 返回 Network is unreachable；`adb reverse --list` 返回 `error: closed`；设备端 `ping/curl/wget` 访问公网和 M5Stack repo 均失败。

## 5. 主机环境

每次会话都重新填写或更新为“本次观察值”，不要作为永久事实跨机器套用。

```text
OS: Microsoft Windows 10 专业版 10.0.19045, 64 位（本次观察）
Shell: PowerShell 7.5.5（本次观察）；脚本调用 Windows PowerShell 5.1
Codex version:
adb path: C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe（官方 ZIP 安装；未改全局 PATH）
adb version: Android Debug Bridge version 1.0.41, Version 37.0.0-14910828
adb devices -l output file: inventory/before/host-tools-20260602-164709.txt
USB permissions / driver state: Host PnP 已识别 ax620e-adb，WinUSB 驱动，见 inventory/before/usb-adb-device-20260602-163018.txt
ssh client: C:\Windows\System32\OpenSSH\ssh.exe, OpenSSH_for_Windows_9.5p1（本次观察）
serial tool:
network access:
```

## 6. 设备连接信息

不要记录真实密码或私钥。  
ADB serial 和 USB 端口只记录为本次会话观察值；换线、换口、重启、换主机后必须重新扫描。

```text
连接方式: USB-C / ADB（本次观察）
本次 ADB serial: axera-ax620e（本次观察；换线/重启后必须重新扫描）
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
- [x] 主机 OS 检查（本次观察：Windows 10 + PowerShell 7.5.5）
- [x] ADB 安装/版本检查（官方 Platform Tools 已安装，版本 37.0.0-14910828）
- [x] 当前 USB 设备扫描（Windows PnP 已识别 ax620e-adb）
- [x] 当前 ADB 设备扫描（唯一 `device` 状态设备：`axera-ax620e`）
- [x] ADB 连接确认（只读 shell 验证成功）
- [ ] 设备只读探测
- [ ] baseline 备份
- [ ] 网络/SSH 确认
- [ ] apt 源配置
- [ ] llm 包清单记录
- [ ] 可选 OpenAI API 插件
- [ ] 恢复方案验证

## 9. 当前卡点

ADB 已通过官方 Android SDK Platform Tools 安装并可用；本次仅通过完整路径/`ADB_BIN` 使用，未改全局 PATH。已确认唯一 ADB 设备 `axera-ax620e` 可进入 root shell。下一步是执行 `tasks/03_device_baseline_backup.md`，做只读 baseline 探测和 `/etc/apt` 备份；不要直接进入 apt/SSH 修改。回家后换开发机继续时，必须重新确认 Host OS、`adb`、`ssh` 和 `adb devices -l`，不能沿用本机路径或本次 serial。

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

## 11. 下一步

在新开发机上继续时，先从 GitHub 拉取仓库并进入 `M5_projs/ModuleLLM/EnvConfig/`，读取 `AGENTS.md` 和 `context/SESSION_HANDOFF.md`。随后识别 Host OS/shell，检查 `adb` 与 `ssh`，执行 `adb version`、`adb start-server`、`adb devices -l`；若只有一个 `device` 状态设备，再执行 `tasks/03_device_baseline_backup.md` 做只读探测与备份。设备侧仍只读，不修改配置。
