# Codex 任务书 — 把 RNode 固件移植到 M5Stack Unit-C6L

> **给 Codex(VSCode 扩展)**:读完本文件即可**自主执行**。顺序:先做 §0 权限预检 → 再按 §4 阶段推进。
> 项目根:`/home/cheyh/projs/embedded/M5_projs/Unit_C6L/C6L_WalkieTalkie`(在 monorepo `embedded` 内)。
> 真相以仓库 `docs/` 为准:硬件事实 `docs/FACTS.md`(F-15~F-21);决策/参数 `docs/DECISIONS.md`(#002/#003);编译烧录 `docs/OPERATIONS.md`;需求 `docs/REQUIREMENTS.md`。动手前先读它们。

---

## 0. 权限预检(最先做,一次性)
用户已在 VSCode Codex 扩展授予最高权限,但个别动作可能仍弹权限申请。**开工前先把"你预计会触发申请的动作"逐一用最小探针跑一遍,让旁边值守的用户一次性批准**,之后再自主跑主线,避免中途卡住。至少覆盖:
- **网络**(后面要 `git clone` RNode、`pip install rns`、查文档):探针 `git ls-remote https://github.com/markqvist/RNode_Firmware` 或 `curl -sSI https://reticulum.network`。
- **串口/设备**(后面要烧录、读 `/dev/ttyACM*`):探针 `ls -l /dev/ttyACM*`。
- **工作区外写 / 装包**:探针写一个 `/tmp` 文件、`pip install --user --dry-run rns`。
先列出你判断会触发申请的动作清单,逐项探通,全过后再进 §1。

## 1. 目标(已定,别推翻)
- 一对 Unit-C6L 作 **Reticulum 的 LoRa modem(RNode)**;主机 = 本机 PC(Ubuntu),经 **USB-CDC** 跑 Reticulum。C6L 只做无线收发、数据透明(DECISIONS #002)。
- 任务 = 把 **RNode_Firmware** 移植到 Unit-C6L,并在 Reticulum 下实现双机互通。

## 2. 已验证硬件事实(直接用,无需再查;详见 docs/FACTS.md F-15~F-21)
- 板 = ESP32-C6 + **SX1262**(Stamp C6LoRa);Flash 16MB;PlatformIO board `esp32-c6-devkitc-1`,pioarduino 平台,**pio Core 必须 ≥6.1.19**(不够按 OPERATIONS.md 用 `python3 -m pip install --user -U platformio --break-system-packages` 升)。
- **SX1262 SPI**:MOSI=GPIO21 / MISO=22 / SCK=20 / **NSS(CS)=23** / **BUSY=19** / **DIO1(IRQ)=7**。
- **RF 控制线非直连 GPIO,经 I2C 扩展器 PI4IOE5V6408**(I2C SDA=10 / SCL=8;M5Unified 里 `M5.getIOExpander(0)`):
  - SX_NRST = 扩展器 pin **7**(复位序列:low → 100ms → high)
  - SX_ANT_SW = 扩展器 pin **6**(置 high 使能)
  - SX_LNA_EN = 扩展器 pin **5**(置 high 使能)
- **两条必备、缺则双向 0 通(已实测 F-21)**:① SX1262 用 **DIO2 控 TX/RX RF 开关**(RadioLib 里是 `setDio2AsRfSwitch(true)`,在 RNode 的驱动里找等价开关);② 收包靠 **DIO1 中断**,不是轮询。
- **主机↔C6L 接口:两个都要支持**——
  - **USB-CDC**(原生,`ARDUINO_USB_MODE=1`/`ARDUINO_USB_CDC_ON_BOOT=1`,115200):**当前测试就用它**(Type-C 直连 CDC 口;**Grove UART 那 4 根线现在没接**)。
  - **Grove UART(G4/G5)**:真正使用时可能改走 UART → 固件也要支持(运行期/编译期可选,或两者并存);**先实现,等接线后再测**。
  - RNode 如何把 host 串口接口路由到 USB-CDC vs 硬件 UART,你读它源码/配置确定。
- **执行环境**:开发 + 烧录 + 测试**全在本机(Ubuntu)**,codex 与板子同机,仓库已在本机——无需跨机/传文件。串口 `/dev/ttyACM*`,**名字不固定 → 先探测、别写死**(探测法见 `docs/OPERATIONS.md`『设备探测』)。
- **板子数量(重要)**:通常**两块 C6L 都接**在 Type-C 上(MAC=`58:8C:81:50:04:38`、`58:8C:81:50:06:E4`),**但也可能只接了一块** → **动手前先探测**:在线几块、哪几块是 C6(VID:PID `303A:1001`,按 hwid 的 `SER=`MAC 区分;若出现第三块是别的板,**别碰**)。双机互通测试需两块都在;只接一块时先做单板验证,双机测试待第二块接上。
- 现有 `src/main.cpp` = 我们之前的 RadioLib ping-pong 测试固件,**已实测这套引脚+初始化能驱动 SX1262 收发**——移植后会被替换,但可作"引脚/初始化正确"的参考样例。

## 3. 你(codex)自己查(别让 CC 代查,省其额度;读源码/官方文档自行定夺)
- **RNode_Firmware**(`github.com/markqvist/RNode_Firmware`,另见 `Reticulum-Things/RNode_Firmware`):
  - 板/目标怎么定义、PlatformIO env 怎么配、怎么加一个新板。
  - 它的 **SX1262 驱动**在哪、如何初始化(注意:RNode 有自己的 radio 抽象,**不一定用 RadioLib**)。
  - **关键适配点**:RNode 对 RST/ANT_SW/LNA 默认假定**直连 GPIO**;本板这些线走 **I2C 扩展器** → 找到这些控制点,改成经 PI4IOE5V6408(I2C)驱动。**先评估其工作量/可行性**;看有没有"用 I2C 扩展器控 RF"的相近板可当模板。
  - 怎么烧录/provision(`rnodeconf`?还是直接 pio?)、它的 KISS/串口接口形态。
- **Reticulum(rns)**:PC 端 `pip install rns`、怎么配 `RNodeInterface`(USB-CDC 口 / 频率 / SF/BW / airtime),查官方 manual(`reticulum.network`)。
- 射频参数:频段 **868.0MHz**;SF/BW 用较快设置(如 SF7~SF8 / BW125)即可。

## 4. 执行阶段
0. §0 权限预检。
1. 研究 §3,产出"加 C6L 板 + 把 RF 控制改到 I2C 扩展器"的具体方案。**若发现 RNode 架构让 I2C-扩展器 RF 控制非常难搞,先停下报告 + 给替代方案,不要硬 hack。**
2. 实现:在 RNode_Firmware 加 C6L 板目标(引脚见 §2 + I2C 扩展器 RF 控制 + DIO2-RF-switch + DIO1 中断);在本项目下以合适结构组织(可把 RNode clone 到子目录,或按其工程结构)。
3. 编译通过(`pio run`)。
4. 烧录两块 C6L(用户在旁值守、会批准设备权限;端口先探测、别写死;别碰非 C6 板)。
5. PC 装 Reticulum + 配 RNodeInterface(两块对应两个 USB-CDC 口)。
6. 双机互通测试。

## 5. 测试与验收
> **CC 未研读 RNode/Reticulum 内部** → "用什么命令/工具测"由你读它们文档后**自行设计**(可能用 `rnstatus`、`rnprobe`/`rnx`、`rncp`,或写个小 Reticulum Python 脚本)。下面是**必须验证的目标(测什么)**;具体 procedure 你定,并写进 `docs/`。本轮**全部经 USB-CDC**(本机 Ubuntu,`/dev/ttyACM*`,先探测);UART 路径等接线后再测。**目标 3–4(双机)需两块板都在线;若当前只接了一块,先做目标 1–2(单板),双机测试待第二块接上。**

**测试目标(逐项过)**
1. **固件自检**:RNode 固件启动、SX1262 初始化无错(务必落实 F-21 两条:DIO2 控 RF 开关 + DIO1 中断收包);板经 USB-CDC 枚举,`rnodeconf`/`rnstatus` 能识别为 RNode。
2. **单板就绪**:两块都能被 Reticulum 作为 RNodeInterface 拉起。
3. **链路连通**:两节点经 LoRa 建链;Reticulum echo/probe 往返成功;打印 **RSSI/SNR**(室内相邻应很强)。
4. **可靠透明传输(核心验收)**:A→B 与 B→A 各传一段**任意字节**并**逐字节校验一致**(checksum);**其中必须含一份 >255B(超过单个 LoRa 包)的数据**,以验证分片+重组+可靠性——这是 DECISIONS #002"数据透明、可靠"需求的关键。文字 + 一个小二进制/文件即可。
5. **诊断**:测试尽量做成可重复脚本(host 侧);失败时打印足够信息(端口、RNode 状态、Reticulum 日志、RSSI/SNR/丢包)。

**产出**:把测试 procedure + 实测结果 + RNode 板定义改动 + Reticulum 配置写进 `docs/`(新增文件或 DESIGN 小节),并更新 `docs/STATUS.md`。

## 6. 纪律 / 边界(重要)
- **娱乐用途、室内、两块板相邻摆放** → **通信距离、频段合规、占空比 / airtime 限制都不重要**,别花精力优化距离或合规(airtime 限制可放宽/关闭),目标就是"能可靠通"。
- 单一真相在仓库 `docs/`;先读 FACTS/DECISIONS/OPERATIONS 再动手。
- 别破坏 monorepo 里其它项目;改动限于本项目目录(+ 其下的 RNode 子目录)。
- 数据无关 / 可靠 / 非实时是核心(DECISIONS #002)。
