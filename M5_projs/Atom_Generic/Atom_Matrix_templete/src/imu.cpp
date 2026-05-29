#include <M5Atom.h>

#include "imu.h"

static const double RAD2DEG = 180.0 / PI;

void imuInit() {
    M5.IMU.Init();  // 初始化 MPU6886
}

void imuReadAttitude(double *pitch, double *roll) {
    float accX = 0, accY = 0, accZ = 0;
    M5.IMU.getAccelData(&accX, &accY, &accZ);  // 单位: g

    // 由重力方向换算静态姿态角（atan2 给出完整量程）
    *pitch = atan2(-accX, sqrt(accY * accY + accZ * accZ)) * RAD2DEG;
    *roll  = atan2(accY, accZ) * RAD2DEG;
}
