# RNode C6L Port - 移植与验收记录

更新时间: 2026-06-25 00:22 +08:00(本机时间)。本轮任务从 2026-06-24 开始,跨过午夜收尾。

## 结论

Unit-C6L 的 RNode 固件移植已完成,并已用两块 C6L 在 Reticulum 下通过 USB-CDC 做双机互通验收:

- 两块板均能被 `rnodeconf` 识别为 RNode/SX1262 设备。
- 两块板均能被 Reticulum `RNodeInterface` 拉起为 `Up`。
- 自定义 Reticulum echo/proof probe 双向成功,证明包带回 SNR。
- `rncp` A->B 与 B->A 文件传输逐字节校验一致,且包含大于 255B 的载荷,覆盖分片/重组/可靠传输路径。
- Grove UART 固件环境已实现并编译通过,但本轮没有接 Grove 线,所以 UART 路径仅 compile-only。

## 固件改动

固件放在本项目子目录 `RNode_Firmware/`,来源为上游 `markqvist/RNode_Firmware`,预检时看到上游 `HEAD=d39339f8ecd5145b248c18bac7b6ea0f82faf85a`。本轮保留为本地子目录工作区。

主要适配点:

- `RNode_Firmware/platformio.ini`
  - 新增 PlatformIO 工程配置,`src_dir = .`。
  - 默认环境: `m5stack-unitc6l-rnode`。
  - UART 环境: `m5stack-unitc6l-rnode-uart`。
  - 平台: `pioarduino/platform-espressif32` develop zip。
  - 板型: `esp32-c6-devkitc-1`。
  - USB-CDC 默认打开: `ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`。
  - 沿用本项目 `scripts/fix_toolchain_path.py` 修复 riscv toolchain PATH。
- `RNode_Firmware/Boards.h`
  - 新增 `BOARD_M5_UNIT_C6L = 0x45` 与 `MODEL_5C = 0x5C`。
  - SX1262 引脚: CS=23, DIO1=7, BUSY=19, SCLK=20, MOSI=21, MISO=22。
  - I2C: SDA=10, SCL=8。
  - 启用 `DIO2_AS_RF_SWITCH`, `HAS_BUSY`, `HAS_TCXO`, `HAS_EEPROM`, `HAS_NP`。
  - C6L 专用开关: `C6L_USE_IO_EXPANDER`, `C6L_AUTOPROVISION`。
- `RNode_Firmware/sx126x.cpp`
  - 用 `Wire` 直接驱动 PI4IOE5V6408,地址 `0x43`。
  - 扩展器 pin 7 控 SX1262 NRST: low 100ms -> high。
  - 扩展器 pin 6 控 ANT_SW high。
  - 扩展器 pin 5 控 LNA_EN high。
  - C6L 进入 SX1262 自定义 SPI begin 路径。
  - C6L TCXO 电压设为 `MODE_TCXO_3_0V_6X`。
- `RNode_Firmware/RNode_Firmware.ino`
  - USB-CDC 为默认 host KISS 串口。
  - UART 环境编译时将 RNode 的 `Serial` 路由到 `HardwareSerial(1)`,RX=G4/GPIO4,TX=G5/GPIO5。
  - 启动时调用 C6L 自动 provision。
- `RNode_Firmware/Utilities.h`
  - C6L 首次启动/旧坏配置时自动写入 EEPROM 信息。
  - 为兼容 stock host tools,EEPROM 对外使用 `PRODUCT_RNODE + MODEL_A6`,板字节保留 `0x45`。因此 `rnodeconf` 会显示 `RNode 820 - 960 MHz (03:a6:45)`。
  - serial 从 ESP32-C6 Wi-Fi STA MAC 后四字节派生。
- `RNode_Firmware/Device.h`
  - 对无 BT/BLE host 路径的 C6L 设置 `bt_ready=true`,避免 USB-only 启动被 BT 状态阻塞。

## 构建

在 `RNode_Firmware/` 下复验:

```bash
pio run -e m5stack-unitc6l-rnode
pio run -e m5stack-unitc6l-rnode-uart
```

结果:

- `m5stack-unitc6l-rnode`: SUCCESS, RAM 11.9%, Flash 29.0%。
- `m5stack-unitc6l-rnode-uart`: SUCCESS, RAM 11.9%, Flash 30.1%。

构建仍有上游代码在当前 ESP32-C6/C++ 工具链下的 `volatile` 与 IRAM section 警告,但无失败;UART 环境此前的 `Serial redefined` 警告已消除。

## 烧录与识别

本轮 §0 预检和烧录前探测到两块 C6L,均为 VID:PID `303A:1001`:

- `/dev/ttyACM0`, SER/MAC `58:8C:81:50:04:38`, USB location `1-12:1.0`
- `/dev/ttyACM1`, SER/MAC `58:8C:81:50:06:E4`, USB location `1-3:1.0`

烧录的是 USB-CDC 默认环境:

```bash
pio run -e m5stack-unitc6l-rnode -t upload --upload-port /dev/ttyACM0
pio run -e m5stack-unitc6l-rnode -t upload --upload-port /dev/ttyACM1
```

esptool 均确认 `ESP32-C6`,16MB flash,upload SUCCESS。

`rnodeconf --info` 识别结果:

- `/dev/ttyACM0`: serial `81:50:04:38`
- `/dev/ttyACM1`: serial `81:50:06:e4`
- Product: `RNode 820 - 960 MHz (03:a6:45)`
- Firmware: 1.86
- HW rev: 1
- Modem: SX1262
- Frequency range: 820-1020 MHz
- Max TX power: 22 dBm
- Device mode: Normal

`Device signature unverified` 是预期现象:这是本地自定义固件,不是官方签名发布件。

## Reticulum 配置

本轮安装 Reticulum/RNS:

```bash
python3 -m pip install --user --break-system-packages -U rns
```

版本: RNS 1.3.5。命令位于 `/home/cheyh/.local/bin/`。

测试用临时配置目录:

- A: `/tmp/c6l_rnode_test/node_a`,port `/dev/ttyACM0`
- B: `/tmp/c6l_rnode_test/node_b`,port `/dev/ttyACM1`

`config` 模板:

```ini
[reticulum]
enable_transport = No
share_instance = Yes
instance_name = c6l_node_a

[interfaces]
  [[C6L RNode]]
    type = RNodeInterface
    enabled = yes
    port = /dev/ttyACM0
    frequency = 868000000
    bandwidth = 125000
    txpower = 7
    spreadingfactor = 8
    codingrate = 5
```

B 节点只需把 `instance_name` 与 `port` 分别改成 `c6l_node_b` 和 `/dev/ttyACM1`。

建议复测顺序:

```bash
/home/cheyh/.local/bin/rnsd --config /tmp/c6l_rnode_test/node_a
/home/cheyh/.local/bin/rnsd --config /tmp/c6l_rnode_test/node_b
/home/cheyh/.local/bin/rnstatus --config /tmp/c6l_rnode_test/node_a
/home/cheyh/.local/bin/rnstatus --config /tmp/c6l_rnode_test/node_b
```

## 传输验收

### rnstatus

两端 `RNodeInterface[C6L RNode]` 均为 `Up`,mode `Full`,rate `3.12 kbps`。实测 noise/interference:

- A: noise floor 约 `-73 dBm`,无 interference。
- B: noise floor 约 `-87 dBm`,interference 约 `-75 dBm`。

### 自定义 packet/proof probe

脚本: `tools/reticulum_packet_probe.py`。

用途:

- `listen`: 创建 `c6ltest.echo` 目的地并周期 announce。
- `send`: 向指定 destination 发一个 Reticulum Packet,等待 proof,打印 proof packet 的 RSSI/SNR。

命令:

```bash
python3 tools/reticulum_packet_probe.py listen --config /tmp/c6l_rnode_test/node_b --announce-interval 10
python3 tools/reticulum_packet_probe.py send --config /tmp/c6l_rnode_test/node_a --destination <B_DEST> --size 32 --timeout 90
```

反向同理,交换 A/B 配置与 destination。

实测:

- A -> B: 32 bytes delivered in 1.003s, proof packet `RSSI=0 dBm`, `SNR=12.75 dB`。
- B -> A: 32 bytes delivered in 1.106s, proof packet `RSSI=0 dBm`, `SNR=12.5 dB`。

近距 RSSI `0 dBm` 与早前 RadioLib ping-pong 测试一致,应视为近场饱和/读数不可靠;SNR 可用。

### rncp 任意字节文件传输

打印 rncp destination:

```bash
/home/cheyh/.local/bin/rncp --config /tmp/c6l_rnode_test/node_b -p
/home/cheyh/.local/bin/rncp --config /tmp/c6l_rnode_test/node_a -p
```

本轮 destination:

- B rncp: `921ed9c2494b7ee8a51d922ff357f2f6`
- A rncp: `d6a6d4dd06feffdda5ce5aa92c0c3426`

接收端:

```bash
/home/cheyh/.local/bin/rncp --config /tmp/c6l_rnode_test/node_b --listen --save /tmp/c6l_rnode_test/recv_b --overwrite -b 10 -n
```

发送端:

```bash
/home/cheyh/.local/bin/rncp --config /tmp/c6l_rnode_test/node_a /tmp/c6l_rnode_test/send/payload_768.bin 921ed9c2494b7ee8a51d922ff357f2f6 -P -w 180
```

实测结果:

- A -> B short text `hello.txt`: sha256 `cb8635189a583068994f635ba86aab126f4ae23b9b527920e648c52102d9f301`,`cmp` OK。
- A -> B 768B binary `payload_768.bin`: sha256 `d1031a4a74d6d775c24a8cc0a29760ed01e97254eaab4c845323dc2ed15eac1f`,`cmp` OK; rncp resource payload 794B,约 2.54 Kbps。
- B -> A 640B binary `reverse_640.bin`: sha256 `b7e8d966c29931a792f74a4b796303966250d52f341598ceb59e3100cb4ae397`,`cmp` OK; rncp resource payload 666B,约 1.48 Kbps。

> 768B/640B 均超过单个 LoRa 包,已覆盖 Reticulum 分片、重组与可靠传输。

## 已知现象

- RNodeInterface 初次打开偶尔先报一次 `Radio state mismatch`,随后自动 reopen 并恢复到 `configured and powered up`。本轮测试做法是启动 shared instance 后等接口稳定再发包。这个现象未阻塞验收,但后续可继续查启动状态机/EEPROM provision 时序。
- `rnodeconf` 看到的是兼容 stock tools 的 `RNode 820 - 960 MHz (03:a6:45)`,不是单独的官方 C6L 产品名。这是有意设计,避免 host tools 拒绝未知 product/model。
- 本地自定义固件没有官方签名,所以签名校验 warning 正常。
- Grove UART 环境已编译通过,但还没有接线实测。接线后烧录 `m5stack-unitc6l-rnode-uart`,主机侧串口改为 Grove UART 对应的 USB-UART 口再跑同一套 Reticulum 测试。
- 入库形态**已定 = patch 流**(不 vendored 整份源码,见下节)。

## 重建与再移植(上游变动时)

> 设计意图:`RNode_Firmware/`(整份上游 clone)**不入库**(见项目 `.gitignore`),避免把第三方代码塞进 monorepo。入库的只有:本文件、`patches/rnode_c6l.patch`、`tools/setup_rnode_firmware.sh`。改动只在你本地 clone + 你自己的仓库,**绝不影响上游 markqvist/RNode_Firmware**。

### A. 一键重建(上游没大改 / 复现本次)
补丁 pin 在上游 commit **`d39339f8ecd5145b248c18bac7b6ea0f82faf85a`**:
```bash
bash tools/setup_rnode_firmware.sh
cd RNode_Firmware && pio run -e m5stack-unitc6l-rnode
```
脚本做:`git clone` 上游 → `git checkout d39339f8` → `git apply patches/rnode_c6l.patch`。

### B. 上游大改后再移植(补丁打不上时)
想基于**新版上游**重做、`git apply` 失败时,**别去通读全仓**——照下面「集成点清单」逐条重做即可(按职责定位,不依赖行号);引脚/初始化的硬事实只看 [`FACTS.md`](FACTS.md) F‑15~F‑21,上面「固件改动」是上一版落点参考。

**移植 = 在新版上游里做这 6 件事:**
1. **构建配置**(本轮 `platformio.ini`):加 C6L env(board `esp32-c6-devkitc-1`、pioarduino develop、`src_dir=.`、`ARDUINO_USB_MODE=1`/`CDC_ON_BOOT=1`、`extra_scripts` 用本项目 `scripts/fix_toolchain_path.py`);再加 `-uart` env(继承 + UART 路由 flag)。
2. **板定义**(上游放"板/引脚宏"的头,本轮 `Boards.h`):新增 C6L 板(`BOARD_M5_UNIT_C6L=0x45`/`MODEL_5C=0x5C`);SX1262 引脚 CS23/DIO1=7/BUSY19/SCK20/MOSI21/MISO22;I2C SDA10/SCL8;开 `DIO2_AS_RF_SWITCH`/`HAS_BUSY`/`HAS_TCXO`/`HAS_EEPROM`/`HAS_NP`;C6L flag `C6L_USE_IO_EXPANDER`/`C6L_AUTOPROVISION`。引脚 = FACTS F‑16/F‑17。
3. **★最关键:把 RF 控制接到 I2C 扩展器**(上游 SX1262 驱动,本轮 `sx126x.cpp`):在芯片**复位/初始化**路径里,用 `Wire` 驱动 PI4IOE5V6408(地址 `0x43`):pin7=NRST(low 100ms→high)、pin6=ANT_SW(high)、pin5=LNA_EN(high);C6L 走自定义 SPI begin;TCXO=3.0V。**这是移植真难点**——本板 RST/ANT/LNA 不直连 GPIO 而走 I2C 扩展器;上游若重构 radio init,就在"复位 radio / 配 RF 开关"那步注入。依据 FACTS F‑17/F‑21。
4. **host 串口路由**(主程序,本轮 `RNode_Firmware.ino`):默认 USB-CDC 作 KISS host 口;`-uart` 编译时把 `Serial` 路由到 `HardwareSerial(1)`,RX=G4/GPIO4、TX=G5/GPIO5;启动调 C6L autoprovision。
5. **EEPROM 自动 provision**(上游 provision 逻辑,本轮 `Utilities.h`):首启/坏配置自动写 EEPROM;对外伪装 `PRODUCT_RNODE+MODEL_A6`、板字节留 `0x45`(否则 `rnodeconf` 拒绝未知 product/model);serial 取 C6 WiFi STA MAC 后四字节。
6. **跳过 BT 阻塞**(本轮 `Device.h`):USB-only 的 C6L 设 `bt_ready=true`,免启动卡在 BT 状态。

做完 `pio run -e m5stack-unitc6l-rnode` 应编译过;再按本文「传输验收」复测。重做完**重新生成补丁**:`cd RNode_Firmware && git add -A && git diff --cached <新上游base> > ../patches/rnode_c6l.patch`,并更新本节 pin 的 commit。

