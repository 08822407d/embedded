#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <MadgwickAHRS.h>
#include <PID_v1.h>
#include "unit_rolleri2c.hpp"


#define IMU_SAMPLE_FREQ		100
#define IMU_SAMPLE_PERIOD	(1000 / (IMU_SAMPLE_FREQ))


UnitRollerI2C RollerI2C;  // Create a UNIT_ROLLERI2C object
Madgwick filter; 


// ---- PID相关变量（角度环） ----
double angleInput  = 0.0;   // 输入：当前角度
double angleOutput = 0.0;   // 输出：角度环PID结果
double angleSetpoint = 0.0; // 目标角度(若想保持水平，通常为0)


// ---- PID参数（示例值，需要根据实际情况调参） ----
double angleKp = 4000.0, angleKi = 0.0, angleKd = 10.0;

// ---- 建立PID对象 ----
// 角度PID：使用“DIRECT”或“REVERSE”，请根据实际方向调试
PID anglePID(&angleInput, &angleOutput, &angleSetpoint, angleKp, angleKi, angleKd, DIRECT);

// ---- 可选：对电机速度做简单低通滤波 ----
static double speedFilter = 0.0;

extern void selfBalanceTask(void * parameter);

void setup()
{
	Serial.begin(115200);
	Serial.printf("Serial print test\r\n");

	auto cfg = M5.config();
	M5.begin(cfg);
	M5.Lcd.setRotation(2);

	RollerI2C.begin(&Wire, 0x64, 2, 1, 400000);


	// 假设IMU更新频率约200Hz，可据实际情况修改
	filter.begin(IMU_SAMPLE_FREQ); 

	// 设置目标 = 0度
	angleSetpoint = 0.0;

	// PID 模式设置
	anglePID.SetMode(AUTOMATIC); 
	// 设置PID输出范围（根据需要可做限制）
	anglePID.SetOutputLimits(MIN_CURRENT, MAX_CURRENT);  // 示例


	RollerI2C.setOutput(0);
	RollerI2C.setMode(ROLLER_MODE_CURRENT);
	RollerI2C.setOutput(1);


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


// PID参数（示例值，需根据实际情况调参）
double Balance_KP_x = 50.0, Balance_KI_x = 0.0, Balance_KD_x = 4.0;
double Speed_KP_x   = -1.0, Speed_KI_x   = -0.5, Speed_KD_x   = 0.0;
// 重心偏移量（机械相关）
double Center_Gravity = 0.0;
// 角度 Integral（示例保留，如果需要，可根据原算法处理）
static double angleIntegral = 0.0;
// 目标为“零速度”平衡
static double targetSpeed = 0.0;
// 速度低通滤波用变量
// static double speedFilter = 0.0;
static double speedLastBias = 0.0;
static double speedIntegral = 0.0;

// 角度PID：根据倾角 Angle、角速度 Gyro，输出平衡控制量
int balance_x(float Angle, float Gyro) {
    // 原始算法中的位置式或者增量式控制，这里沿用位置式 PID
    double bias = Angle - Center_Gravity;
    angleIntegral += bias;
    // 限制积分范围
    if(angleIntegral > 30000) angleIntegral = 30000;
    if(angleIntegral < -30000) angleIntegral = -30000;

    // PD 或 PID 均可，这里保留 PD + 微分来自陀螺仪
    double result = Balance_KP_x * bias 
                  + Balance_KI_x * angleIntegral
                  + Balance_KD_x * Gyro; // 角速度作为微分量

    return (int)result;
}

// 速度PID：根据当前速度 speed，输出速度控制量
int velocity_x(double speed) {
    // 一阶低通滤波
    speedFilter = 0.65 * speedFilter + 0.35 * speed;

    // 位置式PID
    static double speedBiasIntegral = 0.0;
    double bias = speedFilter - targetSpeed;     // 偏差 = 当前速度 - 目标速度
    speedBiasIntegral += bias;

    // 防止积分过大
    if(speedBiasIntegral > 10000)  speedBiasIntegral = 10000;
    if(speedBiasIntegral < -10000) speedBiasIntegral = -10000;
    
    double result = Speed_KP_x * bias
                  + Speed_KI_x * speedBiasIntegral
                  + Speed_KD_x * (bias - speedLastBias);

    speedLastBias = bias;
    return (int)result;
}

// ---- 核心控制逻辑，每周期（如 10ms）调用 ----
void normal() {

	m5::IMU_Class::imu_data_t data;
	if (!M5.Imu.update())
		return;
	data = M5.Imu.getImuData();

	ax = data.accel.x;
	ay = data.accel.y;
	az = data.accel.z;
	gx = data.gyro.x;
	gy = data.gyro.y;
	gz = data.gyro.z;

	filter.updateIMU(gx, gy, gz, ax, ay, az);

    // 1. 获取姿态：通过 MadgwickAHRS 获取当前 Roll/Pitch/Yaw 或者需要的倾斜角/角速度
    //    这里假设需要绕 x 轴的角度 Angle_x 及相应的角速度 Gyro_x。
    float Angle_x = filter.getPitch();       // 假设 pitch 作为平衡角
    float Gyro_x  = gx;       // 获取 x 轴角速度 (deg/s 或 rad/s, 视库而定)

    // 2. 获取电机当前转速（>0 或 <0）
	double currentSpeed = RollerI2C.getSpeed();
	RollerI2C.setOutput(1);

    // 3. 根据转速对重心 Center_Gravity 做微调（原始算法逻辑）
    if (currentSpeed > 10)       Center_Gravity -= 0.001;
    else if (currentSpeed < -10) Center_Gravity += 0.001;
    if (currentSpeed > 20)       Center_Gravity -= 0.002;
    else if (currentSpeed < -20) Center_Gravity += 0.002;
    if (currentSpeed > 30)       Center_Gravity -= 0.005;
    else if (currentSpeed < -30) Center_Gravity += 0.005;

    // 4. 倾角 PD 控制
    int angleOutput  = balance_x(Angle_x, Gyro_x);

    // 5. 速度 PD 控制
    int speedOutput = velocity_x(currentSpeed);

    // 6. 计算最终输出
    int32_t finalSpeed = angleOutput + speedOutput;

    // 7. 将结果写入电机
    //    这里不再做“限幅-方向-PWM”的手动处理，而是直接调用 FOC 库
    //    假设 finalPwm 即可直接用 setSpeed()，具体要看电机库对正负值的处理约定
	RollerI2C.setSpeed(finalSpeed);
	RollerI2C.setOutput(1);
}