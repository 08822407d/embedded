# PLAN_CURRENT — 当前实施方案

版本：v0.3  
最后更新：2026-06-02

## 总原则

这不是“把 AX630C 当普通 Linux 小主机随便改”的任务，而是“在官方支持路径内，做可记录、可恢复的配置”。默认路径是：

文档确认 → 配置登记审计 → 当前主机 OS 识别 → ADB 安装/可用性检查 → 当前 USB/ADB 设备扫描 → ADB/UART/SSH 只读接入 → baseline 备份 → 网络与 apt 源 → 按需安装/更新 llm 包 → 可选 OpenAI API 插件 → 恢复与迁移验证。

## 阶段 0：任务目录和上下文固化

目标：确保 Codex 上下文不会只存在于会话里。

验收：

- `AGENTS.md` 存在且简短。
- `context/PROJECT_STATE.md`、`SESSION_HANDOFF.md` 可独立恢复任务。
- git 已初始化，并至少有一次初始提交。
- 未记录任何真实密码、token、私钥。

## 阶段 1：官方文档消化

目标：让 Codex 用官方资料建立操作边界。

必须阅读：

- Module LLM Kit 产品页
- Module LLM ADB/UART/SSH 连接调试
- Module LLM 软件包更新
- Module LLM 镜像底包更新
- OpenAI API 使用说明（仅可选）
- StackFlow GitHub（仅包名/框架背景，不默认编译）

验收：

- `context/DOCS_INDEX.md` 已补充“查证日期”和“已提取事实”。
- 高风险限制已写入 `AGENTS.md` 与 `RUNBOOK.md`。

当前增量结论（2026-06-02）：

- M5-Bus 上 Module LLM 的 RX/TX 是主控与模块通信 UART，面向 StackFlow/JSON API；默认 115200bps 8N1。
- Linux 登录终端 / 系统 Log 调试路径不是这组 M5-Bus 通信 UART，而是调试板或 Module13.2 LLM Mate 提供的系统 Log/调试串口路径。
- ADB 实测当前 Linux 登录终端为 `ttyS0`，kernel bootargs 为 `console=ttyS0,115200n8 earlycon=uart8250,mmio32,0x4880000`，systemd `serial-getty@ttyS0.service` 正在运行；该路径对应 `DBG_TXD/DBG_RXD` 系统 Log/调试串口。
- 当前不建议把登录终端重配置到 M5-Bus `TRM_TXD/TRM_RXD`；如需第二串口登录，只能另开任务验证 `ttyS1` 物理连线、服务占用和回滚路径。
- 后续串口终端类任务按应用场景反推配置项：外部带屏幕/键盘的开发板通过 UART 登录 Module LLM 的 Linux shell，重点检查物理 UART、电气层、串口参数、kernel console、systemd getty、认证、shell 和业务串口占用，而不是要求用户先给出准确 `tty` 术语。

## 阶段 1.5：配置登记审计

目标：把已采纳配置、检测命令、依赖、缺失时的正确配置步骤、未记录配置、替代方案和整体最佳方案集中维护。

固定入口：

- `context/CONFIG_REGISTRY.md`
- `tasks/09_config_registry_audit.md`

验收：

- 每个配置类任务能从 `CONFIG_REGISTRY.md` 找到对应配置项、检测方法和依赖。
- 发现未记录配置时，先登记证据和影响范围，再决定采纳/保留/修复/回滚。
- 阶段性整体最佳方案可从当前采纳方案和替代方案整理得出，但不会自动覆盖已采纳配置。

## 阶段 2：主机工具与连接探测

目标：确认当前主机能安全接入设备。主机可能是 Windows 10，也可能是 Linux，不能沿用上次主机事实。

本次观察（2026-06-02）：

- Host：Windows 10 专业版 10.0.19045，PowerShell 7.5.5。
- `ssh` 可用。
- PATH 中未找到 `adb`，因此尚不能执行 ADB server 启动和设备列表扫描。
- 2026-06-02 16:30 追加观察：Windows PnP 已识别 Module LLM 为 `ax620e-adb`，实例 `USB\VID_32C9&PID_2003\AXERA-AX620E`，驱动为 Microsoft WinUSB；但 `adb.exe` 仍不可用，所以不能继续 `adb devices -l` / `adb shell`。
- 2026-06-02 16:34 追加观察：已从 Android 官方 `https://dl.google.com/android/repository/platform-tools-latest-windows.zip` 下载并安装 Platform Tools 到当前用户目录；`adb.exe` 签名为 Google LLC 且验证有效；ADB 版本 `37.0.0-14910828`。
- 2026-06-02 16:36 追加观察：`adb devices -l` 显示唯一 `device` 状态设备 `axera-ax620e`；通过显式 `-s axera-ax620e` 执行只读 shell 命令成功，设备为 Ubuntu 22.04 LTS，hostname `m5stack-LLM`。
- 2026-06-02 16:47 追加观察：重新执行 ADB preflight，唯一在线设备仍为 `axera-ax620e`；显式 `-s axera-ax620e` 只读 shell 连接成功。

只读命令范围：

- Host OS：Linux 用 `uname -a`；Windows PowerShell 用 `$PSVersionTable`。
- Host ADB：`adb version`、`adb start-server`、`adb devices -l`。
- Host SSH：`ssh -V`。
- Host USB：Linux 可用 `lsusb || true`；Windows 可用 `Get-PnpDevice -PresentOnly`。
- ADB shell：只有在选定目标 serial 后才执行 `uname -a`、`cat /etc/os-release`、`df -h`、`ip addr`、`apt list --installed | grep llm`。
- 串口只登录观察，不改配置。
- SSH 只验证登录和 `hostname`、`ip addr`。

强制规则：

- 每次会话、换线、换口、设备重启、换主机后，都重新执行 `adb devices -l`。
- 多个 ADB 设备时，必须使用 `ADB_SERIAL=<serial>` 或 `adb -s <serial>`，禁止裸 `adb shell/push/pull`。
- 若 `adb version` 失败，先停在主机工具安装/配置阶段，不继续设备操作。
- 新主机至少先做 Host ADB 工具检查和 USB/ADB 设备扫描。
- 若未发现 `adb`，先询问用户是否安装，不自动安装。
- 若发现已安装 `adb` 但 PATH 未配置，设置推荐环境变量 `ADB_BIN=<adb 绝对路径>` 后继续；持久写入环境变量前需用户确认。
- `adb` 配置正确后，记录版本并按可信来源检查/更新 Host 侧 Platform Tools。

验收：

- 输出保存到 `inventory/before/`。
- `PROJECT_STATE.md` 更新本次主机环境、ADB 状态、目标 serial 选择方式和系统状态。

## 阶段 3：baseline 备份

目标：修改前建立最小恢复依据。

备份对象：

- `/etc/os-release`
- `/etc/apt/sources.list`
- `/etc/apt/sources.list.d/`
- `/etc/apt/keyrings/`
- `/etc/hostname`
- `/etc/hosts`
- `/etc/ssh/sshd_config`（若存在）
- `dpkg -l`
- `apt list --installed`
- `systemctl list-units`（若 systemd 可用）
- `ps aux`
- `df -h`
- `ip addr`
- `ip route`
- `/opt/m5stack`、`/opt/m5stack/etc` 等存在则仅列目录，是否打包另行确认

验收：

- 备份文件存入 `backups/` 或 `inventory/before/`。
- `CHANGELOG.md` 记录备份时间与路径。

## 阶段 4：网络与 apt 源

目标：确保设备能访问 M5Stack apt 源，按官方路径更新软件包索引。

步骤：

1. 确认设备网络。
2. 备份 apt 配置。
3. 配置 M5Stack StackFlow apt 源。
4. `apt update`
5. `apt list | grep llm`
6. 不安装模型包，除非用户确认空间和目标。

验收：

- `apt update` 成功。
- 可用 llm 包列表保存到 `inventory/after/`。

## 阶段 5：按需软件包安装

目标：只安装当前目标必需的软件包。

默认最小包：

- `lib-llm`
- `llm-sys`

可选功能包：

- `llm-llm`
- `llm-whisper`
- `llm-melotts`
- `llm-openai-api`
- 其他模型包按需安装

验收：

- 安装前记录磁盘空间。
- 安装后记录 `dpkg -l | grep llm`。
- 重启前说明影响，重启后验证服务/命令。

## 阶段 6：可选 OpenAI API 插件

目标：仅在用户需要 PC/其他设备通过 OpenAI 标准 API 访问 Module LLM 时启用。

验收：

- 必需包安装完成。
- 设备重启后 API 服务可访问。
- PC 端测试脚本不写真实 API key；本地设备 base_url 另行记录。

## 阶段 7：恢复与迁移

目标：换机器/丢会话后能继续。

GitHub 交接触发：

- 如果用户明确说明当前开发机任务需要保存、下次任务更换开发机、要在另一台开发机继续，视为通过 GitHub 保存当前工作并在新开发机拉取继续。
- 触发后必须先刷新 `PROJECT_STATE.md`、`PLAN_CURRENT.md`、`SESSION_HANDOFF.md`、`CHANGELOG.md`，涉及配置时同步 `CONFIG_REGISTRY.md`。
- 不能只给聊天总结；必须把当前阶段、关键证据路径、下一步、风险和依赖写入可随仓库同步的文件。

验收：

- `SESSION_HANDOFF.md` 是最新的。
- 有 git commit。
- GitHub remote/auth 可用时已 push；不可用时记录阻塞原因。
- 可选生成 `backups/project.bundle` 作为离线备用。
- 新机器只需 clone/pull 后阅读恢复提示即可继续。

## 阶段 8：周期性整体方案复核

目标：在需求增长或方案取舍变化后，复核 `CONFIG_REGISTRY.md` 的替代方案和“当前整体最佳方案”。

验收：

- 采纳方案、未采纳方案、取舍原因、依赖和回滚路径都可追溯。
- 若整体最佳方案变化，`CHANGELOG.md` 记录变化原因。
