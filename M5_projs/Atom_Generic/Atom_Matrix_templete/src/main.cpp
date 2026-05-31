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

// === 简化流程（decisions/006）：开机只表征"起跳所需飞轮速度变化方向"→记录→终止(断电+持续监视) ===
//   危险(超平衡40°/横向/超速)即断电、仅监视、不自救。运行中绝不切控制模式。
void setup() {
    M5.begin(true, true, true);   // 串口 + 内部 I2C(IMU 在 Wire1) + 5×5 点阵
    M5.dis.setBrightness(20);
    imuInit();
    Serial.println();
    Serial.println("# [decisions/006] 开机方向表征：识别A/B→小速度变化冲量→记录起跳飞轮速度变化方向→终止(仅监视)");
    if (!motorInitSpeed(MOTOR_MAX_MA)) { Serial.println("ERR: 电机[速度模式] I2C 初始化失败，停止。"); motorPowerOff(); return; }
    probeSwingUpDirection();   // 判定+记录方向后终止
}

void loop() {
    attitudeReportTick();   // 终点态：每窗聚合判定机体静止态 + 全速率打印（电机已断电）
}

// 双路供电重启诊断暂停用：需要时把 setup 换成 rebootTestSetup()/loop 换成 rebootTestLoop()。
