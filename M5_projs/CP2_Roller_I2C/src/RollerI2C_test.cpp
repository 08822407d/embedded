#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <MadgwickAHRS.h>
#include <PID_v1.h>
#include "unit_rolleri2c.hpp"


#define IMU_SAMPLE_FREQ		300
#define IMU_SAMPLE_PERIOD	(1000 / (IMU_SAMPLE_FREQ))
#define MAX_CURRENT			120000


UnitRollerI2C RollerI2C;  // Create a UNIT_ROLLERI2C object
Madgwick filter; 


// PID参数（根据实际情况调优）
double Kp = 10000.0; 
double Ki = 10.0;
double Kd = 1000.0; 

// PID相关
double pitchInput, pitchOutput, pitchSetpoint;
PID pitchPID(&pitchInput, &pitchOutput, &pitchSetpoint, Kp, Ki, Kd, DIRECT);



extern void selfBalanceTask(void * parameter);

void setup()
{
	auto cfg = M5.config();
	M5.begin(cfg);
	Serial.begin(115200);
	Serial.printf("Serial print test\r\n");
	RollerI2C.begin(&Wire, 0x64, 2, 1, 400000);


	// 假设IMU更新频率约200Hz，可据实际情况修改
	filter.begin(IMU_SAMPLE_FREQ); 

	// 设置目标 = 0度
	pitchSetpoint = 3.0;

	pitchPID.SetMode(AUTOMATIC);
	pitchPID.SetOutputLimits(-MAX_CURRENT, MAX_CURRENT);


	RollerI2C.setOutput(0);
	RollerI2C.setMode(ROLLER_MODE_CURRENT);
	// RollerI2C.setMode(ROLLER_MODE_SPEED);
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

		while (1) {
		// 记录任务开始时间
		TickType_t taskStart = xTaskGetTickCount();


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

		pitchInput = filter.getPitch();
		pitchPID.Compute();

		// 调试输出（减少打印频率以避免延迟）
		printCounter++;
		if (printCounter >= 5) {
			printCounter = 0;
			M5.Lcd.setCursor(10, 10);
			M5.Lcd.fillRect(10, 10, 50, 40, TFT_BLACK);
			M5.Lcd.printf("%.2f\r\n", pitchInput);
			M5.Lcd.printf("PID: %-4.2f\r\n", pitchOutput);
		}

		RollerI2C.setOutput(0);
		RollerI2C.setCurrent(pitchOutput);
		RollerI2C.setOutput(1);


		// 计算任务执行时间
		TickType_t taskEnd = xTaskGetTickCount();
		TickType_t elapsedTime = taskEnd - taskStart;

		// 计算剩余延迟时间
		uint32_t delayTime = (IMU_SAMPLE_PERIOD > (elapsedTime * portTICK_PERIOD_MS)) ? 
							  IMU_SAMPLE_PERIOD - (elapsedTime * portTICK_PERIOD_MS) : 0;

		// 延迟指定时间后再次执行
		vTaskDelay(pdMS_TO_TICKS(delayTime));
	}
}



// #include "unit_roller485.hpp"
// #include <Arduino.h>
// #include <M5Unified.h>

// UnitRoller485 Roller485;     // Create a UnitRoller485 object
// HardwareSerial mySerial(1);  // Create a hardware serial port object
// #define motor485Id 0x00
// uint8_t rgbValues[3];  // Array to store R, G, B values
// double speedPID[3];    // Array to   P I D values
// void setup()
// {
//     M5.begin();
//     // Call the begin function and pass arguments
//     Roller485.begin(&mySerial, 115200, SERIAL_8N1, 22, 19, -1, false, 10000UL, 112U);
//     // Set the motor mode to speed
//     Roller485.setMode(motor485Id, ROLLER_MODE_SPEED);
//     // Set speed Speed and current
//     Roller485.setSpeedMode(motor485Id, 2400, 1200);
//     // Set the motor to enable
//     Roller485.setOutput(motor485Id, true);
// }

// void loop()
// {
//     int errorCode = Roller485.setMode(motor485Id, ROLLER_MODE_SPEED);
//     if (errorCode == 1) {
//         // Successful operation
//         Serial.println("setMode() successful.");
//     } else {
//         // Print error code
//         Serial.print("setMode() failed with error code: ");
//         Serial.println(errorCode);
//     }
//     delay(1000);

//     int32_t temperature = Roller485.getActualTemp(motor485Id);
//     Serial.printf("temperature: %d\n", temperature);
//     delay(1000);
//     int32_t vin = Roller485.getActualVin(motor485Id);
//     Serial.printf("vin: %d\n", vin);
//     delay(1000);
//     int32_t encoder = Roller485.getEncoder(motor485Id);
//     Serial.printf("encoder: %d\n", encoder);
//     delay(1000);
//     int rgbBrightness = Roller485.getRGBBrightness(motor485Id);
//     Serial.printf("rgbBrightness: %d\n", rgbBrightness);
//     delay(1000);
//     int32_t motorId = Roller485.getMotorId(motor485Id);
//     Serial.printf("motorId: %d\n", motorId);
//     delay(1000);
//     int32_t speed = Roller485.getActualSpeed(motor485Id);
//     Serial.printf("speed: %d\n", speed);
//     delay(1000);
//     int32_t position = Roller485.getActualPosition(motor485Id);
//     Serial.printf("position: %d\n", position);
//     delay(1000);
//     int32_t current = Roller485.getActualCurrent(motor485Id);
//     Serial.printf("current: %d\n", current);
//     delay(1000);
//     int8_t mode = Roller485.getMode(motor485Id);
//     Serial.printf("mode: %d\n", mode);
//     delay(1000);
//     int8_t status = Roller485.getStatus(motor485Id);
//     Serial.printf("status: %d\n", status);
//     delay(1000);
//     int8_t error = Roller485.getError(motor485Id);
//     Serial.printf("error: %d\n", error);
//     int8_t errorRGB = Roller485.getRGB(motor485Id, rgbValues);
//     if (errorRGB != 1) {
//         // Print error code
//         Serial.print("getRGB failed with error code: ");
//         Serial.println(errorRGB);
//     } else {
//         printf("RGB values: R=%d, G=%d, B=%d\n", rgbValues[0], rgbValues[1], rgbValues[2]);
//     }

//     int8_t errorSpeedPID = Roller485.getSpeedPID(motor485Id, speedPID);
//     if (errorSpeedPID != 1) {
//         Serial.print("getSpeedPID failed with error code: ");
//         Serial.println(errorSpeedPID);
//     } else {
//         printf("speedPID values: P=%.8lf, I=%.8lf, D=%.8lf\n", speedPID[0], speedPID[1], speedPID[2]);
//     }

//     delay(1000);
// }