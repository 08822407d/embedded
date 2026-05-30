/*
 * 屏动平衡系统 —— 主程序（装配各功能模块）
 * 硬件: M5Stack ATOM Matrix (MPU6886 IMU + 5x5 点阵) + RollerCAN 反作用轮电机
 *
 * 代码组织：按功能分模块，main 只做初始化与主循环装配。
 *   - imu.*       IMU 姿态读取（IMU 在 Wire1）
 *   - motor.*     RollerCAN 电机驱动（电流模式，Wire/Grove）
 *   - dir_test.*  电机旋转方向辨识 + 防翻保底
 *   - led.*       LED 点阵（临时，未接入）
 *
 * 当前阶段：电机旋转方向辨识（一次性，含安全保底）。
 */
#include <M5Atom.h>

#include "imu.h"
#include "motor.h"
#include "dir_test.h"
#include "reboot_test.h"

// === 启动表征序列：I²C自检 → 供电探测 → 识别A/B → 击杀式方向探测 → 当前侧危险边界 ===
void setup() {
    M5.begin(true, true, true);   // 串口 + 内部 I2C(IMU 在 Wire1) + 5×5 点阵
    M5.dis.setBrightness(20);
    imuInit();
    Serial.println();
    Serial.println("# 启动表征序列：I²C自检 → 供电探测 → 识别A/B → 击杀式方向探测 → 当前侧危险边界");
    if (!motorInit()) { Serial.println("ERR: 电机 I2C 初始化失败，停止。"); motorPowerOff(); return; }
    startupSequence();
}

void loop() {
    double pitch = 0, roll = 0;
    imuReadAttitude(&pitch, &roll);
    Serial.printf("# idle pitch=%.2f  motorErr=%u\n", pitch, motorErrorCode());
    delay(500);
}

// 双路供电重启诊断暂停用：需要时把 setup 换成 rebootTestSetup()/loop 换成 rebootTestLoop()。
