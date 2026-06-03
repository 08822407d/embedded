# DOCS_INDEX — 官方文档索引与已提取事实

最后查证：2026-06-02  
原则：优先使用 M5Stack 官方文档、M5Stack GitHub、OpenAI Codex 官方文档。第三方帖子只作参考，不作为依据。

## 1. M5Stack 官方入口

### Module LLM Kit 产品页

```text
https://docs.m5stack.com/zh_CN/module/Module%20LLM%20Kit
https://docs.m5stack.com/en/module/Module%20LLM%20Kit
https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/module/Module%20LLM/Sch_M5_Module-LLM.pdf
https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1131/K144_Sch_Module13.2_LLM_MATE.pdf
```

已提取事实：

- SKU：K144。
- Kit 包含 Module LLM 与 Module13.2 LLM Mate。
- SoC：AX630C。
- 内存：4GB LPDDR4，其中 1GB 系统内存 + 3GB 硬件加速专用内存。
- 存储：32GB eMMC5.1。
- 通信：串口通信，默认 115200@8N1。
- 支持 ADB 调试。
- 官方称内置 Ubuntu 系统。
- Type-C / RJ45 / FPC-8P / M5-Bus / HT3.96*9P 等接口和 Mate 板相关。
- 2026-06-02 复核：Module LLM Kit 产品页 M5-Bus 表中 Module LLM 的串口相关信号标为 `TRM_TXD(NT)` / `TRM_RXD(NT)`，`NT` 引脚可通过引脚切换方式适配不同主控。
- 2026-06-02 复核：Module LLM Kit 产品页 M5-Bus 表显示 Module LLM 的 I2C 为 `SoC_SCL` 在 pin 17、`SoC_SDA` 在 pin 18；Module13.2 LLM Mate 表显示 `SCL` 在 pin 17、`SDA` 在 pin 18。该方向与标准 M5-Bus / CoreS3 / Fan v1.1 的 `SDA` pin 17、`SCL` pin 18 相反。
- 2026-06-02 注意：M5Stack 官方不同页面/原理图之间存在不一致。Module LLM Kit 页面显示 17/18 反向；Module LLM 单品页和部分原理图文本提取显示 `G21/SYS_SDA` pin 17、`G22/SYS_SCL` pin 18。当前实机叠插 Fan v1.1 后 `0x18` 不应答，支持“默认叠插未形成可用 I2C”这一现场判断。
- 2026-06-02 复核：Module LLM 原理图中 `DBG_TXD/DBG_RXD` 与 `TRM_TXD/TRM_RXD` 分属不同信号；`DBG_TXD/DBG_RXD` 接到 FPC-8P 调试/系统 Log 路径，`TRM_TXD/TRM_RXD` 接到 M5-Bus 可切换通信引脚。
- 2026-06-02 复核：Module LLM 原理图的 AX630C UART 区域同时标出 `UART0_RXD/UART0_TXD`、`UART1_RXD/UART1_TXD` 以及 `DBG_TXD/DBG_RXD`、`TRM_TXD/TRM_RXD`；结合 ADB 实测 `serial0=/soc/ax_uart@4880000`、`ttyS0` 为 kernel console，可判断默认 Linux 登录终端走 `DBG_TXD/DBG_RXD` 调试路径。
- 支持 apt 快速更新软件和模型包。
- 模型不是普通通用格式；M5 文档称 AXERA 特有格式，不能直接使用市面现有模型。

### ADB / UART / SSH 连接调试

```text
https://docs.m5stack.com/zh_CN/stackflow/module_llm/config
```

已提取事实：

- ADB 可用于进入终端和文件传输。
- USB 连接 Module LLM 的 Type-C 到电脑后，可用 `adb shell` 进入设备。
- 官方文档要求操作前按当前操作系统下载 ADB Platform-Tools。
- 本任务不假设 USB 端口或 ADB serial 固定；每次会话都必须重新执行 `adb devices -l`。
- UART 默认 115200 bps 8N1。
- UART/SSH 默认账户为 root，默认密码为 123456；首次配置应改密，不要把新密码写入仓库。
- SSH 前需要通过 ADB/UART 或设备终端查看 IP，例如 `ip addr`。
- 2026-06-02 复核：官方连接调试页的 UART 登录终端描述是“接入调试板并连接系统 Log 调试接口”，不是 M5-Bus 主控通信 RX/TX。
- 2026-06-02 ADB 实机复核：当前镜像 `/proc/cmdline` 使用 `console=ttyS0,115200n8 earlycon=uart8250,mmio32,0x4880000`，`serial-getty@ttyS0.service` 为 active/running；因此官方 UART 登录说明对应系统 `ttyS0` / 系统 Log 调试串口。

### Module LLM 串口通信 / 引脚切换

```text
https://docs.m5stack.com/zh_CN/guide/llm/llm/pins_change
https://docs.m5stack.com/en/stackflow/module_llm/api
https://docs.m5stack.com/zh_CN/stackflow/module_llm/arduino_api
```

已提取事实：

- Module LLM 通过 UART 与 M5 主控交互，数据包为 JSON 格式。
- 该通信 UART 默认接口参数为 115200 bps 8N1。
- `M5ModuleLLM.begin(Stream *targetPort)` 初始化的是 Module LLM UART 接口，`checkConnection()` / `sys.ping` 通过该 UART 检查模块响应。
- 引脚切换教程明确这组 RX/TX 是“通信引脚”；例如 CoreS3 + Module LLM 默认出厂通信引脚为 G18/G17。
- 结论：M5-Bus 上 `TRM_TXD/TRM_RXD` / 主控侧 `UART_TX/UART_RX` 是 StackFlow/JSON API 通信用 UART，不是默认 Linux shell 登录终端；系统 Log/调试终端走调试板或 Module13.2 LLM Mate 的系统 Log/调试串口路径。

### Module Fan v1.1

```text
https://docs.m5stack.com/zh_CN/module/Module%20Fan%20v1.1
https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1125/M013_v11_Schematic.pdf
https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1125/Module-FAN_Protol_ZH.pdf
https://docs.m5stack.com/en/arduino/projects/module/module_fan_v1.1
```

已提取事实：

- SKU：M013-V11。
- 内置 MCU：STM32F030F4P6。
- 通信方式：I2C，默认地址 `0x18`。
- M5-Bus 使用：`SDA` 在 pin 17，`SCL` 在 pin 18，另需电源/地；风扇自身 `FanCtr/PWM` 与 `FG_OUT` 由模块内 STM32 连接和处理，不直接作为 M5-Bus 控制脚给主控。
- 与 Module LLM Kit M5-Bus I2C 对照：Fan v1.1 是 `SDA` pin 17、`SCL` pin 18；Module LLM Kit 页面显示 LLM/Mate 是 `SCL` pin 17、`SDA` pin 18，正好相反。未改焊盘/未用转接时，Fan 叠插在 LLM Kit 上很可能无法通过默认 M5-Bus I2C 通信；这可解释实机 `0x18` 不应答。
- 与 Module LLM M5-Bus UART 对照：这仍不属于 UART 脚冲突；问题是 I2C 的 SDA/SCL 位置疑似相反。UART `TRM_TXD/TRM_RXD` 仍在独立的 `NT` 引脚组。
- I2C 协议寄存器：
  - `0x00` R/W：风扇工作状态，`0` 停止，`1` 工作。
  - `0x10` R/W：PWM 频率，`0=1kHz`、`1=12kHz`、`2=24kHz`、`3=48kHz`。
  - `0x20` R/W：PWM 占空比，`0~100`。
  - `0x30` R：风扇 RPM，低字节/高字节。
  - `0x40` R：风扇 FG 信号频率，低字节/高字节。
  - `0xF0` 写回 Flash/固件版本/地址相关寄存器；避免频繁写 Flash。
- Arduino 示例通过库函数 `setPWMDutyCycle()`、`setPWMFrequency()`、`setStatus()` 控制转速/启停，并用 `getRPM()` 读取转速；示例中的 CoreS3 GPIO 不应直接套用到 AX630C Linux。

### 软件包更新

```text
https://docs.m5stack.com/zh_CN/stackflow/module_llm/software
```

已提取事实：

- Module LLM 使用 apt 安装和升级应用/功能单元。
- M5Stack apt 源示例：
  - GPG key：`https://repo.llm.m5stack.com/m5stack-apt-repo/key/StackFlow.gpg`
  - apt 源：`deb [arch=arm64 signed-by=/etc/apt/keyrings/StackFlow.gpg] https://repo.llm.m5stack.com/m5stack-apt-repo jammy ax630c`
- `llm-model-name` 形式通常是模型包，`llm-name` 形式通常是功能单元包。
- 模型包可能占用较大空间，应按需安装。
- `lib-llm` 提供运行环境，`llm-sys` 提供 StackFlow 基础功能。

### 镜像底包更新 / 刷机

```text
https://docs.m5stack.com/zh_CN/stackflow/module_llm/image
```

已提取事实：

- 镜像底包更新用于整机系统升级或系统损坏。
- 烧录工具目前仅支持 Windows 平台。
- 这是高风险恢复/升级路径，不属于日常配置默认步骤。
- 官方注意事项明确禁止尝试对 `/dev/mmcblk0` 进行分区操作。
- `/dev/mmcblk0` 是板载 eMMC，默认系统磁盘；错误分区可能导致几乎无法在线修复。

### OpenAI API 使用说明（可选）

```text
https://docs.m5stack.com/zh_CN/stackflow/openai_api/intro
```

已提取事实：

- 安装 `llm-openai-api` 后可通过标准 OpenAI API 访问设备模型能力。
- 官方教程示例依赖包包括 `llm-sys llm-llm llm-vlm llm-whisper llm-melotts llm-openai-api`。
- 安装后通常需要重启设备。

## 2. StackFlow 资料

```text
https://github.com/m5stack/StackFlow
```

已提取事实：

- StackFlow 面向 AXERA 加速平台，芯片平台包括 ax630c 和 ax650n。
- 系统要求为 Ubuntu。
- 本项目默认不编译 StackFlow；仅在需要理解包、配置、API 时阅读。

## 3. OpenAI Codex 官方文档

```text
https://developers.openai.com/codex/guides/agents-md
https://github.com/openai/codex
```

已提取事实：

- Codex 会在开始工作前读取 `AGENTS.md`。
- Codex 说明链包括全局和项目级文件；越靠近当前目录的项目说明越晚出现，优先级越高。
- 默认项目说明合并大小限制为 32 KiB，可通过配置调整。
- 因此本项目把 AGENTS.md 保持短小，详细上下文放在 `context/` 并在 AGENTS 中要求显式读取。


## 4. Android / ADB 官方文档

```text
https://developer.android.com/tools/adb
```

已提取事实：

- `adb` 包含在 Android SDK Platform Tools 中；也可使用 standalone SDK Platform Tools。
- Host 端 ADB client、Host 端 ADB server、设备端 adbd 共同工作。
- 可用 `adb devices -l` 查询当前连接设备，`-l` 会显示设备描述，便于区分多设备。
- 多设备时，必须用 `adb -s <serial> ...` 指定目标；否则不带目标的命令可能报错或误操作。
- `adb kill-server` / `adb start-server` 是 Host 侧 ADB server 维护动作，可用于连接异常排查。
