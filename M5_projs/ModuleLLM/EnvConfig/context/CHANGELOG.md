# CHANGELOG

## 2026-06-08 — M5-Bus ttyS1 登录口改为 921600 8N1

- 当前 Windows Host ADB preflight 成功，Platform Tools 版本 `37.0.0-14910828`，唯一在线设备为本次观察值 `axera-ax620e`。
- 改动前确认 ttyS1 使用 115200 8N1、`serial-getty@ttyS1.service` enabled/active、`llm-sys.service` masked/inactive。
- 使用伪终端只读验证设备 `stty` 支持 921600。
- 改动前设备备份：`/root/m5bus-uart-login-backup-20260608-142700`；已拉回 `backups/m5bus-uart-login-backup-20260608-142700/`。
- 将 `/etc/systemd/system/serial-getty@ttyS1.service.d/override.conf` 的 `agetty` 波特率改为 921600，保留 8N1、无硬件流控和 `--autologin root`。
- 实时验证和模块重启后的强校验均通过：`stty` 为 `speed 921600 baud`、`cs8`、`-cstopb`、`-parenb`、`-crtscts`；getty enabled/active；`llm-sys` masked/inactive；结果 `VERIFY=PASS`。
- 未修改 `ttyS0`、kernel bootargs、earlycon、DTB、分区、网络、SSH、密码或 apt 配置。
- 更新 `scripts/enable_m5bus_uart_login_adb.sh`：支持 `BAUD` 参数，当前默认 921600，状态快照包含 ttyS1 的 `stty` 参数；保留 ttyS1 登录但回退旧速率可用 `BAUD=115200 APPLY=1`。
- 新增 `.gitattributes`，强制本脚本使用 LF；已修复 Windows 工作树中的 CRLF，并通过 Git Bash `bash -n` 语法检查。
- 证据：`inventory/before/m5bus-uart-921600-preflight-20260608-142242.txt`、`inventory/before/m5bus-uart-921600-readonly-20260608-142430.txt`、`inventory/before/m5bus-uart-921600-backup-20260608-142700.txt`、`inventory/after/m5bus-uart-921600-apply-20260608-142718.txt`、`inventory/after/m5bus-uart-921600-reboot-preflight-20260608-142754.txt`、`inventory/after/m5bus-uart-921600-reboot-verify-20260608-142828.txt`。

## 2026-06-02 — 启用 M5-Bus UART 实体终端登录

- 用户决定后续不再关注从 LLM 控制 Fan 模块，转而要求让外部带屏/键盘嵌入式开发板作为实体串口终端，通过 M5-Bus UART 登录 Module LLM 并进行 shell 交互。
- 本轮保留默认 `ttyS0` / `DBG_TXD/DBG_RXD` 系统 Log/调试登录路径，不迁移 kernel console 或 earlycon。
- ADB preflight 成功，当前唯一 `device` 状态设备为 `axera-ax620e`。
- 只读确认：`ttyS1` 映射到 `/soc/ax_uart@4881000` / `serial1`，是 M5-Bus `TRM_TXD/TRM_RXD` 的候选 Linux UART；`ttyS0` 仍为 kernel console。
- 只读确认：`/opt/m5stack/bin/llm_sys` 默认持有 `/dev/ttyS1`，对应 `llm-sys.service`。因此把 M5-Bus UART 改作登录口必须牺牲 StackFlow/JSON 主控通信。
- 新增脚本 `scripts/enable_m5bus_uart_login_adb.sh`，支持 dry-run、实际启用和 rollback。
- 已执行实际启用：停用/禁用 `llm-sys.service`，写入 `/etc/systemd/system/serial-getty@ttyS1.service.d/override.conf`，启用并启动 `serial-getty@ttyS1.service`。
- 后验验证：`llm-sys.service` 为 inactive/dead 且 disabled；`serial-getty@ttyS1.service` 为 active/running 且 enabled；`/dev/ttyS1` 为 115200 baud、8N1、无硬件流控。
- 登录认证确认：当前 `ttyS1` 使用 `--autologin root`，进程为 `/bin/login -f` 和 `-bash`；外部实体终端接入后不需要输入账号/密码即可获得 root shell。
- 用户随后要求重启 Module LLM。第一次重启后发现：`serial-getty@ttyS1` 自动恢复，但 `llm-sys.service` 虽 disabled 仍被其他 `llm-*` 服务依赖拉起，并重新打开 `/dev/ttyS1`。
- 进一步确认 `llm-sys.service` 的 `RequiredBy` 包括 `llm-asr`、`llm-audio`、`llm-camera`、`llm-kws`、`llm-llm`、`llm-melotts`、`llm-skel`、`llm-tts`、`llm-vlm`、`llm-yolo`。单纯 disable 不足以阻止开机依赖启动。
- 已修正为 mask `llm-sys.service`。第二次重启验证通过：`llm-sys.service` 为 masked/inactive，`serial-getty@ttyS1.service` 为 active/running，`/dev/ttyS1` 只由 `/bin/login -f` 和 `-bash` 占用，配置永久且开机自动生效。
- 设备侧备份目录：`/root/m5bus-uart-login-backup-20260602-221439`。
- 回滚命令：`MODE=rollback APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>`。
- 输出保存：
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

## 2026-06-02 — 查阅 Fan Module v1.1 控制与 M5-Bus 引脚冲突

- 用户说明已在 Module LLM 上叠插 M5Stack Fan Module v1.1，要求先查资料确认如何控制转速，以及所需引脚是否与 M5-Bus UART 引脚重叠。
- 查阅资料和本地上下文后，进一步通过 ADB 对设备 I2C adapter 做只读探测。
- 官方资料结论：Fan Module v1.1 内置 STM32F030F4P6，通过 I2C `0x18` 控制；M5-Bus 使用 pin 17 `SDA`、pin 18 `SCL`。
- I2C 协议结论：`0x00` 控制启停，`0x10` 控制 PWM 频率，`0x20` 控制 PWM 占空比，`0x30` 可读 RPM。
- 修正对照结论：标准 M5-Bus / CoreS3 / Fan v1.1 为 pin 17 `SDA`、pin 18 `SCL`；Module LLM Kit 页面显示 Module LLM / Module13.2 LLM Mate 为 pin 17 `SCL`、pin 18 `SDA`，疑似正好相反。
- 因此当前问题不是 UART 脚冲突，而是 I2C 的 SDA/SCL 位置疑似相反。未改焊盘/未做交叉转接时，Fan v1.1 默认叠插很可能无法通过 M5-Bus I2C 被 LLM 控制。
- 实机只读探测：当前 Linux 有 `/dev/i2c-0`、`/dev/i2c-2`、`/dev/i2c-4`；Fan 默认地址 `0x18` 在三条总线上均未应答。
- 读模式全地址扫描只看到 `i2c-4` 上 `0x30 lp5562` 和 `0x47 sgm7220`，未发现 Fan 模块。
- 新增 `scripts/fan_temp_control_adb.sh`，按近似 Raspberry Pi 5 风扇曲线设计：`<50C=0%`、`50C=30%`、`60C=50%`、`67.5C=70%`、`75C=100%`，5C 回差。
- 运行短测时脚本未检测到 `0x18`，按设计退出，未写 Fan `0x00/0x10/0x20` 控制寄存器。
- 更新 `DOCS_INDEX.md` 和 `CONFIG_REGISTRY.md`，新增 Fan Module v1.1 资料和实机探测结论。
- 输出保存：
  - `inventory/before/m5bus-i2c-fan-scan-20260602-215510.txt`
  - `inventory/before/fan-temp-control-attempt-20260602-215650.txt`

## 2026-06-02 — 新增 5 秒间隔 SoC 温度读取脚本

- 按用户要求尝试在 Module LLM 上持续读取 SoC 温度；本轮仅执行 ADB 只读命令，未修改设备配置。
- ADB preflight 成功，当前唯一 `device` 状态设备为 `axera-ax620e`。
- 设备侧发现温度接口：`/sys/class/thermal/thermal_zone0/type=soc_thm`，`/sys/class/thermal/thermal_zone0/temp` 为毫摄氏度。
- 新增 Host 侧脚本 `scripts/read_soc_temp_adb.sh`，默认 `INTERVAL=5`、`COUNT=0`，通过 ADB 每 5 秒持续读取一次 SoC 温度。
- 验证命令：`COUNT=3 INTERVAL=5 bash scripts/read_soc_temp_adb.sh axera-ax620e`。
- 验证输出显示每 5 秒输出一次，例如 `raw=42350 temp=42.350 C`。
- 观察到输出时间戳来自设备侧，当前显示为 `2023-08-22`，与 Host 当前日期不一致；已登记为现场事实，本轮不修改设备时间。
- 输出保存：
  - `inventory/before/soc-temp-interface-scan-20260602-192732.txt`
  - `inventory/before/soc-temp-read-20260602-192730.txt`

## 2026-06-02 — Linux Host ADB 权限修复后连接成功

- 用户已在本机终端授权执行建议的 Host 侧 udev 修复命令。
- 复查 `/etc/udev/rules.d/51-m5stack-axera-adb.rules`：规则已写入，匹配 `32c9:2003`，设置 `MODE="0660"`、`GROUP="plugdev"`、`TAG+="uaccess"`。
- 当前 USB 节点 `/dev/bus/usb/001/015` 已变为 `root:plugdev 0660`。
- 重新执行 ADB preflight：`adb version`、`adb start-server`、`adb devices -l` 均成功；`adb devices -l` 显示唯一 `device` 状态设备 `axera-ax620e`。
- 使用显式 serial 执行只读 shell 验证成功：`whoami=root`，hostname `m5stack-LLM`，Ubuntu 22.04 LTS，kernel `4.19.125` aarch64。
- 本轮只修复 Host 侧 ADB USB 权限，未修改 Module LLM 设备配置。
- 输出保存：`inventory/before/adb-reconnect-success-20260602-192018.txt`。
- 下一步：执行 `tasks/03_device_baseline_backup.md` 做只读 baseline 和 `/etc/apt` 备份。

## 2026-06-02 — 尝试恢复当前 Linux Host ADB 连接

- 按用户要求尝试重新连接并排查无法连接原因；本轮只处理 Host 侧连接，不修改 Module LLM 设备配置。
- 重新启动 ADB server 后，`adb devices -l` 仍显示 `axera-ax620e no permissions`，排除单纯 ADB server 卡住。
- 已确认 Host 侧 `adb` 可用，用户 `cheyh` 已在 `plugdev` 组，`~/.android/adbkey` 存在，登录会话为 active seat。
- 已确认 USB 物理枚举正常：kernel 和 `lsusb` 均识别 `32c9:2003 axera ax620e-adb`，serial `axera-ax620e`。
- 已定位直接原因：当前 USB 节点 `/dev/bus/usb/001/015` 为 `root:root 0664`；现有 udev 规则未覆盖 M5Stack/AXERA vendor/product `32c9:2003`。
- 新增 Host 修复脚本 `scripts/fix_m5stack_adb_udev.sh`：默认 dry-run；`APPLY=1` 时写入 `/etc/udev/rules.d/51-m5stack-axera-adb.rules` 并修正当前 USB 节点权限。
- 尝试应用脚本时被 sudo 交互密码阻止：`sudo: a terminal is required to read the password`；因此尚未写入 Host udev 规则。
- 输出保存：`inventory/before/adb-linux-permission-diagnosis-20260602-190543.txt`。

## 2026-06-02 — 读取项目上下文并执行当前 Linux Host 门禁

- 按项目启动规则读取 `PROJECT_STATE.md`、`PLAN_CURRENT.md`、`CONFIG_REGISTRY.md`、`DOCS_INDEX.md`、`RUNBOOK.md`、`SESSION_HANDOFF.md` 和 `tasks/*.md`。
- 当前任务脉络确认：项目目标是通过 ADB/UART/SSH 对 M5Stack Module LLM Kit（AX630C）做安全、可恢复、可审计的 Linux/apt/网络/SSH 配置；当前入口仍是 `tasks/03_device_baseline_backup.md`。
- 当前 Linux Host 观察：Ubuntu 24.04.4 LTS + zsh；`adb` 位于 `/usr/bin/adb`，版本 `34.0.4-debian`；`ssh` 可用。
- 当前 USB/ADB 观察：`lsusb` 识别 `32c9:2003 axera ax620e-adb`；`adb devices -l` 显示 `axera-ax620e no permissions`，不是可操作的 `device` 状态。
- 按 ADB 硬规则，本轮未执行 `adb shell/push/pull`，未接触设备配置，未执行 baseline。
- 输出保存：`inventory/before/host-tools-20260602-184723.txt`。
- 下一步：用户确认后处理 Linux Host 侧 ADB USB 权限，或换到能显示 `device` 状态的主机；随后再执行 `tasks/03_device_baseline_backup.md`。

## 2026-06-02 — 下班前跨开发机保存准备

- 用户明确要求：当前开发机上的任务需要保存，回家后在家中开发机继续当前工作。
- 按跨开发机 GitHub 交接规则处理：刷新 `SESSION_HANDOFF.md`、`PROJECT_STATE.md` 和本变更日志。
- 本次保存范围限定为 `M5_projs/ModuleLLM/EnvConfig/`，包含规则、上下文、任务书、脚本和必要 `inventory/` 证据输出。
- 同一 Git 仓库中检测到 `Atom_Generic/Atom_IMU_test` 存在无关未提交改动；本次不纳入、不回滚、不修改。
- 当前任务阶段未变化：下一步仍是 `tasks/03_device_baseline_backup.md`，在新开发机上必须先重新检查 Host ADB 工具和 USB/ADB 设备列表。
- 本轮未接触设备，未修改设备配置。

## 2026-06-02 — 增加跨开发机 GitHub 交接规则

- 用户要求：如果明确说明当前开发机任务需要保存、下次任务更换开发机，应理解为通过 GitHub 保存当前工作，并在下一台开发机拉取全部内容继续。
- 更新 `AGENTS.md`：新增“跨开发机 GitHub 交接触发”规则，要求刷新可续接上下文、检查 git 状态和敏感信息，并在 remote/auth 可用时提交推送。
- 更新 `RUNBOOK.md`：新增跨开发机 GitHub 交接操作清单。
- 更新 `tasks/08_handoff_backup.md`：把换机继续场景定义为 GitHub 交接保存任务。
- 更新 `PLAN_CURRENT.md`、`PROJECT_STATE.md`、`SESSION_HANDOFF.md`：同步该规则，确保新会话可恢复。
- 本轮未接触设备，未执行 git commit/push。

## 2026-06-02 — 补充串口终端任务的应用场景解释规则

- 用户说明对 Linux 虚拟终端设备框架术语不熟悉，后续应以应用场景为准反推检查和修改项。
- 更新 `AGENTS.md`：当用户使用 Linux/UART/终端相关术语不精确时，不按字面术语缩小问题。
- 更新 `CFG-SERIAL-LOGIN-001`：明确目标场景是外部带屏幕/键盘的嵌入式开发板通过 UART 获得 Module LLM 的 Linux 登录 shell。
- 更新 `RUNBOOK.md`：增加外部独立终端设备检查顺序，覆盖物理 UART、电气电平、外部终端软件、Module LLM kernel console/getty、重配置风险。
- 未接触设备，未修改任何设备配置。

## 2026-06-02 — 确认 Linux 串口登录配置

- 通过 ADB preflight 重新确认当前唯一在线设备为 `axera-ax620e`，随后仅执行只读检查；未修改 bootargs、systemd、设备树、pinmux 或任何串口服务配置。
- 当前 Linux 登录串口配置：
  - kernel bootargs：`console=ttyS0,115200n8 earlycon=uart8250,mmio32,0x4880000`
  - kernel console：`/proc/consoles` 显示 `ttyS0`
  - systemd：`serial-getty@ttyS0.service` 为 active/running
  - 运行时映射：`ttyS0 -> /soc/ax_uart@4880000`，`ttyS1 -> /soc/ax_uart@4881000`
  - 设备树 alias：`serial0=/soc/ax_uart@4880000`，`serial1=/soc/ax_uart@4881000`
- SoC UART 节点状态：`ax_uart@4880000` 和 `ax_uart@4881000` 为 `okay`；`ax_uart@4882000`、`ax_uart@6080000`、`ax_uart@6081000`、`ax_uart@6082000` 为 `disabled`。
- 结论：当前应使用 `DBG_TXD/DBG_RXD` / 系统 Log 调试串口作为外部独立终端设备的登录连接路径，参数 `115200 8N1`；M5-Bus `TRM_TXD/TRM_RXD` 仍按 StackFlow/JSON 通信 UART 管理，不作为默认 Linux 登录终端。
- 已新增 `CFG-SERIAL-LOGIN-001` 到 `context/CONFIG_REGISTRY.md`，记录检测命令、合规判定、正确使用步骤、重配置边界和替代方案。
- 输出保存：
  - `inventory/before/serial-login-adb-preflight-20260602-173920.txt`
  - `inventory/before/serial-login-config-adb-20260602-174529.txt`
  - `inventory/before/serial-login-devicetree-adb-20260602-174558.txt`
  - `inventory/before/serial-uart-status-adb-20260602-174644.txt`

## 2026-06-02 — 新增配置登记与整体方案台账

- 新增 `context/CONFIG_REGISTRY.md`，集中记录：
  - 已采纳配置
  - 检测命令和合规判定
  - 配置依赖关系
  - 缺失配置的正确处理步骤
  - 未记录配置观察
  - 替代方案、取舍和当前整体最佳方案
- 新增 `tasks/09_config_registry_audit.md`，作为配置审计和整体方案整理入口。
- 更新 `AGENTS.md`：每轮必读加入 `CONFIG_REGISTRY.md`；每轮结束维护加入 `CONFIG_REGISTRY.md`。
- 更新 `RUNBOOK.md`：新增配置登记与审计固定流程。
- 更新 `README.md`、`PLAN_CURRENT.md`、`PROJECT_STATE.md`、`SESSION_HANDOFF.md`、`DECISIONS.md`。
- 更新 `tasks/00_intake.md` 到 `tasks/08_handoff_backup.md` 的阅读清单，加入 `context/CONFIG_REGISTRY.md`。
- 当前 `CONFIG_REGISTRY.md` 已登记 Host ADB、ADB 目标选择、设备联网路径、StackFlow apt 源、C/C++ 工具链和可选 OpenAI API 插件的初始配置项。

## 2026-06-02 — 检查 ADB 下的网络/互联网能力

- 通过 ADB 显式 serial `axera-ax620e` 执行只读网络检查；未修改路由、DNS、USB gadget、NAT、代理或 apt 配置。
- 当前设备网络接口：
  - `lo`：正常
  - `eth0`：存在，driver `stmmaceth`，但 `DOWN/NO-CARRIER`
  - `sit0`：DOWN
  - 未发现 `wlan*` 或 `usb0/rndis/ecm/ncm` 这类 USB 网络接口
- USB gadget 配置当前只有 `functions/ffs.adb`，`configs/c.1/ffs.adb` 链接到 ADB function；没有 RNDIS/ECM/NCM 网络 function。
- 当前设备无默认 IPv4 路由，`ip route get 1.1.1.1` 返回 `Network is unreachable`。
- DNS 使用 `127.0.0.53` systemd-resolved stub，但当前无上游网络，解析 M5Stack repo 失败。
- `adb forward --list` 为空；`adb reverse --list` 返回 `error: closed`。
- 设备端直接测试：
  - `ping 1.1.1.1`：Network is unreachable
  - `curl https://repo.llm.m5stack.com/`：无法解析主机
  - `wget https://repo.llm.m5stack.com/`：Temporary failure in name resolution
- 结论：当前系统镜像没有显示出“通过 ADB 原生获取互联网”的能力；ADB 当前提供 shell/文件/可能的端口级调试能力，不等同于通用网络接口或默认 Internet sharing。
- 输出保存：
  - `inventory/before/adb-network-capability-20260602-170131.txt`
  - `inventory/before/usb-gadget-adb-functions-20260602-170200.txt`
  - `inventory/before/device-internet-probe-adb-20260602-170221.txt`

## 2026-06-02 — 检查设备 C/C++ 工具链

- 通过 ADB 显式 serial `axera-ax620e` 执行只读检查；未执行 `apt update` / `apt install`，未编译测试程序，未修改设备配置。
- 已确认 apt/dpkg 安装状态：
  - `build-essential 12.9ubuntu3`
  - `binutils 2.38-4ubuntu2.6`
  - `binutils-aarch64-linux-gnu 2.38-4ubuntu2.6`
  - `gcc 4:11.2.0-1ubuntu1` / 实际 `gcc 11.4.0`
  - `g++ 4:11.2.0-1ubuntu1` / 实际 `g++ 11.4.0`
  - `make 4.3-4.1build1`
  - `libc6-dev:arm64 2.35-0ubuntu3.8`
  - `dpkg-dev 1.21.1ubuntu2.3`
- 已确认常用 binutils 命令存在：`ld`、`as`、`ar`、`ranlib`、`strip`、`objcopy`、`objdump`、`readelf`、`addr2line`、`nm`、`size`、`strings`。
- `gdb` 可执行文件存在于 `/usr/bin/gdb`，版本 `GNU gdb 9.2`，但不是 dpkg 管理的 `gdb` 包；`gdbserver` 未安装。
- `cmake`、`ninja-build`、`pkg-config` 未安装。
- 输出保存：
  - `inventory/before/toolchain-check-adb-20260602-165625.txt`
  - `inventory/before/gdb-origin-check-adb-20260602-165651.txt`

## 2026-06-02 — 重新连接 Module LLM ADB

- Host 侧设置本轮 `ADB_BIN` 为 `C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe`。
- 重新执行 ADB preflight：`adb version`、`adb start-server`、`adb devices -l`。
- 当前 `adb devices -l` 显示唯一在线设备：`axera-ax620e`，状态 `device`。
- 使用显式 `-s axera-ax620e` 执行只读 ADB shell 验证成功。
- 设备返回：
  - shell 用户：root
  - hostname：`m5stack-LLM`
  - 系统：Ubuntu 22.04 LTS
  - kernel：`Linux 4.19.125` aarch64
- 输出保存：
  - `inventory/before/host-tools-20260602-164709.txt`
  - `inventory/before/adb-connect-readonly-20260602-164730.txt`

## 2026-06-02 — 固化新环境 ADB 前置规则

- 将新环境最低前置检查固化到 `AGENTS.md`、`RUNBOOK.md`、`tasks/02_host_setup.md`、`README.md`、`scripts/README.md`：
  - 先做 Host ADB 工具检查。
  - 再做 USB/ADB 设备扫描。
  - 未发现 `adb` 时先询问用户是否安装。
  - 已安装但 PATH 未配置时，优先设置 `ADB_BIN=<adb 绝对路径>`。
  - 配置正确后记录 `adb version`，并按可信来源检查/更新 Host 侧 Platform Tools。
- 更新 PowerShell/Bash 脚本，使其优先使用 `ADB_BIN`，并在 PATH 与常见 Android SDK 路径中查找已安装 adb。
- 验证 PowerShell 脚本语法解析通过：`host_check.ps1`、`device_probe_adb.ps1`、`pull_apt_backup_adb.ps1`。
- 验证未设置 `ADB_BIN` 时，`host_check.ps1` 可从常见路径发现官方 `adb.exe`，并完成版本检查和 `adb devices -l`；输出保存到 `inventory/before/host-tools-20260602-164338.txt`。
- 当前 Windows `bash.exe` 指向 WSL，WSL 启动失败，未能完成 Bash 脚本语法检查。

## 2026-06-02 — 安装官方 ADB 并完成只读连接验证

- Host 侧安装 Android 官方 Platform Tools：
  - 来源：`https://dl.google.com/android/repository/platform-tools-latest-windows.zip`
  - 安装路径：`C:\Users\cheyh\AppData\Local\Android\Sdk\platform-tools\adb.exe`
  - ZIP SHA256：`4FE305812DB074CEA32903A489D061EB4454CBC90A49E8FEA677F4B7AF764918`
  - `adb.exe` SHA256：`957E46B8615F7AF5B7292A2DDABE98D2E61940C3FB2B0545756507F080613E71`
  - Authenticode：Google LLC，有效。
- 未修改全局 PATH；本轮通过完整路径 / `ADB_BIN` 使用 `adb.exe`。
- `adb version` 成功：Android Debug Bridge `1.0.41`，Platform Tools `37.0.0-14910828`。
- `adb devices -l` 显示唯一在线设备：`axera-ax620e`，状态 `device`。
- 使用显式 `-s axera-ax620e` 执行只读 shell 验证成功：
  - shell 用户：root
  - hostname：`m5stack-LLM`
  - 系统：Ubuntu 22.04 LTS
  - kernel：`Linux 4.19.125` aarch64
- 输出保存：
  - `inventory/after/host-adb-install-20260602-163412.txt`
  - `inventory/before/host-tools-20260602-163454.txt`
  - `inventory/before/adb-connect-readonly-20260602-163601.txt`

## 2026-06-02 — Host USB/ADB 侧探测

- Host 只读执行 `scripts/host_check.ps1`。
- 当前 Windows PnP 层已识别 Module LLM：`ax620e-adb`，实例 `USB\VID_32C9&PID_2003\AXERA-AX620E`。
- 驱动状态：`ADB Device`，Microsoft WinUSB，`winusb.inf`，驱动版本 `10.0.19041.1`。
- PATH、常见 Android SDK Platform Tools 路径、Downloads/Desktop 中均未找到 `adb.exe`；未能执行 `adb version` / `adb devices -l` / `adb shell`。
- 当前只确认 USB 侧正确，尚未完成 ADB 连接。
- 输出保存：
  - `inventory/before/host-tools-20260602-162913.txt`
  - `inventory/before/usb-adb-device-20260602-163018.txt`

## 2026-06-02 — M5-Bus UART 默认用途确认

- 只读复核 M5Stack 官方产品页、连接调试页、引脚切换教程、Arduino/API 文档和 Module LLM / Module13.2 LLM Mate 原理图。
- 确认 M5-Bus 上 Module LLM RX/TX 是主控通信 UART，面向 StackFlow/JSON API，默认 115200bps 8N1；不是默认 Linux shell 登录终端。
- 确认系统 Log/调试终端使用调试板或 Module13.2 LLM Mate 提供的系统 Log/调试串口路径。
- 本次 Host 观察：Windows 10 专业版 10.0.19045，PowerShell 7.5.5；PATH 中未找到 `adb`；`ssh` 可用。
- 保存 Host 预检输出：`inventory/before/host-adb-preflight-20260602-160720.txt`。
- 更新 `context/PROJECT_STATE.md`、`context/DOCS_INDEX.md`、`context/PLAN_CURRENT.md`、`context/SESSION_HANDOFF.md`。

## 2026-06-02 — v0.2

- 强化 Windows 10 / Linux 双主机环境要求。
- 增加“每次会话必须重新扫描 USB/ADB 设备”的硬规则。
- 增加 ADB preflight：`adb version`、`adb start-server`、`adb devices -l`。
- 明确多 ADB 设备时必须指定 `ADB_SERIAL` / `-s <serial>`，禁止裸 `adb shell/push/pull`。
- 更新 `AGENTS.md`、`RUNBOOK.md`、`PROJECT_STATE.md`、`PLAN_CURRENT.md`、`SESSION_HANDOFF.md`、`tasks/00_intake.md`、`tasks/02_host_setup.md`、`tasks/03_device_baseline_backup.md`。
- 新增 Windows PowerShell 脚本：
  - `scripts/host_check.ps1`
  - `scripts/device_probe_adb.ps1`
  - `scripts/pull_apt_backup_adb.ps1`
- 更新 Bash 脚本，加入 ADB serial 选择和多设备保护。

## 未初始化

- 创建任务书骨架。
- 尚未连接设备，尚未修改设备。
