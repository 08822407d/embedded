# SESSION_HANDOFF — 可恢复交接包

最后更新：2026-06-02

## 一句话目标

让 Codex 本地协助 M5Stack Module LLM Kit（AX630C）完成 ADB/UART/SSH/Linux/apt 配置，并把上下文、实施方案、设备状态和恢复路径保存在项目目录中，支持换目录、换机器或本地任务丢失后继续。

## 当前阶段

阶段 2：当前 Linux Host 的 ADB 权限已修复并连接确认完成；下一步执行 `tasks/03_device_baseline_backup.md` 做只读 baseline。

## 本次跨开发机交接

触发时间：2026-06-02 下班前  
触发原因：用户要回家后在家中开发机继续当前工作，需要通过 GitHub 保存并拉取完整项目内容。

保存范围：

- 保存本项目目录：`M5_projs/ModuleLLM/EnvConfig/`
- 包含任务规则、上下文、脚本、任务书和必要的 `inventory/` 证据输出。
- 不包含同一 Git 仓库内其他项目的未提交改动，例如 `Atom_Generic/Atom_IMU_test`。

新开发机续接步骤：

1. 从 GitHub 拉取仓库 `https://github.com/08822407d/embedded.git`。
2. 进入 `M5_projs/ModuleLLM/EnvConfig/`。
3. 先阅读 `AGENTS.md`、`context/SESSION_HANDOFF.md`、`context/PROJECT_STATE.md`、`context/CONFIG_REGISTRY.md`、`context/RUNBOOK.md`、`context/PLAN_CURRENT.md`、`context/DOCS_INDEX.md`。
4. 新主机不要沿用本机路径或旧 ADB serial；先执行 Host ADB 工具检查和 USB/ADB 设备扫描。
5. 如果已安装 `adb` 但不在 PATH，优先设置当前会话 `ADB_BIN=<adb 绝对路径>`；如果未安装，先询问是否安装。
6. 设备连接重新确认后，再继续 `tasks/03_device_baseline_backup.md`。

## 当前事实

- 设备：M5Stack Module LLM Kit（AX630C），待实机确认。
- 任务性质：Linux/ADB/运维配置，不是业务软件开发。
- 主机环境：可能在 Windows 10 与 Linux 发行版之间切换；必须每次重新识别。
- 本次主机观察：Ubuntu 24.04.4 LTS，zsh；`ssh` 可用；`adb` 位于 `/usr/bin/adb`，版本 `34.0.4-debian`。
- 本次 USB 观察：Linux `lsusb` 已识别 `32c9:2003 axera ax620e-adb`。
- 本次 ADB 观察：2026-06-02 19:20 重新执行 `adb devices -l`，唯一 `device` 状态 serial 为 `axera-ax620e`；通过显式 `-s axera-ax620e` 执行只读 shell 验证成功，hostname `m5stack-LLM`，Ubuntu 22.04 LTS，root shell。
- 当前 Linux ADB 权限修复：用户已授权执行 `scripts/fix_m5stack_adb_udev.sh`；Host 侧规则 `/etc/udev/rules.d/51-m5stack-axera-adb.rules` 已落地，匹配 `32c9:2003`，设备节点 `/dev/bus/usb/001/015` 为 `root:plugdev 0660`。
- ADB 规则：先 `adb version`、`adb start-server`、`adb devices -l`；不能假设 USB 端口或 ADB serial 固定。
- 新环境规则：至少先做 Host ADB 工具检查和 USB/ADB 设备扫描；未发现 `adb` 时先询问是否安装；已安装但环境变量未配置时设置 `ADB_BIN=<adb 绝对路径>`；配置正确后记录版本并按可信来源检查/更新 Host 侧 Platform Tools。
- 配置登记规则：所有配置类任务先查 `context/CONFIG_REGISTRY.md`；用登记的检测命令判断状态；缺失配置按依赖关系推进；未记录配置先登记证据和影响范围；替代方案和整体最佳方案集中维护。
- 用户意图解释规则：用户可能不熟悉 Linux `tty` / 虚拟终端 / console / getty 术语；串口终端类任务要以应用场景为准反推配置层，例如外部带屏幕键盘的开发板通过 UART 登录 Module LLM 的 Linux shell。
- 跨开发机保存规则：用户明确说当前开发机任务需要保存、下次换开发机或要在另一台开发机继续时，视为 GitHub 交接保存任务；必须刷新状态/交接/变更/配置登记文件，检查敏感信息和 git 状态，然后在 remote/auth 可用时提交并推送。
- 文档入口：见 `context/DOCS_INDEX.md`。
- 高风险红线：绝不主动对 `/dev/mmcblk0` 分区/格式化/写镜像；镜像底包更新另行确认。
- M5-Bus UART 结论：M5-Bus 上 Module LLM RX/TX 是主控通信 UART，面向 StackFlow/JSON API，默认 115200bps 8N1；系统终端/Log 调试串口是另一条调试板或 Module13.2 LLM Mate 路径。
- Linux 串口登录结论：当前镜像把 kernel console 和 systemd serial getty 配在 `ttyS0`，参数 `115200n8`；运行时映射为 `serial0` / `/soc/ax_uart@4880000` / MMIO `0x04880000`，对应 `DBG_TXD/DBG_RXD` 系统 Log/调试串口。已按用户目标新增 M5-Bus UART `/dev/ttyS1` 运行期登录口；`ttyS1` 对应 `/soc/ax_uart@4881000` / `TRM_TXD/TRM_RXD`，当前 `serial-getty@ttyS1.service` active/running，`llm-sys.service` masked/inactive。
- M5-Bus UART 登录认证结论：当前 `ttyS1` 使用 `--autologin root`，不需要输入账号/密码；外部实体终端接入后应直接得到 root shell。若要恢复 M5-Bus StackFlow/JSON 通信，执行 `MODE=rollback APPLY=1 bash scripts/enable_m5bus_uart_login_adb.sh <serial>`。
- SoC 温度读取结论：当前可通过 `/sys/class/thermal/thermal_zone0/temp` 读取 SoC 温度；`type=soc_thm`，数值为毫摄氏度。Host 脚本 `scripts/read_soc_temp_adb.sh` 默认每 5 秒持续读取。
- Fan Module v1.1 资料结论：用户已叠插 M5Stack Fan Module v1.1；资料显示其通过 I2C `0x18` 控制，Fan/标准 M5-Bus/CoreS3 使用 pin 17 `SDA`、pin 18 `SCL`。Module LLM Kit 页面显示 Module LLM/Mate 为 pin 17 `SCL`、pin 18 `SDA`，与 Fan 疑似正好相反；这不是 M5-Bus `TRM_TXD/TRM_RXD` UART 脚冲突。
- Fan Module v1.1 实机探测结论：当前 Linux 有 `/dev/i2c-0/2/4`，I2C 控制器启用；但 Fan 默认地址 `0x18` 在这些总线上均无应答，读模式扫描也未发现其他疑似 Fan 地址。因此本轮未写风扇控制寄存器。

## 已完成

- 已建立任务书骨架。
- 已复核 M5-Bus UART / 调试串口用途。
- 已保存本次 Host 预检输出：`inventory/before/host-adb-preflight-20260602-160720.txt`。
- 已保存本次 Host/ADB/USB 预检输出：`inventory/before/host-tools-20260602-162913.txt`。
- 已保存本次 USB ADB 设备详情：`inventory/before/usb-adb-device-20260602-163018.txt`。
- 已保存官方 Platform Tools 安装记录：`inventory/after/host-adb-install-20260602-163412.txt`。
- 已保存安装后 ADB preflight：`inventory/before/host-tools-20260602-163454.txt`。
- 已保存显式 serial 的只读 ADB 连接验证：`inventory/before/adb-connect-readonly-20260602-163601.txt`。
- 已保存脚本自动发现 `adb.exe` 的 Host 检查输出：`inventory/before/host-tools-20260602-164338.txt`。
- 已保存本轮 Host/ADB preflight：`inventory/before/host-tools-20260602-164709.txt`。
- 已保存本轮显式 serial 的只读 ADB 连接验证：`inventory/before/adb-connect-readonly-20260602-164730.txt`。
- 已执行只读设备命令：`whoami`、`id`、`hostname`、`uname -a`、`cat /etc/os-release`、`uptime`、`pwd`。
- 已更新脚本和文档，使新环境优先识别 `ADB_BIN` / PATH / 常见 SDK 路径，避免已安装但未进 PATH 时误判为缺失。
- 已新增 `context/CONFIG_REGISTRY.md` 和 `tasks/09_config_registry_audit.md`，用于配置检测、依赖、未记录配置、替代方案和阶段性整体最佳方案。
- 已新增 `CFG-SERIAL-LOGIN-001`，记录串口登录检测命令、默认配置、重配置风险和当前最佳接线方案。
- 已在 `CFG-SERIAL-LOGIN-001` 与 `RUNBOOK.md` 中补充“外部独立终端设备”场景解释和检查清单。
- 已在 `AGENTS.md`、`RUNBOOK.md`、`PLAN_CURRENT.md`、`tasks/08_handoff_backup.md` 中补充跨开发机 GitHub 交接规则。
- PowerShell 脚本解析验证通过；当前 Windows 的 `bash.exe` 指向 WSL 且 WSL 启动失败，本机未完成 Bash 脚本语法验证。
- 已保存当前 Linux Host/ADB/USB 预检输出：`inventory/before/host-tools-20260602-184723.txt`。
- 已保存当前 Linux ADB 权限诊断：`inventory/before/adb-linux-permission-diagnosis-20260602-190543.txt`。
- 已新增 Host 侧 udev 修复脚本：`scripts/fix_m5stack_adb_udev.sh`；当前用户已授权应用，Host 侧 udev 规则已写入 `/etc/udev/rules.d/51-m5stack-axera-adb.rules`。
- 已保存当前 Linux Host ADB reconnect 成功输出：`inventory/before/adb-reconnect-success-20260602-192018.txt`。
- 已新增 SoC 温度读取脚本：`scripts/read_soc_temp_adb.sh`。
- 已保存 SoC 温度接口扫描与三次读取验证输出：`inventory/before/soc-temp-interface-scan-20260602-192732.txt`、`inventory/before/soc-temp-read-20260602-192730.txt`。
- 已完成 Fan Module v1.1 控制资料复核并登记到 `DOCS_INDEX.md` / `CONFIG_REGISTRY.md`；尚未实机写 I2C 控制寄存器。
- 已新增温控脚本 `scripts/fan_temp_control_adb.sh`；脚本按近似 Raspberry Pi 5 风扇曲线设计，但当前因 `0x18` 未检测到而未实际控制。
- 已保存 I2C/Fan 探测与温控脚本短测输出：`inventory/before/m5bus-i2c-fan-scan-20260602-215510.txt`、`inventory/before/fan-temp-control-attempt-20260602-215650.txt`。
- 已新增并执行 M5-Bus UART 登录脚本 `scripts/enable_m5bus_uart_login_adb.sh`：`/dev/ttyS1` 已启用 `serial-getty@ttyS1.service`，`llm-sys.service` 已 mask/inactive；最终重启验证保存到 `inventory/after/llm-reboot-after-mask-verify-20260602-223640.txt`。
- 已确认 M5-Bus UART 登录不需要账号/密码，证据保存到 `inventory/after/m5bus-uart-login-auth-check-20260602-221842.txt`。
- 设备配置改动仅限 M5-Bus UART 登录：mask `llm-sys.service`，启用 `serial-getty@ttyS1.service`，新增 ttyS1 的 systemd drop-in；未修改 bootargs、dtb、分区、apt、网络、SSH 或密码。

## 未解决问题

- 当前使用的主机 OS / shell 本次已知，但不可跨会话沿用。
- 当前 Linux Host 的 `adb` 已在 PATH；如需显式指定，可设置 `export ADB_BIN=/usr/bin/adb`。
- 当前 USB 设备已识别；当前 ADB 设备已扫描，serial 为本次观察值 `axera-ax620e`，状态为 `device`。换线、重启、换主机后仍必须重新扫描。
- 设备基础系统版本已知，但尚未做完整 baseline。
- ADB 可用；UART 登录路径已通过 ADB 侧配置确认，但尚未进行物理串口线实际登录测试；SSH 是否可用未知。
- 设备网络状态：当前无默认路由；`eth0` 存在但 `DOWN/NO-CARRIER`；未发现无线网卡或 USB 网络接口；USB gadget 当前只有 ADB function。
- ADB 获取互联网能力：当前镜像未显示原生支持。`adb reverse --list` 返回 `error: closed`，设备端访问公网/M5Stack repo 失败。
- apt 源与已安装包状态未知。
- C/C++ 工具链状态已知：`build-essential`、`binutils`、`gcc/g++`、`make`、`libc6-dev`、`dpkg-dev` 已安装；`gdb` 可运行但不是 dpkg 管理包；`gdbserver` 未安装。
- 串口登录重配置边界：保留 `ttyS0`/`DBG_TXD/DBG_RXD` 为默认系统 Log/调试路径；已按用户目标额外启用 `ttyS1` / M5-Bus `TRM_TXD/TRM_RXD` 运行期 root 登录口。该配置牺牲 M5-Bus StackFlow/JSON 通信，且当前为免账号密码 root 自动登录。
- 用户最终目标配置清单待确认。

## 下一步建议

1. 让 Codex 读取 `AGENTS.md`、`PROJECT_STATE.md`、`CONFIG_REGISTRY.md`、`RUNBOOK.md`、`PLAN_CURRENT.md`、`DOCS_INDEX.md`。
2. 在新开发机上先识别 Host OS/shell，并检查 `adb`、`ssh`。
3. 重新执行 `adb version`、`adb start-server`、`adb devices -l`；不要沿用本机的 `axera-ax620e` 作为永久事实。
4. 重新执行 `adb devices -l`；若只有一个 `device` 状态设备，执行 `tasks/03_device_baseline_backup.md` 做只读探测并备份。
5. 若换主机或换 USB 后再次出现 Linux `no permissions`，用 `scripts/fix_m5stack_adb_udev.sh` 处理 Host 侧 udev 权限。
6. baseline 完成前不要修改 apt、网络、SSH、密码或系统配置。
