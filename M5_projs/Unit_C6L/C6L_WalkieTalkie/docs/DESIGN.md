# DESIGN — 架构与当前设计(Codex 实现依据)

> 这里放**经讨论定稿**的架构 / 接口 / 模块划分,作为 Codex 写代码的依据。
> 需求未定,本文件现为占位骨架;每定稿一块就写一块,并在 `STATUS.md` 标"已派给 Codex:<什么>"。

## 1. 系统框图
*(待定:音频采集 → 编码 → 无线发送 → 无线接收 → 解码 → 播放 的完整链路)*

## 2. 硬件接口
- MCU:ESP32-C6(详见 [`FACTS.md`](FACTS.md) F‑1)。
- 引脚分配:**待定**(麦克风 I2S、喇叭/功放、PTT 按键、指示灯…)。
- 总线:**待定**。

## 3. 软件架构
- 框架:Arduino + M5Unified(FACTS F‑2/F‑3)。
- 任务划分(FreeRTOS):采集 / 编解码 / 收发 / UI ——**待定**。
- 缓冲与时序(音频帧大小、抖动缓冲)——**待定**。

## 4. 无线协议设计
- 选型与帧格式 ——**待定**(见 REQUIREMENTS 待确认 #1)。

## 5. 模块清单(供 Codex 拆任务)
- *(待需求定稿后填:模块名 / 职责 / 输入输出 / 依赖)*

## 6. LoRa 通信测试(首个 codex 任务)
**目标**:两块 Unit-C6L 验证 SX1262 LoRa **双向**通信通,产出可量化日志(RSSI/SNR/丢包/RTT)。硬件依据 FACTS F‑15~F‑19。
**前置(硬件)**:两端都接 **LoRa 天线**、同频同参(见 F‑19)。

**角色**:同一固件烧两块,**按 MAC 自动分角色**——MAC 较小者 = `PINGER`,较大者 = `PONGER`(本测试两块为 `58:8C:81:50:04:38` / `58:8C:81:50:06:E4`)。启动打印横幅:自身 MAC、角色、射频参数、`radio.begin()` 返回码。

**协议**(沿用官方例程射频参数 868.0/BW125/SF12/CR5/sync0x34/22dBm):
- `PINGER`:每 ~4s 发 `PING <seq>`(seq 递增)→ `startReceive` 等 `PONG <seq>`,超时(~3s)记 `TIMEOUT`;收到记 rssi/snr/rtt。每 10 包打印汇总 `sent/ponged/loss%`。
- `PONGER`:收到 `PING <seq>` 立即回 `PONG <seq>`,记本端 rssi/snr。

**日志**(115200 USB-CDC,单行可解析):`[ROLE] seq=<n> <EVENT> rssi=<..> snr=<..> rtt=<..>ms`。

**主机侧抓取**:pyserial 同时读两块(端口先按 MAC 探测,FACTS F‑13 法),各存日志文件,跑 ~90s,脚本汇总丢包率/RSSI 区间。

**通过判据**:PINGER 持续收到 PONG、loss 低、近距离 RSSI 约 −20~−60dBm、SNR 为正 → 双向链路通。

**实现要点(codex)**:RadioLib ≥7.3.0 + M5Unified;**严格照官方例程**做 IOExpander 初始化(F‑17)与 SPI 引脚 `SPI.begin(20,22,21,23)`;**必判** `radio.begin()`/收发返回码并打印错误码;不改 platformio `board`;`lib_deps` 增 RadioLib。**不要烧录**——CC 在确认天线后再烧。参考官方例程 `docs.m5stack.com/en/arduino/unit_c6l/lora`。

**⚠️ SX1262 收发必备两步(F‑21 实测,缺则双向 0 通)**:`begin()` 成功后立刻 ① `radio.setDio2AsRfSwitch(true)`(DIO2 控 TX/RX 天线开关);② 收包用 **DIO1 中断**(`radio.setPacketReceivedAction(cb)` 置标志,再 `readData`),**不要用 `radio.available()` 轮询**(SX126x 上不触发)。

**运行时长 + 关射频兜底(必须)**:**有界运行**——PINGER 发满 `N=30` 个 ping(或总超时 ~150s)即停;PONGER 持续 `20s` 无 PING 判结束。结束动作:打印最终汇总 → **`radio.sleep()`(SX1262 睡眠,停止收发)** → 打印 `=== TEST COMPLETE — RADIO OFF ===` → 空转(可点 RGB)。→ 射频发射严格限制在测试窗口内,不会持续发。

**启动时机 / 日志对齐(保证抓得到)**:一次性 banner + 早期包易丢(原生 USB-CDC 竞态,同 F‑13)。双保险:① 固件 `radio.begin()` 后**先等 USB-CDC 串口打开**(`while(!Serial && 超时<30s)`)再打 banner、再开测,串口没通就不发;② 主机**先挂载抓取再复位**——`tools/lora_capture.py` 先打开两口(置 DTR)开始读,再脉冲 RTS 复位两块(先 PONGER 后 PINGER),两端从 t=0 被录。叠加 → banner 与全程日志必被捕获。

**主机抓取脚本(codex 一并写)** `tools/lora_capture.py`:按 MAC 探测两口(F‑13 法:`303A:1001` + `SER=MAC`)、115200、置 DTR、可选脉冲 RTS 复位对齐;双线程带时间戳各存日志文件;读到两端均 `TEST COMPLETE` 或超时(~180s)停;末尾汇总每端 PING/PONG 计数、丢包率、RSSI/SNR 区间。纯主机脚本(系统 python3 + pyserial 3.5)。
