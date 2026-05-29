/*
 * 屏动平衡系统 —— 主程序（装配各功能模块）
 * 硬件: M5Stack ATOM Matrix (MPU6886 IMU + 5x5 RGB 点阵)
 *
 * 代码组织约定：按功能分模块，main 只做初始化与主循环装配，不堆业务代码。
 *   - imu.*   IMU 姿态读取
 *   - led.*   LED 点阵控制（临时，暂未接入）
 *   - (后续) 电机控制等各自独立成文件
 *
 * 当前阶段（第 1 步）：读 IMU 输出 pitch/roll，50ms/20Hz。
 */
#include <M5Atom.h>

#include "imu.h"
#include "led.h"  // 临时功能，暂未在 loop 中调用

// 采样间隔：沿用原工程的 50ms (20Hz)
static const uint32_t SAMPLE_INTERVAL_MS = 50;

void setup() {
    // 初始化 ATOM-Matrix：串口 + I2C(IMU)，暂不使用 LED 点阵
    M5.begin(true, true, false);
    imuInit();
    delay(100);
    Serial.println();
    Serial.println("# step1: IMU attitude (pitch/roll), 20Hz");
    Serial.println("# columns: pitch_deg, roll_deg");
}

void loop() {
    double pitch = 0, roll = 0;
    imuReadAttitude(&pitch, &roll);

    Serial.printf("pitch=%7.2f  roll=%7.2f\n", pitch, roll);

    delay(SAMPLE_INTERVAL_MS);
}
