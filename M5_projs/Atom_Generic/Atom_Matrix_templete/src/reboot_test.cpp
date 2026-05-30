#include <M5Atom.h>
#include <esp_system.h>

#include "motor.h"
#include "reboot_test.h"

// RTC 慢速内存：热复位(EN/软件/看门狗)会保留，**掉电/上电**会丢失 → 用来判断是否真断过电。
RTC_DATA_ATTR uint32_t g_bootCount;
RTC_DATA_ATTR uint32_t g_rtcMagic;
static const uint32_t  RTC_MAGIC = 0xCAFEBABE;

static const char *reasonStr(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:   return "POWERON 上电/EN复位";
        case ESP_RST_EXT:       return "EXT 外部引脚复位";
        case ESP_RST_SW:        return "SW 软件esp_restart";
        case ESP_RST_PANIC:     return "PANIC 崩溃";
        case ESP_RST_INT_WDT:   return "INT_WDT 中断看门狗";
        case ESP_RST_TASK_WDT:  return "TASK_WDT 任务看门狗";
        case ESP_RST_WDT:       return "WDT 其它看门狗";
        case ESP_RST_BROWNOUT:  return "BROWNOUT 掉压复位";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP 深睡唤醒";
        default:                return "UNKNOWN";
    }
}

void rebootTestSetup() {
    bool rtcValid = (g_rtcMagic == RTC_MAGIC);
    if (!rtcValid) { g_rtcMagic = RTC_MAGIC; g_bootCount = 0; }
    g_bootCount++;
    esp_reset_reason_t rr = esp_reset_reason();

    Serial.println();
    Serial.println("===== 双路供电 重启/复位 诊断 =====");
    Serial.printf("# BOOT #%u\n", g_bootCount);
    Serial.printf("# 复位原因 = %d (%s)\n", (int)rr, reasonStr(rr));
    Serial.printf("# RTC 内存 = %s → 上次%s\n",
                  rtcValid ? "保留" : "丢失",
                  rtcValid ? "是热复位(板子全程有电、未断电)" : "经历了掉电/冷启动(板子曾断电)");

    // 读电机 Vin 确认 12V 是否在场（=> RollerCAN 经 Grove 有回灌供电路径）
    if (motorInit()) {
        Serial.printf("# 电机 Vin = %.2fV %s\n", motorVoltage(),
                      (motorVoltage() > 6.0f) ? "(12V在场：存在经 Grove 回灌的第二路供电)"
                                              : "(未见12V：仅 USB 供电)");
        motorPowerOff();
    } else {
        Serial.println("# 电机 I2C 未响应（可能 RollerCAN 未上电）");
    }
    Serial.println("# 之后每秒心跳。点阵常亮蓝=板子在线；");
    Serial.println("# 若拔掉 USB 后点阵仍亮/心跳继续 → 说明被 12V 经 Grove 回灌供着电(没断电)。");
    M5.dis.fillpix(CRGB(0x00, 0x00, 0x30));  // 蓝：在线
}

void rebootTestLoop() {
    static uint32_t boot_ms = millis();
    static uint32_t n = 0;
    Serial.printf("# 心跳 boot#%u uptime=%lus\n", g_bootCount, (millis() - boot_ms) / 1000);
    M5.dis.fillpix((n++ & 1) ? CRGB(0x00, 0x00, 0x30) : CRGB(0x00, 0x14, 0x00));  // 蓝/绿交替=活着
    delay(1000);
}
