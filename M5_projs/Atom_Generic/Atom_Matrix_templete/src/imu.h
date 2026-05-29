// imu —— MPU6886 姿态读取模块
// 读取加速度并换算成姿态角（pitch 俯仰 / roll 横滚），不暴露原始数据。
#pragma once

// 初始化 IMU（在 M5.begin 之后调用）
void imuInit();

// 读取当前姿态角，单位：度
//   pitch = atan2(-accX, hypot(accY,accZ))
//   roll  = atan2(accY, accZ)
void imuReadAttitude(double *pitch, double *roll);
