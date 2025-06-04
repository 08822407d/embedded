#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <MadgwickAHRS.h>
#include <PID_v1.h>

#include <unit_rolleri2c.hpp>

#include "config.hpp"


TaskHandle_t	imuTaskHandle = NULL;
UnitRollerI2C	RollerI2C;  // Create a UNIT_ROLLERI2C object
Madgwick		filter; 

int16_t		ax, ay, az,		//三轴陀螺仪
			gx, gy, gz,		//三轴加速度
			mx, my, mz;		//三轴磁力计
float		AngleX,
			AngleY,
			AngleZ;			//三轴角度
int			printCounter;


// ---- PID相关变量（角度环） ----
double angleInput  = 0.0;   // 输入：当前角度
double angleOutput = 0.0;   // 输出：角度环PID结果
double angleSetpoint = 0.0; // 目标角度(若想保持水平，通常为0)

// ---- PID参数（示例值，需要根据实际情况调参） ----
double angleKp = 1000.0, angleKi = 0.0, angleKd = 100.0;
// ---- 建立PID对象 ----
// 角度PID：使用“DIRECT”或“REVERSE”，请根据实际方向调试
PID anglePID(&angleInput, &angleOutput, &angleSetpoint, angleKp, angleKi, angleKd, DIRECT);


// ---- 可选：对电机速度做简单低通滤波 ----
static double speedFilter = 0.0;

extern void imuTask(void *pvParameters);
extern void selfBalanceTask(void * parameter);
extern void normal(void);

void setup()
{
	delay(500);				// 等待一段时间以确保系统稳定
	Serial.begin(115200);
	Serial.printf("Serial print test\r\n");

	Serial.println("Initiating MCU ...");
	auto cfg = M5.config();
	M5.begin(cfg);
	M5.Lcd.setRotation(2);


	Serial.println("Initiating Algos ...");
	// 假设IMU更新频率约200Hz，可据实际情况修改
	filter.begin(IMU_SAMPLE_FREQ); 
	// 设置目标 = 0度
	angleSetpoint = 0.0;
	// PID 模式设置
	anglePID.SetMode(AUTOMATIC); 
	// 设置PID输出范围（根据需要可做限制）
	anglePID.SetOutputLimits(MIN_CURRENT, MAX_CURRENT);
	delay(200);				// 等待一段时间以确保系统稳定


	// Serial.println("Initiating FOC Motor ...");
	// RollerI2C.begin(&Wire, 0x64, 2, 1, 115200);
	// RollerI2C.setOutput(0);
	// RollerI2C.setMode(ROLLER_MODE_CURRENT);
	// RollerI2C.setOutput(1);
	// delay(200);				// 等待一段时间以确保系统稳定


	Serial.println("Starting IMU Task ...");
	// 创建IMU任务
	xTaskCreatePinnedToCore(
		imuTask,			// 任务函数
		"IMU Task",			// 任务名称
		4096,				// 栈大小（字节）
		NULL,				// 任务参数
		3,					// 任务优先级
		&imuTaskHandle,		// 任务句柄
		1					// 绑定到核心 1
	);
	delay(200);							//延时等待初始化完成

	// // 创建FreeRTOS任务
	// xTaskCreate(
	// 	selfBalanceTask,			// Task function
	// 	"SelfBalance Task",			// Task name
	// 	8192,						// Stack size
	// 	NULL,						// Task input parameter
	// 	5,							// Priority
	// 	NULL						// Task handle
	// );

	Serial.println("Finished Initiation.");
}

void loop()
{
	Serial.printf("%4.2f\n", AngleX);

	// vTaskDelay(pdMS_TO_TICKS(50));
	delay(20);
}

void imuTask(void *pvParameters) {
	const TickType_t xFrequency = pdMS_TO_TICKS(5); // 5 毫秒
	TickType_t xLastWakeTime = xTaskGetTickCount();
	m5::IMU_Class::imu_data_t data;

	for (;;) {
		M5.Imu.update();
		// if (!M5.Imu.update())
		// 	continue;
		data = M5.Imu.getImuData();

		ax = data.accel.x;
		ay = data.accel.y;
		az = data.accel.z;
		gx = data.gyro.x;
		gy = data.gyro.y;
		gz = data.gyro.z;
		
		filter.updateIMU(gx, gy, gz, ax, ay, az);
		AngleX = filter.getPitch(); // 获取当前 Pitch 角度
		AngleY = filter.getRoll();  // 获取当前 Roll 角度
		AngleZ = filter.getYaw();   // 获取当前 Yaw 角度

		// 延迟至下一个周期
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

// 任务实现
void selfBalanceTask(void * parameter) {
	(void) parameter; // 避免未使用参数的编译警告
	uint32_t last_ticks = xTaskGetTickCount();

	while (1) {
		// 记录任务开始时间
		TickType_t taskStart = xTaskGetTickCount();

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
		double currentSpeed = RollerI2C.getSpeedReadback() / 100.0f;

		// 3. 计算角度PID
		anglePID.Compute();  // 执行PID计算，输出存到 angleOutput
		double finalCommand = angleOutput;

		// 调试输出（减少打印频率以避免延迟）
		printCounter++;
		if (printCounter >= 5) {
			printCounter = 0;
			M5.Lcd.setCursor(10, 10);
			M5.Lcd.fillRect(10, 10, 50, 40, TFT_BLACK);
			M5.Lcd.printf("pitch %.2f\r\n", angleInput);
			M5.Lcd.printf("value: %-4.2f\r\n", finalCommand);
			M5.Lcd.printf("speed: %-4.2f\r\n", currentSpeed);
		}


		RollerI2C.setCurrent(finalCommand);
		RollerI2C.setOutput(1);


		// // 延迟指定时间后再次执行
		// vTaskDelay(pdMS_TO_TICKS(delayTime));
		vTaskDelayUntil(&last_ticks, pdMS_TO_TICKS(IMU_SAMPLE_PERIOD));
	}
}