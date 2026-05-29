/*
 * 屏动平衡系统 —— 第 1 步：IMU 姿态角读取
 * 硬件: M5Stack ATOM Matrix (MPU6886 IMU)
 *
 * 目标:
 *   读取 IMU，换算成两个姿态角（不输出原始加速度）:
 *     pitch 俯仰角 —— 绕 Y 轴前后俯仰
 *     roll  横滚/倾角 —— 绕 X 轴左右倾斜
 *   采样间隔 50ms (20Hz)，与原工程一致，保证实时性。
 *
 * 用途:
 *   通过物理倾斜设备，观察哪个角随系统“可活动方向”变化，
 *   以确定可动轴对应的是 pitch 还是 roll。
 */
#include <M5Atom.h>

// 采样间隔：沿用原工程的 50ms (20Hz)
static const uint32_t SAMPLE_INTERVAL_MS = 50;

static const double RAD2DEG = 180.0 / PI;

void setup() {
    // 初始化 ATOM-Matrix：串口 + I2C(IMU)，暂不使用 LED 点阵
    M5.begin(true, true, false);
    M5.IMU.Init();  // 初始化 MPU6886
    delay(100);
    Serial.println();
    Serial.println("# step1: IMU attitude (pitch/roll), 20Hz");
    Serial.println("# columns: pitch_deg, roll_deg");
}

void loop() {
    float accX = 0, accY = 0, accZ = 0;
    M5.IMU.getAccelData(&accX, &accY, &accZ);  // 单位: g

    // 由重力方向换算静态姿态角（atan2 给出完整量程）
    double pitch = atan2(-accX, sqrt(accY * accY + accZ * accZ)) * RAD2DEG;
    double roll  = atan2(accY, accZ) * RAD2DEG;

    Serial.printf("pitch=%7.2f  roll=%7.2f\n", pitch, roll);

    delay(SAMPLE_INTERVAL_MS);
}
