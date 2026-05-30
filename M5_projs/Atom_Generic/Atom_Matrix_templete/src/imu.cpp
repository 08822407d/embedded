#include <M5Atom.h>

#include "imu.h"

static const double RAD2DEG = 180.0 / PI;

// 直接写 MPU6886 寄存器（在 Wire1，地址 0x68）
static void mpu6886Write(uint8_t reg, uint8_t val) {
    Wire1.beginTransmission(0x68);
    Wire1.write(reg);
    Wire1.write(val);
    Wire1.endTransmission();
}

void imuInit() {
    M5.IMU.Init();  // 初始化 MPU6886
    // 收窄片上 DLPF 抗混叠：M5 默认陀螺~176Hz/加速度~218Hz，远超 50Hz 采样的 Nyquist(25Hz)，
    //   飞轮高转速振动会混叠进 gy → 融合角假尖峰（实测 12V/1330rpm 下假冲到 -35° 触发误保底）。
    //   陀螺 DLPF→20Hz(CFG=4)、加速度→21Hz(CFG=4)：远低于 Nyquist，仍保留摆体真实动态(<5Hz)。
    mpu6886Write(0x1A, 0x04);  // CONFIG: gyro DLPF_CFG=4 → 20Hz
    mpu6886Write(0x1D, 0x04);  // ACCEL_CONFIG2: accel DLPF_CFG=4 → 21Hz
    delay(2);
}

void imuReadRaw(float *ax, float *ay, float *az, float *gx, float *gy, float *gz) {
    M5.IMU.getAccelData(ax, ay, az);  // g
    M5.IMU.getGyroData(gx, gy, gz);   // °/s
}

void imuComputeAttitude(float ax, float ay, float az, double *pitch, double *roll) {
    *pitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD2DEG;
    *roll  = atan2(ay, az) * RAD2DEG;
}

void imuReadAttitude(double *pitch, double *roll) {
    float ax = 0, ay = 0, az = 0;
    M5.IMU.getAccelData(&ax, &ay, &az);
    imuComputeAttitude(ax, ay, az, pitch, roll);
}

// ===== 陀螺零偏 =====
static float  g_gBias[3] = {0, 0, 0};

// 加固版静止标定：丢弃开机首帧 + 运动门(采样窗内有晃动则作废重标) + 限次。
//   静止判据：单轴角速度峰峰 < QUIET_PP（实测静止噪声峰峰 < 0.7°/s，阈值留 ~4x 余量）。
//   角度无关——陀螺测角速度，静止时真值=0 与朝向无关，故对摆放倾角无要求，只要"别动"。
//   返回 true=拿到静止窗并已更新零偏；false=多次都没静止（零偏保持上次/0，调用方应中止）。
bool imuCalibrateGyro(uint16_t samples) {
    if (samples == 0) return false;
    const uint16_t WARMUP   = 20;     // 丢弃开机后头 ~100ms（IMU 内部滤波/FIFO 未稳）
    const float    QUIET_PP = 3.0f;   // 静止运动门：单轴峰峰阈值（°/s）
    const uint8_t  MAX_TRY  = 5;

    for (uint8_t attempt = 1; attempt <= MAX_TRY; attempt++) {
        float a, b, c, d, e, f;
        for (uint16_t i = 0; i < WARMUP; i++) { imuReadRaw(&a, &b, &c, &d, &e, &f); delay(5); }

        double sx = 0, sy = 0, sz = 0;
        float mn[3], mx[3];
        for (uint16_t i = 0; i < samples; i++) {
            float ax, ay, az, gx, gy, gz;
            imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
            sx += gx; sy += gy; sz += gz;
            float g[3] = {gx, gy, gz};
            for (int k = 0; k < 3; k++) {
                if (i == 0 || g[k] < mn[k]) mn[k] = g[k];
                if (i == 0 || g[k] > mx[k]) mx[k] = g[k];
            }
            delay(5);  // ~200Hz，samples=200 → ~1s
        }
        float ppx = mx[0] - mn[0], ppy = mx[1] - mn[1], ppz = mx[2] - mn[2];
        if (ppx < QUIET_PP && ppy < QUIET_PP && ppz < QUIET_PP) {
            g_gBias[0] = (float)(sx / samples);
            g_gBias[1] = (float)(sy / samples);
            g_gBias[2] = (float)(sz / samples);
            return true;
        }
        Serial.printf("# 陀螺标定第%u次检测到运动(峰峰 gx=%.2f gy=%.2f gz=%.2f °/s>%.1f)，请静置，重标...\n",
                      attempt, ppx, ppy, ppz, QUIET_PP);
    }
    return false;  // 始终未静止 → 放弃，零偏不更新
}

void imuGyroBias(float *bx, float *by, float *bz) {
    if (bx) *bx = g_gBias[0];
    if (by) *by = g_gBias[1];
    if (bz) *bz = g_gBias[2];
}

// ===== 单轴 pitch 互补滤波 =====
static double g_fusePitch = 0;
static float  g_alpha     = 0.98f;

void imuFusionInit(double seedPitchDeg, float alpha) {
    g_fusePitch = seedPitchDeg;
    g_alpha     = alpha;
}

double imuFusionStepPitch(double accelPitchDeg, float gyroPitchRate, float dt_s) {
    double gyroPredict = g_fusePitch + (double)gyroPitchRate * (double)dt_s;  // 陀螺主导(无滞后)
    g_fusePitch = g_alpha * gyroPredict + (1.0 - g_alpha) * accelPitchDeg;    // 加速度慢校正
    return g_fusePitch;
}

double imuFusedPitch() {
    return g_fusePitch;
}

// ===== 单轴 lateral(侧向/out-of-plane) 互补滤波（同 pitch 思路，轴=gx）=====
//   侧向加速度角同样在飞轮强力矩下有线加速度伪象；用 gx(扣零偏)主导消伪象。
static double g_fuseLat   = 0;
static float  g_alphaLat  = 0.98f;

void imuFusionInitLateral(double seedLatDeg, float alpha) {
    g_fuseLat  = seedLatDeg;
    g_alphaLat = alpha;
}

double imuFusionStepLateral(double accelLatDeg, float gxRate, float dt_s) {
    double pred = g_fuseLat + (double)gxRate * (double)dt_s;
    g_fuseLat = g_alphaLat * pred + (1.0 - g_alphaLat) * accelLatDeg;
    return g_fuseLat;
}

double imuFusedLateral() {
    return g_fuseLat;
}
