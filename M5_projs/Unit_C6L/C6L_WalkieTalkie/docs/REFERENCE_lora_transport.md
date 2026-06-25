# REFERENCE — LoRa 透明可靠数据传输(备料)

> 用途:为"C6L 当纯 LoRa 数据 modem、外接主机做大脑"这一架构(REQUIREMENTS §2)准备的参考资料与方案对比。
> 资料级文档,事实标来源;选型未定,列清选项与取舍供拍板。整理日期 2026-06-21。

## 1. 架构(已定)
`主机(大脑) ──串口帧──> C6L(LoRa modem) ))) 空中 ((( C6L(LoRa modem) ──串口帧──> 主机(大脑)`
- C6L 职责仅:串口帧 ⇄ LoRa 分片收发 + 可靠性(ARQ)。不碰数据语义。
- 已具备基础:SX1262 经 RadioLib 收发实测可用(FACTS F-21:必须 `setDio2AsRfSwitch(true)` + DIO1 中断收包)。

## 2. 两条路线(待拍板,REQUIREMENTS 待确认 #2)

### 路线 A — 借用 Reticulum + RNode 固件(最贴合)
- **RNode** = 开源 LoRa 调制解调器固件:把 SX126x/SX127x + (ESP32/nRF52) 板变成一个 **USB-CDC / KISS 串口的 LoRa 无线接口**,由外接主机上的 **Reticulum(RNS)** 协议栈驱动。这与本项目架构几乎一一对应。
- Reticulum 提供:**数据无关传输 + 可靠投递 + 端到端加密 + 寻址/路由**;其上 **LXMF** 可做消息/文件传输。即"可靠、非实时、任意数据"现成具备。
- 支持芯片:SX1276/78/**1262**/1268/1280(SPI + 暴露 DIO 中断)——C6L 的 SX1262 符合。官方支持一批成品板,并支持"基于通用 ESP32 的自制 RNode"。
- **代价/风险**:RNode 官方未列 Unit-C6L → 需**移植**(C6L 的 RF 控制线走 PI4IOE5V6408 I2C 扩展器,非直连 GPIO,F-17;RNode 通常假定直连 GPIO,移植要改这部分)。主机侧需运行 Reticulum(Python,适合 PC/树莓派;单片机主机不适合)。
- 适合:主机是 PC/树莓派/手机,且愿意用 Reticulum 生态。

### 路线 B — 自研"串口帧 + 分片 + ARQ"透传 modem(最省事起步)
- 直接在**已验证的 RadioLib 收发**上加:主机串口帧(KISS)→ 分片(≤255B)→ 加序号/CRC → 停等或选择重传 ARQ → 对端重组 → 出串口。
- 优点:完全基于现有可用固件;简单、可控;主机侧任意语言/单片机都能对接(就是个带定界的串口协议)。
- 缺点:可靠性/分片/(可选)加密要自己实现;无现成寻址/路由(点对点够用)。
- 适合:主机是单片机/任意设备,或只想要一根"可靠的无线串口管道"。

### 路线 C — Meshtastic(次选)
- C6L 官方就是 Meshtastic 节点。但 Meshtastic 是**消息/网状**导向(文本、位置、小载荷,单条 ~200B 量级),不是干净的"任意字节透明管道",传图片/大文件不顺手。除非将来要组网,否则不如 A/B 贴合。

## 3. 主机 ↔ C6L 接口
- 物理:**USB-CDC**(已验证可用,115200)或 **Grove UART**(G4/G5,FACTS Unit_C6L pinmap)。RNode 惯用 USB-CDC。
- 帧定界:推荐 **KISS**(简单、字节填充、电台界通用)或自定义"长度前缀+CRC"。裸串口无定界不可靠,必须有帧。

## 4. 可靠传输要素(路线 B 自研时的清单;A 由 Reticulum 自带)
- **分片**:SX1262 LoRa 单包 payload ≤ **255 字节** → 大数据必须切片。
- **每片**:序号(seq)+ 总片数/偏移 + CRC。
- **ARQ**:停等(stop-and-wait,简单)或选择重传(selective-repeat,快但复杂)。**超时重传**;ACK 确认。
- **完整性**:整体校验(如整包 CRC32 / 哈希)确认重组无误。
- 业界已验证:有工作用 250B 分片 + 停等 ACK + 超时重传可靠传输图片(见来源)。

## 5. 吞吐量现实(务必先有预期;精确值用 Semtech LoRa Calculator 算)
LoRa 有效载荷速率随 SF/BW 变化(**近似量级**,BW125k):
| 配置 | 大致有效速率 | 1 KB 约需 | 10 KB 约需 | 100 KB 约需 |
|---|---|---|---|---|
| SF12/BW125(最远最慢,F-18 测试用) | ~0.25–0.3 kbps | ~30–40 s | ~5–7 min | ~1 小时 |
| SF9/BW125 | ~1.5–1.8 kbps | ~5–6 s | ~1 min | ~10 min |
| SF7/BW125 | ~5 kbps | ~1.6 s | ~16 s | ~3 min |
| SF7/BW500(最近最快) | ~20 kbps | ~0.4 s | ~4 s | ~40 s |
- 结论:**文字**秒级可达;**压缩语音消息**(如 Codec2 ~1–3 kbps)可行但偏慢、且非实时;**图片**即便压到几十 KB 也要**数分钟~数十分钟**——用户已接受非实时,关键是设好预期 + 选合适 SF/BW(距离换速度)。
- 注:扣除 ARQ 的 ACK 往返 + 重传 + 协议开销,实际再打折扣。占空比受限地区(如 EU868 1%)会进一步拖慢——见待确认 #5。

## 6. 待你拍板(汇总到 REQUIREMENTS 待确认)
1. 外接主机是什么设备、用哪个口(决定 A 是否可行、接口形态)。
2. 路线 A(Reticulum/RNode,需移植)还是 B(自研串口+ARQ,起步快)。
3. 目标距离 → SF/BW → 吞吐(§5)。
4. 典型载荷大小 + 可接受耗时。
5. 频段/占空比/合规。

## 来源
- Reticulum 硬件/RNode:reticulum.network/manual/hardware.html;github.com/markqvist/RNode_Firmware;unsigned.io/rnode_firmware
- 可靠 LoRa 传输(分片+ARQ,图片):Kim et al.(250B 分片+停等 ACK);LoRa-Alliance TS004 分片;RFC 9011 SCHC(均偏 LoRaWAN 网络向,P2P 仅作参考)
- 速率/airtime:Semtech SX1262 数据手册 + Semtech LoRa Calculator(精确值以此为准)
