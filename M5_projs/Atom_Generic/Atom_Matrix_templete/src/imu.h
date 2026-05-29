// imu —— MPU6886 读取模块（原始 6 轴 + 姿态角换算）
#pragma once

// 初始化 IMU（在 M5.begin 之后调用）。注意：MPU6886 在 Wire1。
void imuInit();

// 读取原始 6 轴：加速度 ax/ay/az（单位 g）、角速度 gx/gy/gz（单位 °/s）
void imuReadRaw(float *ax, float *ay, float *az, float *gx, float *gy, float *gz);

// 由加速度纯计算姿态角（不再访问 I²C），单位：度
//   pitch = atan2(-ax, hypot(ay,az))，roll = atan2(ay, az)
void imuComputeAttitude(float ax, float ay, float az, double *pitch, double *roll);

// 便利函数：读一次加速度并换算姿态角（内部会访问 I²C）
void imuReadAttitude(double *pitch, double *roll);
