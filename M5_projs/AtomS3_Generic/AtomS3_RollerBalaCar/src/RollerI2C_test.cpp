#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <MadgwickAHRS.h>
#include <PID_v1.h>
#include "unit_rolleri2c.hpp"


#define IMU_SAMPLE_FREQ		100
#define IMU_SAMPLE_PERIOD	(1000 / (IMU_SAMPLE_FREQ))


UnitRollerI2C RollerI2C_1;  // Create a UNIT_ROLLERI2C object
UnitRollerI2C RollerI2C_2;  // Create a UNIT_ROLLERI2C object
Madgwick filter; 


// ---- PID相关变量（角度环） ----
double angleInput  = 0.0;   // 输入：当前角度
double angleOutput = 0.0;   // 输出：角度环PID结果
double angleSetpoint = 0.0; // 目标角度(若想保持水平，通常为0)

// ---- PID相关变量（速度环） ----
double speedInput  = 0.0;   // 输入：当前速度
double speedOutput = 0.0;   // 输出：速度环PID结果
double speedSetpoint = 0.0; // 目标速度(平衡小车设为0)

// ---- PID参数（示例值，需要根据实际情况调参） ----
double angleKp = 500.0, angleKi = 0.5, angleKd = 50.0;
double speedKp = 100.0, speedKi = 0.0, speedKd = 0.5;

// ---- 建立PID对象 ----
// 角度PID：使用“DIRECT”或“REVERSE”，请根据实际方向调试
PID anglePID(&angleInput, &angleOutput, &angleSetpoint, angleKp, angleKi, angleKd, DIRECT);
// 速度PID：同上
PID speedPID(&speedInput, &speedOutput, &speedSetpoint, speedKp, speedKi, speedKd, DIRECT);

// ---- 可选：对电机速度做简单低通滤波 ----
static double speedFilter = 0.0;


extern void selfBalanceTask(void * parameter);

void setup()
{
	auto cfg = M5.config();
	M5.begin(cfg);
	Serial.begin(115200);
	Serial.printf("Serial print test\r\n");
	RollerI2C_1.begin(0x64, 2, 1, 400000UL);
	RollerI2C_2.begin(0x65, 2, 1, 400000UL);


	// 假设IMU更新频率约200Hz，可据实际情况修改
	filter.begin(IMU_SAMPLE_FREQ); 

	// 设置目标 = 0度
	angleSetpoint = 0.0;
	speedSetpoint = 0.0;

	// PID 模式设置
	anglePID.SetMode(AUTOMATIC); 
	speedPID.SetMode(AUTOMATIC);
	// 设置PID输出范围（根据需要可做限制）
	anglePID.SetOutputLimits(MIN_CURRENT, MAX_CURRENT);  // 示例
	speedPID.SetOutputLimits(MIN_CURRENT, MAX_CURRENT);


	RollerI2C_1.setOutput(0);
	RollerI2C_1.setMode(ROLLER_MODE_CURRENT);
	RollerI2C_1.setOutput(1);

	RollerI2C_2.setOutput(0);
	RollerI2C_2.setMode(ROLLER_MODE_CURRENT);
	RollerI2C_2.setOutput(1);


	// 创建FreeRTOS任务
	xTaskCreate(
		selfBalanceTask,			// Task function
		"SelfBalance Task",			// Task name
		8192,						// Stack size
		NULL,						// Task input parameter
		5,							// Priority
		NULL						// Task handle
	);
}

void loop()
{
	delay(1000);
}


float ax, ay, az, gx, gy, gz, mx, my, mz;
int printCounter;
// 任务实现
void selfBalanceTask(void * parameter) {
	(void) parameter; // 避免未使用参数的编译警告
	uint32_t last_ticks = xTaskGetTickCount();

	while (1) {
		// 记录任务开始时间
		TickType_t taskStart = xTaskGetTickCount();


		// extern void normal();
		// normal();

		m5::IMU_Class::imu_data_t data;
		if (!M5.Imu.update())
			continue;;
		data = M5.Imu.getImuData();

		ax = data.accel.x;
		ay = data.accel.y;
		az = data.accel.z;
		gx = data.gyro.x;
		gy = data.gyro.y;
		gz = data.gyro.z;

		filter.updateIMU(gx, gy, gz, ax, ay, az);

		angleInput = filter.getPitch();

		// 2. 从电机库获取速度(编码器/无感测的封装都由MotorFOC库自行处理)
		double currentSpeed_1 = RollerI2C_1.getSpeedReadback() / 100.f;
		double currentSpeed_2 = RollerI2C_2.getSpeedReadback() / 100.f;

		// 可选：一阶低通滤波，减少速度抖动
		speedFilter = 0.65 * speedFilter + 0.35 * (currentSpeed_1 + currentSpeed_2) / 2.0;
		speedInput  = speedFilter;  // 若不需要滤波，可直接 speedInput = currentSpeed;

		// 3. 计算角度PID
		anglePID.Compute();  // 执行PID计算，输出存到 angleOutput
		// 4. 计算速度PID
		speedPID.Compute();  // 执行PID计算，输出存到 speedOutput
		// 5. 叠加两个环的结果
		double finalCommand = angleOutput + speedOutput;
		// double finalCommand = angleOutput;

		// 调试输出（减少打印频率以避免延迟）
		printCounter++;
		if (printCounter >= 5) {
			printCounter = 0;
			M5.Lcd.setCursor(10, 10);
			M5.Lcd.fillRect(10, 10, 50, 40, TFT_BLACK);
			M5.Lcd.printf("pitch %.2f\r\n", angleInput);
			M5.Lcd.printf("speed: %-4.2f\r\n", finalCommand);
			M5.Lcd.printf("speed_1: %-4.2f\r\n", currentSpeed_1);
			M5.Lcd.printf("speed_2: %-4.2f\r\n", currentSpeed_2);
		}


		RollerI2C_1.setCurrent(finalCommand);
		RollerI2C_1.setOutput(1);

		RollerI2C_2.setCurrent(-finalCommand);
		RollerI2C_2.setOutput(1);


		// // 计算任务执行时间
		// TickType_t taskEnd = xTaskGetTickCount();
		// TickType_t elapsedTime = taskEnd - taskStart;
		// // 计算剩余延迟时间
		// uint32_t delayTime = (IMU_SAMPLE_PERIOD > (elapsedTime * portTICK_PERIOD_MS)) ? 
		// 					  IMU_SAMPLE_PERIOD - (elapsedTime * portTICK_PERIOD_MS) : 0;
		// // 延迟指定时间后再次执行
		// vTaskDelay(pdMS_TO_TICKS(delayTime));
		vTaskDelayUntil(&last_ticks, pdMS_TO_TICKS(IMU_SAMPLE_PERIOD));
	}
}