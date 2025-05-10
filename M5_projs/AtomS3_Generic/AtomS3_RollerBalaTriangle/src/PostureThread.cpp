#include <Arduino.h>
#include <M5Unified.h>
#include <MadgwickAHRS.h>
#include <SimpleKalmanFilter.h>  // 单变量卡尔曼滤波库
#include <1EuroFilter.h>     // OneEuro 滤波器库

#include <unit_rolleri2c.hpp>

#include "config.hpp"


#define SENSOR_TASK_PRIO 5           // 传感器任务优先级 (数值越大优先级越高)

extern UnitRollerI2C RollerI2C;

// 全局共享数据和同步
float filteredAngle = 0.0f;          // 滤波后的姿态角 (俯仰角) - 共享变量
float filteredSpeed = 0.0f;          // 滤波后的电机速度 - 共享变量
SemaphoreHandle_t dataMutex;         // 互斥锁，用于保护共享数据

// 传感器和滤波对象
Madgwick ahrs;    // MadgwickAHRS 姿态解算滤波器对象
SimpleKalmanFilter angleKalmanFilter(0.5 /*测量噪声*/, 0.5 /*估计噪声*/, 0.01 /*过程噪声*/);  
// 卡尔曼滤波器对象 (用于姿态角平滑), 初始参数需根据实际情况调整
OneEuroFilter speedFilter;   // OneEuro 低通滤波器对象 (用于电机速度平滑)


float getSystemAngle() {
	// float retval;
	// xSemaphoreTake(dataMutex, portMAX_DELAY);
	// retval = filteredAngle;
	// xSemaphoreGive(dataMutex);
	// return retval;

	return filteredAngle;
}

float getMotorSpeed() {
	// float retval;
	// xSemaphoreTake(dataMutex, portMAX_DELAY);
	// retval = filteredSpeed;
	// xSemaphoreGive(dataMutex);
	// return retval;

	return filteredSpeed;
}


// 传感器任务函数：读取传感器并进行滤波处理
void postureTask(void *pvParameters) {
	TickType_t lastWakeTime = xTaskGetTickCount();
	const TickType_t interval = pdMS_TO_TICKS(IMU_SAMPLE_PERIOD);
	// 初始化 OneEuro 滤波器参数（频率、最小截止频率、β系数）
	speedFilter.begin((float)IMU_SAMPLE_FREQ, 2.0f, 0.1f);  // minCutoff=2Hz, beta=0.1 (需根据实际调节)

	for (;;) {
		m5::IMU_Class::imu_data_t data;
		if (!M5.Imu.update())
			continue;

		// 1. 读取 IMU 传感器原始数据
		float ax, ay, az, gx, gy, gz, mx, my, mz;
		data = M5.Imu.getImuData();
		ax = data.accel.x;
		ay = data.accel.y;
		az = data.accel.z;
		gx = data.gyro.x;
		gy = data.gyro.y;
		gz = data.gyro.z;
		mx = data.mag.x;
		my = data.mag.y;
		mz = data.mag.z;

		// 2. Madgwick AHRS 姿态解算 (陀螺仪 + 加速度计, 更新内部四元数状态)
		ahrs.updateIMU(gx, gy, gz, ax, ay, az);
		// 3. 获取欧拉角（单位：度）- 取俯仰角 pitch 作为小车倾斜角
		float pitch = ahrs.getPitch();
		// 4. 对俯仰角应用卡尔曼滤波进行平滑
		float filteredPitch = angleKalmanFilter.updateEstimate(pitch);
		// 5. 读取电机速度，并应用 OneEuro 滤波进行平滑
		float speed = RollerI2C.getSpeedReadback() / 100.0f;
		float filteredMotorSpeed = speedFilter.filter(speed);
		// 6. 将滤波后的结果写入共享变量 (使用互斥锁保护临界区)
		// xSemaphoreTake(dataMutex, portMAX_DELAY);
		filteredAngle = filteredPitch;
		filteredSpeed = filteredMotorSpeed;
		// xSemaphoreGive(dataMutex);

		Serial.println("Angle: " + String(filteredAngle) + "; speed: " + String(filteredSpeed));
		// 7. 周期延时，确保任务以 200Hz 周期运行
		vTaskDelayUntil(&lastWakeTime, interval);
	}
}

void creatPostureThread() {
	// 创建互斥锁
	dataMutex = xSemaphoreCreateMutex();
	if (dataMutex == NULL) {
		Serial.println("Failed to create mutex");
		return;
	}
	ahrs.begin(IMU_SAMPLE_FREQ);  // 初始化姿态解算器，设置采样频率
	// 创建传感器任务
	xTaskCreate(postureTask, "PostureTask", 2 * STACK_SIZE, NULL, SENSOR_TASK_PRIO, NULL);
}