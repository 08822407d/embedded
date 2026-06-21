/**
 * C6L_WalkieTalkie — LoRa 点对点通信测试固件
 *
 * 同一固件烧入两块 Unit-C6L,按 WiFi MAC 自动分角色:
 *   MAC 较小 → PINGER  (每 ~4s 发 PING <seq>,等 PONG,统计 RTT/RSSI/SNR/丢包)
 *   MAC 较大 → PONGER  (收到 PING <seq> 立即回 PONG <seq>,记本端 RSSI/SNR)
 *
 * 硬件依据: FACTS F-15~F-19/F-20
 * 射频参数: 868.0 MHz / BW125 / SF12 / CR4/5 / sync 0x34 / 14dBm / preamble20 / TCXO 3.0V
 * 串口: 115200 (USB-CDC, FACTS F-4)
 *
 * ⛔ 发射前必须接 LoRa 天线 108mm/868MHz 那根 (FACTS F-19a, F-20)
 *
 * ── 本版新增 ──────────────────────────────────────────────
 * [启动对齐] radio.begin() 后先等 USB-CDC 串口打开(最多 30s),
 *            串口通才打 banner、才开始收发,避免早期日志丢失。
 * [关射频兜底] 有界运行:
 *   PINGER: 发满 N=30 个 ping 或总超时 ~150s 即停。
 *   PONGER: 持续 20s 无任何 PING 到达即停。
 *   结束动作:打印汇总 → radio.sleep() → 打印 TEST COMPLETE → 空转。
 * ─────────────────────────────────────────────────────────
 */

#include <Arduino.h>
#include <M5Unified.h>
#include <RadioLib.h>
#include <WiFi.h>  // 只为读 MAC

// ---------------------------------------------------------------------------
// 已知两块板 MAC (FACTS F-13), 用于角色分配
// 58:8C:81:50:04:38  (ttyACM2) → PINGER
// 58:8C:81:50:06:E4  (ttyACM1) → PONGER
// ---------------------------------------------------------------------------

// SX1262 引脚 (FACTS F-16)
static const int PIN_LORA_CS   = 23;
static const int PIN_LORA_IRQ  = 7;
static const int PIN_LORA_BUSY = 19;

// SPI 引脚 (FACTS F-16/F-17)
static const int PIN_SPI_SCK  = 20;
static const int PIN_SPI_MISO = 22;
static const int PIN_SPI_MOSI = 21;

// IOExpander 引脚位 (FACTS F-17, 按官方例程口径)
static const int IOE_PIN_NRST    = 7;  // SX_NRST
static const int IOE_PIN_ANT_SW  = 6;  // SX_ANT_SW
static const int IOE_PIN_LNA_EN  = 5;  // SX_LNA_EN

// RadioLib SX1262 实例
SX1262 radio = new Module(PIN_LORA_CS, PIN_LORA_IRQ, RADIOLIB_NC, PIN_LORA_BUSY);

// DIO1 收包中断标志 (SX126x 收包须靠 DIO1 中断, 不能裸轮询 available() —— F-21)
static volatile bool g_rx_done = false;
static void IRAM_ATTR onRxDone() { g_rx_done = true; }

// ---------------------------------------------------------------------------
// 角色枚举
// ---------------------------------------------------------------------------
enum Role { UNKNOWN, PINGER, PONGER };
static Role g_role = UNKNOWN;
static char g_mac_str[18];  // "XX:XX:XX:XX:XX:XX\0"

// ---------------------------------------------------------------------------
// 有界运行参数
// ---------------------------------------------------------------------------
static const uint32_t PINGER_MAX_PINGS = 30;        // PINGER 最多发 30 包
static const uint32_t PINGER_TOTAL_TIMEOUT = 150000; // PINGER 总超时 150s (ms)
static const uint32_t PONGER_IDLE_TIMEOUT  = 20000;  // PONGER 20s 无包结束 (ms)

// ---------------------------------------------------------------------------
// MAC 工具
// ---------------------------------------------------------------------------

/** 读取 WiFi Station MAC, 写入 buf (至少 18 字节), 同时返回 uint64 便于比较 */
static uint64_t readMac(char *buf)
{
    WiFi.mode(WIFI_STA);
    String mac_s = WiFi.macAddress();
    mac_s.toUpperCase();
    strncpy(buf, mac_s.c_str(), 17);
    buf[17] = '\0';

    uint8_t mac[6];
    sscanf(buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

    uint64_t v = 0;
    for (int i = 0; i < 6; i++) v = (v << 8) | mac[i];
    return v;
}

// ---------------------------------------------------------------------------
// SX1262 初始化 (严格按 M5 官方例程 FACTS F-17/F-18)
// ---------------------------------------------------------------------------

static int16_t initRadio()
{
    // 1. IOExpander 复位序列
    auto &ioe = M5.getIOExpander(0);

    // SX_NRST: low → delay 100ms → high
    ioe.digitalWrite(IOE_PIN_NRST, false);
    delay(100);
    ioe.digitalWrite(IOE_PIN_NRST, true);

    // ANT_SW & LNA_EN enable
    ioe.digitalWrite(IOE_PIN_ANT_SW, true);
    ioe.digitalWrite(IOE_PIN_LNA_EN, true);

    delay(10);  // 稳定时间

    // 2. SPI 初始化 (SCK, MISO, MOSI, SS)
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_LORA_CS);

    // 3. RadioLib begin (FACTS F-18)
    //    begin(freq, bw, sf, cr, syncWord, power, preambleLen, tcxoVoltage, useRegulatorLDO)
    // power=14dBm:桌面近距离测试,降功率避免接收端饱和、并给小天线留安全余量(替代官方 14dBm)
    int16_t rc = radio.begin(868.0f, 125.0f, 12, 5, 0x34, 14, 20, 3.0f, true);
    if (rc == RADIOLIB_ERR_NONE) {
        // SX1262 用 DIO2 控制 TX/RX 天线 RF 开关 (F-21: 首版漏此步 → 双向 0 通)
        radio.setDio2AsRfSwitch(true);
        // 注册 DIO1 收包中断回调, 替代不可靠的 available() 轮询
        radio.setPacketReceivedAction(onRxDone);
    }
    return rc;
}

// ---------------------------------------------------------------------------
// 结束动作: 汇总 → radio.sleep() → 打印 COMPLETE → 空转
// ---------------------------------------------------------------------------

static void finalize(uint32_t sent, uint32_t ponged, float rssi_min, float rssi_max,
                     float snr_min, float snr_max)
{
    Serial.println("========================================");
    Serial.println("  FINAL SUMMARY");
    Serial.printf("  sent   : %lu\n", (unsigned long)sent);
    Serial.printf("  ponged : %lu\n", (unsigned long)ponged);
    if (sent > 0) {
        float loss = 100.0f * (sent - ponged) / (float)sent;
        Serial.printf("  loss   : %.1f%%\n", loss);
    }
    if (ponged > 0) {
        Serial.printf("  rssi   : [%.1f, %.1f] dBm\n", rssi_min, rssi_max);
        Serial.printf("  snr    : [%.1f, %.1f] dB\n", snr_min, snr_max);
    }
    Serial.println("========================================");

    radio.sleep();  // SX1262 进入睡眠,停止收发

    Serial.println("=== TEST COMPLETE \xe2\x80\x94 RADIO OFF ===");
}

// ---------------------------------------------------------------------------
// PINGER 主流程 (有界运行)
// ---------------------------------------------------------------------------

static void runPingerLoop(uint32_t loop_start)
{
    uint32_t sent   = 0;
    uint32_t ponged = 0;
    uint32_t seq    = 0;

    // RSSI/SNR 统计 (仅对收到 PONG 的包)
    float rssi_min =  1e9f, rssi_max = -1e9f;
    float snr_min  =  1e9f, snr_max  = -1e9f;

    uint32_t batch_ponged = 0;  // 上次打汇总时的 ponged 基准

    char txbuf[32];
    char rxbuf[64];

    while (true) {
        // ── 有界检查 ──────────────────────────────────────────
        if (sent >= PINGER_MAX_PINGS) {
            Serial.printf("[PINGER] Reached max pings (%lu), stopping.\n",
                          (unsigned long)PINGER_MAX_PINGS);
            break;
        }
        if (millis() - loop_start >= PINGER_TOTAL_TIMEOUT) {
            Serial.printf("[PINGER] Total timeout (%lus), stopping.\n",
                          (unsigned long)(PINGER_TOTAL_TIMEOUT / 1000));
            break;
        }

        // ── 发 PING ───────────────────────────────────────────
        seq++;
        snprintf(txbuf, sizeof(txbuf), "PING %lu", (unsigned long)seq);

        int16_t rc_tx = radio.transmit((uint8_t *)txbuf, strlen(txbuf));
        sent++;
        uint32_t t_sent = millis();

        if (rc_tx != RADIOLIB_ERR_NONE) {
            Serial.printf("[PINGER] seq=%lu TX_ERR rc=%d\n", (unsigned long)seq, rc_tx);
        } else {
            Serial.printf("[PINGER] seq=%lu SENT\n", (unsigned long)seq);
        }

        // ── 等 PONG (超时 ~3s) ────────────────────────────────
        bool got_pong = false;
        g_rx_done = false;
        radio.startReceive();

        while (millis() - t_sent < 3000) {
            if (g_rx_done) {
                g_rx_done = false;
                int16_t rc_rx = radio.readData((uint8_t *)rxbuf, sizeof(rxbuf) - 1);
                if (rc_rx == RADIOLIB_ERR_NONE) {
                    rxbuf[radio.getPacketLength()] = '\0';
                    uint32_t rtt  = millis() - t_sent;
                    float    rssi = radio.getRSSI();
                    float    snr  = radio.getSNR();

                    char expected[32];
                    snprintf(expected, sizeof(expected), "PONG %lu", (unsigned long)seq);
                    if (strncmp(rxbuf, expected, strlen(expected)) == 0) {
                        got_pong = true;
                        ponged++;
                        // 更新 RSSI/SNR 区间
                        if (rssi < rssi_min) rssi_min = rssi;
                        if (rssi > rssi_max) rssi_max = rssi;
                        if (snr  < snr_min)  snr_min  = snr;
                        if (snr  > snr_max)  snr_max  = snr;
                        Serial.printf("[PINGER] seq=%lu PONG rssi=%.1f snr=%.1f rtt=%lums\n",
                                      (unsigned long)seq, rssi, snr, (unsigned long)rtt);
                    } else {
                        Serial.printf("[PINGER] seq=%lu UNEXPECTED_PKT pkt=\"%s\"\n",
                                      (unsigned long)seq, rxbuf);
                    }
                } else {
                    Serial.printf("[PINGER] seq=%lu RX_ERR rc=%d\n", (unsigned long)seq, rc_rx);
                }
                break;
            }
            delay(5);
        }

        if (!got_pong) {
            Serial.printf("[PINGER] seq=%lu TIMEOUT\n", (unsigned long)seq);
        }

        // ── 每 10 包打印滑动汇总 ──────────────────────────────
        if (sent % 10 == 0) {
            uint32_t window_ponged = ponged - batch_ponged;
            float loss = 100.0f * (10 - window_ponged) / 10.0f;
            Serial.printf("[PINGER] SUMMARY sent=%lu ponged=%lu loss=%.1f%%\n",
                          (unsigned long)sent, (unsigned long)ponged, loss);
            batch_ponged = ponged;
        }

        // ── 等到下次发送时刻 (~4s 含收发时间) ────────────────
        uint32_t elapsed = millis() - t_sent;
        if (elapsed < 4000) {
            delay(4000 - elapsed);
        }
    }

    finalize(sent, ponged, rssi_min, rssi_max, snr_min, snr_max);
}

// ---------------------------------------------------------------------------
// PONGER 主流程 (有界运行)
// ---------------------------------------------------------------------------

static void runPongerLoop()
{
    uint32_t received = 0;  // 收到合法 PING 次数
    uint32_t sent_pong = 0;

    // RSSI/SNR 统计 (本端收到的 PING)
    float rssi_min =  1e9f, rssi_max = -1e9f;
    float snr_min  =  1e9f, snr_max  = -1e9f;

    char rxbuf[64];
    char txbuf[32];

    uint32_t last_ping_time = millis();  // 上次收到 PING 的时刻,用于 idle 超时

    // 进入接收模式
    g_rx_done = false;
    radio.startReceive();

    while (true) {
        // ── 空闲超时检查 ──────────────────────────────────────
        if (millis() - last_ping_time >= PONGER_IDLE_TIMEOUT) {
            Serial.printf("[PONGER] No PING for %lus, stopping.\n",
                          (unsigned long)(PONGER_IDLE_TIMEOUT / 1000));
            break;
        }

        if (!g_rx_done) {
            delay(5);
            continue;
        }
        g_rx_done = false;

        // ── 收包 ──────────────────────────────────────────────
        int16_t rc_rx = radio.readData((uint8_t *)rxbuf, sizeof(rxbuf) - 1);
        if (rc_rx != RADIOLIB_ERR_NONE) {
            Serial.printf("[PONGER] RX_ERR rc=%d\n", rc_rx);
            radio.startReceive();
            continue;
        }

        rxbuf[radio.getPacketLength()] = '\0';
        float rssi = radio.getRSSI();
        float snr  = radio.getSNR();

        // 解析 "PING <seq>"
        uint32_t seq = 0;
        if (sscanf(rxbuf, "PING %lu", (unsigned long *)&seq) == 1) {
            last_ping_time = millis();  // 重置 idle 计时器
            received++;

            // 更新 RSSI/SNR 区间
            if (rssi < rssi_min) rssi_min = rssi;
            if (rssi > rssi_max) rssi_max = rssi;
            if (snr  < snr_min)  snr_min  = snr;
            if (snr  > snr_max)  snr_max  = snr;

            Serial.printf("[PONGER] seq=%lu PING rssi=%.1f snr=%.1f\n",
                          (unsigned long)seq, rssi, snr);

            // 立即回 PONG <seq>
            snprintf(txbuf, sizeof(txbuf), "PONG %lu", (unsigned long)seq);
            int16_t rc_tx = radio.transmit((uint8_t *)txbuf, strlen(txbuf));
            if (rc_tx != RADIOLIB_ERR_NONE) {
                Serial.printf("[PONGER] seq=%lu TX_ERR rc=%d\n", (unsigned long)seq, rc_tx);
            } else {
                sent_pong++;
                Serial.printf("[PONGER] seq=%lu PONG_SENT\n", (unsigned long)seq);
            }

            // 回完后继续接收
            radio.startReceive();
        } else {
            Serial.printf("[PONGER] UNEXPECTED_PKT pkt=\"%s\" rssi=%.1f snr=%.1f\n",
                          rxbuf, rssi, snr);
            radio.startReceive();
        }
    }

    // PONGER 侧:sent 字段用 received 表示收到的 PING 数,ponged 用 sent_pong
    finalize(received, sent_pong, rssi_min, rssi_max, snr_min, snr_max);
}

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(300);

    auto cfg = M5.config();
    M5.begin(cfg);

    // --- 读 MAC, 决定角色 ---
    uint64_t my_mac = readMac(g_mac_str);

    // 两块 C6 的 MAC (FACTS F-13): 04:38 < 06:E4 → 04:38 板为 PINGER
    const uint64_t MAC_A = 0x588C81500438ULL;
    const uint64_t MAC_B = 0x588C815006E4ULL;
    (void)MAC_B;  // 避免 unused warning;PONGER 是 else 分支

    if (my_mac <= MAC_A) {
        g_role = PINGER;
    } else {
        g_role = PONGER;
    }

    // --- 初始化 SX1262 ---
    int16_t rc_begin = initRadio();

    // ── [启动对齐] 等 USB-CDC 串口打开,最多 30s ──────────────
    // radio.begin() 已完成,射频处于待机;先不发任何包。
    // 主机 lora_capture.py 会先打开串口(置 DTR),再脉冲 RTS 复位,
    // 复位后固件从这里等待串口枚举完成,保证 banner 和所有日志都被捕获。
    uint32_t t_wait = millis();
    while (!Serial && (millis() - t_wait < 30000)) {
        delay(50);
    }

    // --- 启动横幅 (串口通后才打) ---
    const char *role_str = (g_role == PINGER) ? "PINGER" : "PONGER";
    Serial.println("========================================");
    Serial.println("  C6L_WalkieTalkie LoRa P2P Test");
    Serial.println("========================================");
    Serial.printf("  MAC    : %s\n", g_mac_str);
    Serial.printf("  Role   : %s\n", role_str);
    Serial.println("  RF     : 868.0MHz / BW125 / SF12 / CR4/5 / sync=0x34 / 14dBm");
    Serial.printf("  begin(): rc=%d (%s)\n",
                  rc_begin,
                  (rc_begin == RADIOLIB_ERR_NONE) ? "OK" : "ERROR");
    if (g_role == PINGER) {
        Serial.printf("  Limit  : max %lu pings or %lus total\n",
                      (unsigned long)PINGER_MAX_PINGS,
                      (unsigned long)(PINGER_TOTAL_TIMEOUT / 1000));
    } else {
        Serial.printf("  Limit  : stop after %lus idle\n",
                      (unsigned long)(PONGER_IDLE_TIMEOUT / 1000));
    }
    Serial.println("========================================");

    if (rc_begin != RADIOLIB_ERR_NONE) {
        Serial.printf("[FATAL] radio.begin() failed rc=%d, halting.\n", rc_begin);
        while (true) delay(1000);
    }

    Serial.printf("[INFO] Role=%s, starting test.\n", role_str);

    // ── 进入有界测试主流程 (不从 loop() 反复调用,直接在此运行完) ──
    uint32_t t_loop_start = millis();
    if (g_role == PINGER) {
        runPingerLoop(t_loop_start);
    } else {
        runPongerLoop();
    }

    // setup() 返回后 loop() 会空转,射频已 sleep
}

void loop()
{
    // 测试已在 setup() 里完整跑完并 radio.sleep()
    // 这里只空转,不再做任何收发
    delay(1000);
}
