/*
 * 屏动平衡系统 —— 主程序（命令常驻架构）
 * 硬件: M5Stack ATOM Matrix (MPU6886 IMU + 5x5 点阵) + RollerCAN 反作用轮电机（电流/力矩模式）
 *
 * 架构（2026-05-31）：**固件常驻、串口命令分发**——烧一次，之后发字符命令触发流程，不必重编烧。
 *   loop 持续监视(83Hz 紧凑日志) + 收串口命令；命令跑完回监视。电机待命时输出断开。
 *   发命令：用 tools/serial_bridge.py（单进程独占串口、并发收发）→ `echo "r" > tools/serial.cmd`。
 *
 * 命令：r=急救(大力矩把卡第三面的机体磕回A/B)  i=系统辨识  b=探扭矩  c=起跳+缓冲  g=起跳→平衡  s=姿态  h=菜单
 */
#include <M5Atom.h>

#include "imu.h"
#include "motor.h"
#include "dir_test.h"

static void printMenu() {
    Serial.println("# === 命令待命(发 字符+换行) ===");
    Serial.println("#   r=急救(大力矩磕回A/B)  i=系统辨识  b=探扭矩  c=起跳+缓冲  g=起跳→平衡  s=姿态  h=菜单");
    Serial.println("#   d=突然加速扫描(电流逐档↑)  k=蓄能急停扫描(蓄能转速逐档↑)  ← 采起跳扭矩特征曲线");
    Serial.println("#   loop 持续监视(紧凑日志)；命令跑完回监视。");
}

static void dispatchCommand(const String &cmd) {
    char c = cmd.length() ? cmd[0] : 'h';
    Serial.printf("# >>> CMD '%c'\n", c);
    // 驱动类命令：先重新使能电流输出（上个流程/待命时输出是断的；不重 begin，避免双 I²C 挂死）
    if (c == 'r' || c == 'i' || c == 'b' || c == 'c' || c == 'g' || c == 'd' || c == 'k') motorReenable();
    switch (c) {
        case 'r': recoverFlipTest();       break;   // 急救：面内翻越(第三面)且横向小 → 蓄能+全力急刹磕回 A/B
        case 'i': sysIdExperiment();       break;   // 系统辨识激励
        case 'b': probeBreakawayTorque();  break;   // 探能起跳的扭矩 τ_break
        case 'c': swingUpCushionTest();    break;   // 起跳 + 回落缓冲
        case 'g': swingUpToBalance();      break;   // 起跳 → 平衡
        case 'd': directSweepTest();       break;   // 突然加速 扭矩特征扫描(电流逐档↑)
        case 'k': brakeSweepTest();        break;   // 蓄能急停 扭矩特征扫描(蓄能转速逐档↑)
        case 's': { double p = 0, r = 0; imuReadAttitude(&p, &r); Serial.printf("# 姿态 pitch=%.1f roll=%.1f\n", p, r); break; }
        case 'h':
        default:  printMenu();             break;
    }
    motorPowerOff();   // 命令收尾：输出断开，回待命/监视
    Serial.println("# >>> done，回监视。");
}

void setup() {
    M5.begin(true, true, true);   // 串口 + 内部 I2C(IMU 在 Wire1) + 5×5 点阵
    M5.dis.setBrightness(20);
    imuInit();
    Serial.println();
    Serial.println("# [命令常驻] 固件烧一次，串口发命令触发流程，不必重烧。");
    if (!motorInit()) { Serial.println("ERR: 电机 I2C 初始化失败。"); }
    motorPowerOff();   // 待命：输出断开
    printMenu();
    M5.dis.fillpix(CRGB(0x00, 0x00, 0x20));  // 蓝：待命/监视
}

void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length()) dispatchCommand(line);
        return;
    }
    attitudeReportTick();   // 无命令 → 跑一个监视窗（紧凑日志）
}
