#include <M5Atom.h>

#include "imu.h"

static const double RAD2DEG = 180.0 / PI;

void imuInit() {
    M5.IMU.Init();  // 初始化 MPU6886
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
