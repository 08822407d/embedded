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

void setup() {
    M5.begin(true, true, false);  // 串口 + 内部 I2C(IMU 在 Wire1)，LED 不用
    imuInit();

    Serial.println();
    Serial.println("# motor direction test (with anti-flip failsafe)");

    if (!motorInit()) {
        Serial.println("ERR: 电机 I2C 初始化失败，停止。");
        motorPowerOff();
        return;
    }

    dirTestRun();  // 阻塞执行一次，内嵌看门狗与断电保底
}

void loop() {
    // 测试结束后空闲：电机已停/断电；周期打印 pitch 供观察
    double pitch = 0, roll = 0;
    imuReadAttitude(&pitch, &roll);
    Serial.printf("# idle pitch=%.2f  motorErr=%u\n", pitch, motorErrorCode());
    delay(500);
}
