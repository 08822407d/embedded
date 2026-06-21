# FACTS — 已验证事实台账

> **用法规则**:本表只放**确凿、有来源**的事实(实测 / 用户确认 / 官方手册 / 仓库文件)。
> 调试反复失败时**先读这里,把表中条目当地面真值**,优先怀疑代码 / 接线 / 参数,**别无故怀疑已验证事实**。
> 每条标来源;若某条后来被推翻,改写并注明日期与新证据,不要默默删。

## A. 硬件 / 平台(来源:`platformio.ini` + 仓库文件,2026-06-21 核实)
- **F‑1** 目标板 = M5Stack **Unit-C6L**,MCU = **ESP32-C6**(RISC-V 单核 @160MHz,XTAL 40MHz,rev 0.02)。**板载射频(2026-06-21 真机启动自检实测)**:✅ 2.4GHz WiFi、✅ BLE(Low Energy)、✅ IEEE 802.15.4(Thread/Zigbee);❌ **无 Classic BT**;**无板载 PSRAM**。来源:`board=esp32-c6-devkitc-1` + 真机 `CORE_DEBUG_LEVEL=5` 自检(见 F‑13)。*用哪种射频做对讲仍待 REQUIREMENTS 定,但候选集已坐实:WiFi/ESP-NOW、BLE、802.15.4 均可用;LoRa 需外接模块(芯片不带)。*
- **F‑2** 构建:framework = **Arduino**;platform = **pioarduino fork**(`platform-espressif32` 的 `develop` 分支 zip)。来源:`platformio.ini`。
- **F‑3** 依赖库:**M5Unified**(github `m5stack/M5Unified`)。来源:`lib_deps`。
- **F‑4** 串口走**原生 USB-CDC**(`ARDUINO_USB_MODE=1`、`ARDUINO_USB_CDC_ON_BOOT=1`),非外置 USB-UART 桥;`monitor_speed=115200`,`upload_speed=1500000`,`CORE_DEBUG_LEVEL=5`。来源:`platformio.ini` build_flags。
- **F‑5** 工具链 PATH 修复:`scripts/fix_toolchain_path.py`(`pre:` extra_script)把 `toolchain-riscv32-esp` 的 `bin` 目录前置进 PATH,解决 PlatformIO 找不到 `riscv32-esp-elf-g++` 的问题。来源:该脚本。

## B. 代码 / 仓库现状(2026-06-21)
- **F‑6** `src/main.cpp` 仅骨架:`Serial.begin(115200)` + 打印 `"Unit C6L skeleton boot"`,`loop()` 空转。**无任何功能实现。**
- **F‑7** git:本项目目录 `M5_projs/Unit_C6L/C6L_WalkieTalkie` 在父仓库 `/home/cheyh/projs/embedded` 中**尚未入库(untracked)**。→ 安全网未激活,**首次 commit 前任何误删都找不回**。

## C. 实测参数(待补)
- *(物理参数 / 射频实测距离 / 音频延迟 / 续航等,实测后入账,标日期与测法)*

## D. 开发环境 / 联机(来源:用户 2026-06-21)
- **F‑8** 开发板经 USB **Type-C** 数据线连主机;Unit-C6L 走原生 USB-CDC(F‑4),枚举为 Espressif native-USB:VID `0x303A` / **PID `0x1001`(2026-06-21 实测确认)**,描述 `USB JTAG/serial debug unit`,Linux 下 `/dev/ttyACM*`。`pio device list` 的 `hwid` 里 **`SER=` 字段就是芯片 MAC**(实测有值)→ 可直接据此区分多块同型号板,通常无需 esptool。**注意**:VID:PID 在 C6/S3/C3/H2 上都是 `303A:1001`,**单凭它无法区分芯片型号**。
- **F‑9** 用户购入 **2 块 Unit-C6L**;通信测试时**两块都接**主机。也可能:只接 1 块(做与通信无关的功能),或 0 块(无需烧录 / 读日志的工作)。
- **F‑10** 开发主机**当前是 Ubuntu 24.04,但可能临时换到 Windows** → **不得假定串口名**(Linux 形如 `/dev/ttyACM*`,Windows 形如 `COM*`)。
- **F‑11** 主机上**不一定只连 Unit-C6L**,可能同时连其他 M5Stack 开发板 → 任何烧录 / 读串口前**必须先按 [`OPERATIONS.md`](OPERATIONS.md)『设备探测』主动确认**目标板在线、型号、数量,别假定"连着的就是 C6"。两块 C6 互为同型号,需区分时用 MAC / by-id 序列号(见 OPERATIONS)。**已应验**:2026-06-21 探测到 **3 个** `303A:1001` 设备,而用户只有 2 块 C6 → 确有第 3 块别的 Espressif 原生 USB 板共存。
- **F‑12 ⛔ 构建阻塞(2026-06-21)** pioarduino develop 平台 `espressif32@55.3.39+develop` 要求 **PlatformIO Core ≥ 6.1.19**;本机为 **6.1.18**(pip 用户安装,包在 `~/.local/lib/python3.12/site-packages`,exec `~/.local/bin/pio`)→ `pio run` 报 `IncompatiblePlatform`、平台装完即被卸载、**无法编译 / 烧录**。
  - **实测 `pio upgrade` 失败**:系统 Python(`/usr/bin/python`,Ubuntu 24.04)受 **PEP 668『externally-managed-environment』**保护,pip 拒绝安装。
  - **就地升级**(pio 是 `--user` 安装,只动 `~/.local`、不碰系统包):`python3 -m pip install --user -U platformio --break-system-packages`。
  - 或改用 **pipx / venv** 隔离安装(更干净,但会改变 pio 的路径与调用方式,需调 PATH)。
  - **✅ 已解决(2026-06-21)**:用 `--user --break-system-packages` 升至 **pio Core 6.1.19**,`pio run` 通过,esptool 正常生成 ESP32-C6 镜像(RAM 5.7% / Flash 30.2%)。
- **F‑13 ✅ 烧录+运行实测(2026-06-21)** 骨架烧入**两块 C6**(MAC `58:8C:81:50:04:38`@ttyACM2、`58:8C:81:50:06:E4`@ttyACM1)均 `[SUCCESS]`、`Hash of data verified`;第三块 `30:ED:A0:E2:E2:48`(ttyACM0)未碰。用 pyserial **脉冲 RTS 复位**后抓到完整启动日志,含 `Unit C6L skeleton boot` → upload→运行→串口链路全通。`CORE_DEBUG_LEVEL=5` 会在启动打印整板自检(Chip/Memory/Flash/Partitions/Board/Software Info),是 F‑1/F‑14 的数据来源。
- **F‑14 闪存/分区/版本(2026-06-21 实测)** 外部 Flash **16 MB**(QIO @80MHz);默认分区:nvs 20KB / otadata 8KB / **app0 1280KB / app1 1280KB(OTA 双区)** / spiffs 1408KB / coredump 64KB。ESP-IDF **v5.5.4**,Arduino-ESP32 **3.3.10**,变体 `esp32c6`(`Espressif ESP32-C6-DevKitC-1`)。

## E. LoRa 子系统(SX1262)— 来源:M5 官方文档/例程,2026-06-21
> 修正早先猜测:Unit-C6L **本身就带 LoRa**(SX1262 是 SPI 外挂芯片,故 MCU 自检看不到 ≠ 没有)。
- **F‑15** Unit-C6L = **Stamp C6LoRa 模组**:ESP32-C6 + **SX1262** LoRa 收发 + RF 开关 + LNA;频段 **868~923MHz**,**+22dBm** 发 / **−147dBm** 收;16MB Flash(对上 F‑14)。另带 0.66" OLED(SSD1306)、RGB(G2)、蜂鸣器(G11)、按键(经扩展器 P0)、**2×RP-SMA 天线口(WiFi / LoRa 各一)**。官方定位 Meshtastic。
- **F‑16 SX1262 接线(ESP32-C6 GPIO)**:SPI **MOSI=21 / MISO=22 / SCK=20 / NSS(CS)=23**;**BUSY=19**;**DIO1(IRQ)=7**。RadioLib 构造:`SX1262 radio = new Module(23, 7, RADIOLIB_NC, 19);`(CS, IRQ, RST=NC, BUSY)。
- **F‑17 LoRa 控制线经 I2C 扩展器 PI4IOE5V6408**(I2C SDA=10 / SCL=8;M5Unified `M5.getIOExpander(0)`)。**官方例程口径为准**(pinmap 表与例程对 NRST/LNA 位号矛盾):`ioe.digitalWrite(7,false);delay(100);ioe.digitalWrite(7,true)`=SX_NRST 复位、`digitalWrite(6,true)`=SX_ANT_SW、`digitalWrite(5,true)`=SX_LNA_EN。
- **F‑18 官方 RadioLib 参数**:`radio.begin(868.0, 125.0, 12, 5, 0x34, 22, 20, 3.0, true)` = 868.0MHz / BW125k / **SF12** / CR4/5 / sync `0x34` / 22dBm / preamble20 / TCXO 3.0V / LDO。库:**RadioLib ≥7.3.0** + M5Unified;收发 `startTransmit()` / `startReceive()`+`readData()`;`getRSSI()` / `getSNR()`。
- **F‑19 ⚠️ 安全与坑**:(a)**发射前必接 LoRa 天线**(LoRa 那个 RP-SMA 口)——+22dBm 空载发射会损伤 SX1262 PA;(b)两端须**同频同参**才能通;(c)SF12 慢,每包空中时间约 1~2s;(d)已知初始化坑(RadioLib issue #1713 / M5 社区 init error)→ 照官方例程,并检查 `begin()` 返回 `RADIOLIB_ERR_NONE`。
- 参考:`docs.m5stack.com/en/arduino/unit_c6l/lora`、`/en/unit/Unit_C6L`、`/en/core/Stamp_C6LoRa`。
- **F‑20 天线要求(2026-06-21,官方/商店)**:随附**两根 SMA 鞭状天线,频段专用、不可互换**——**LoRa = 长 108mm / 868MHz / 3dBi**,WiFi = 短 84mm / 2.4GHz / 3dBi(均 50Ω)。**辨认:LoRa 是较长的 108mm 那根**。测试频率用 868.0MHz(F‑18)正好与该天线匹配。**务必把 108mm/868 那根拧到 LoRa 口**,别把 84mm WiFi 那根接 LoRa(频段不匹配→高 VSWR→F‑19a 损伤 PA 风险)。*(本次测试用户实际用的是 LilyGo SX1262 板配套的 ~5cm sub-GHz 胶棒天线代替,频段正确、近距够用。)*
- **F‑21 LoRa 实测进展(2026-06-21)**:两块烧入 ping-pong 固件(14dBm),启动均 `begin(): rc=0 (OK)` → **SX1262 引脚/SPI 配置(F‑16/F‑17)真机验证正确**。但首版 **100% 丢包**:PINGER `transmit()` 返回成功,RX 侧用 `radio.available()` 轮询**从未触发**(连 RX_ERR 都没有)。疑因:SX126x 在 RadioLib 需 **DIO1 中断收包**(非裸轮询)和/或缺 **`setDio2AsRfSwitch(true)`**(RF 开关)。
  - **✅ 已解决(2026-06-21)**:`begin()` 后加 **`radio.setDio2AsRfSwitch(true)`** + 收包改 **DIO1 中断**(`radio.setPacketReceivedAction(cb)` 设标志,弃用 `available()` 轮询)→ 复测 **28/28、双向 0% 丢包、SNR 5~9dB**,**LoRa P2P 通信确认可用**。**本板 SX1262 收发必备这两步**(写代码/给 codex 时务必带上)。
  - 注:近距 14dBm 下 **RSSI 饱和钉在 ~0dBm**(功能 OK;要测真实 RSSI/距离需再降功率或拉开两块距离)。
