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

// ===== 陀螺零偏标定（加固版）=====
// 在**机体静止**时调用：丢弃开机首帧 → 连采 samples 帧取均值作为零偏 → 运动门校验。
//   运动门：采样窗内任一轴角速度峰峰超阈即判为"被晃动"，作废并重标（最多 5 次）。
//   ★对摆放倾角无要求★：陀螺测角速度、静止时真值=0 与朝向无关，只要采样那 ~1s 别动即可。
//   实测零偏 gy(pitch轴) 上电间会漂(+0.10→+0.40°/s)，故每次运行前现标、勿硬编码。
//   返回 true=拿到静止窗并更新零偏；false=多次仍有运动（零偏未更新，调用方应中止）。
bool imuCalibrateGyro(uint16_t samples);
// 取标定得到的三轴零偏（°/s）。未标定则为 0。
void imuGyroBias(float *bx, float *by, float *bz);

// ===== 单轴 pitch 互补滤波（陀螺为主 + 加速度慢校正）=====
// 为什么：飞轮转动时加速度角含巨大线加速度伪象（实测滑行段 accel-pitch 暴冲 +14°，
//   而 pitch 轴陀螺积分仅 0.03°）。低通滤波**无法**去除——伪象是低频实信号而非高频噪声。
//   互补滤波让陀螺主导动态（无滞后、抗线加速度），加速度仅做秒级慢校正消陀螺漂移。
//   详见 agent-context/decisions/003-imu-filtering.md。
// seedPitchDeg: 用当前加速度角播种；alpha: 陀螺权重(0.90~0.99)，越大越信陀螺、加速度校正越慢。
void   imuFusionInit(double seedPitchDeg, float alpha);
// 推进一步：传入本帧加速度 pitch(°)、pitch 轴陀螺率(已扣零偏, °/s)、dt(s)，返回融合 pitch(°)。
//   公式: fused = alpha*(fused + gyroRate*dt) + (1-alpha)*accelPitch
double imuFusionStepPitch(double accelPitchDeg, float gyroPitchRate, float dt_s);
// 当前融合 pitch（不重新计算，仅读状态）。
double imuFusedPitch();

// ===== 单轴 lateral(侧向/out-of-plane) 互补滤波（轴=gx，同 pitch 思路）=====
// 侧向加速度角在飞轮强力矩下同样有线加速度伪象；陀螺 gx 主导消伪象，使侧向安全判定可信。
void   imuFusionInitLateral(double seedLatDeg, float alpha);
double imuFusionStepLateral(double accelLatDeg, float gxRate, float dt_s);
double imuFusedLateral();
