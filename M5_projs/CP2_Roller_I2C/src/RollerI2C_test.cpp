#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <MadgwickAHRS.h>
#include <PID_v1.h>
#include "unit_rolleri2c.hpp"


#define IMU_SAMPLE_FREQ		200
#define IMU_SAMPLE_PERIOD	(1000 / (IMU_SAMPLE_FREQ))


UnitRollerI2C RollerI2C;  // Create a UNIT_ROLLERI2C object
Madgwick filter; 

// PID相关
double rollInput, rollOutput, rollSetpoint;

// PID参数（根据实际情况调优）
double Kp = 10.0; 
double Ki = 0.5;
double Kd = 0.1; 

PID rollPID(&rollInput, &rollOutput, &rollSetpoint, Kp, Ki, Kd, DIRECT);

// PID相关
double pitchInput, pitchOutput, pitchSetpoint;

PID pitchPID(&pitchInput, &pitchOutput, &pitchSetpoint, Kp, Ki, Kd, DIRECT);

// 使用FreeRTOS计时
TickType_t lastUpdate;


void setup()
{
	auto cfg = M5.config();
	M5.begin(cfg);
	Serial.begin(115200);
	Serial.printf("Serial print test\r\n");
	RollerI2C.begin(&Wire, 0x64, 2, 1, 400000);


	// 假设IMU更新频率约200Hz，可据实际情况修改
	filter.begin(IMU_SAMPLE_FREQ); 
	lastUpdate = xTaskGetTickCount(); // 获取当前tick计数

	// 设置目标roll = 0度
	rollSetpoint = 0.0;

	rollPID.SetMode(AUTOMATIC);
	rollPID.SetOutputLimits(-1000, 1000);
	pitchPID.SetMode(AUTOMATIC);
	pitchPID.SetOutputLimits(-1000, 1000);


	RollerI2C.setOutput(0);
	// RollerI2C.setMode(ROLLER_MODE_POSITION);
	// RollerI2C.setPos(0);
	RollerI2C.setMode(ROLLER_MODE_SPEED);
	RollerI2C.setSpeed(0);
	RollerI2C.setOutput(1);
}

void loop()
{
	float ax, ay, az, gx, gy, gz;
	m5::IMU_Class::imu_data_t data;
	// 排空IMU的FIFO中的数据
	while (M5.Imu.update()) {
		data = M5.Imu.getImuData();
	}

	ax = data.accel.x;
	ay = data.accel.y;
	az = data.accel.z;
	gx = data.gyro.x;
	gy = data.gyro.y;
	gz = data.gyro.z;

		// 获取当前tick
		TickType_t now = xTaskGetTickCount();

		// 计算时间间隔（秒）
		float dt = (now - lastUpdate) * portTICK_PERIOD_MS / 1000.0f;
		lastUpdate = now;

		// 将陀螺仪读数从deg/s转为rad/s（如果Madgwick库需要）
		const float deg2rad = 3.14159265358979f / 180.0f;
		filter.updateIMU(gx * deg2rad, gy * deg2rad, gz * deg2rad, ax, ay, az);

		double roll = filter.getRoll(); // 获取roll角度
		double pitch = filter.getPitch(); // 获取roll角度
		
		// // 将roll作为PID输入
		// rollInput = roll;
		// rollPID.Compute();

		// pitchInput = pitch;
		// pitchPID.Compute();

		// 调试输出（减少打印频率以避免延迟）
		static int printCounter = 0;
		printCounter++;
		if (printCounter >= 10) { // 每100次循环打印一次
			printCounter = 0;
			
			M5.Lcd.setCursor(10, 10);
			M5.Lcd.clear();
			M5.Lcd.printf("%.2f\r\n", roll);
			M5.Lcd.printf("%.2f\r\n", pitch);
		}
		// M5.Lcd.printf("PID - roll: %-4.2f\r\n", rollOutput);
		// M5.Lcd.printf("PID - pitch: %-4.2f\r\n", pitchOutput);
		// // 将PID输出作为电机速度指令
		// motor.setSpeed(rollOutput);

		// // 输出调试信息
		// Serial.print("Roll: "); Serial.print(roll, 2);
		// Serial.print(" | Output: "); Serial.print(rollOutput, 2);
		// Serial.print(" | dt: "); Serial.println(dt, 4);


		// RollerI2C.setOutput(0);
		// RollerI2C.setSpeed(2000 * pitch);
		// RollerI2C.setPos(pitch * 10000);
		// RollerI2C.setOutput(1);

	// 根据需要调整循环延时
	vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_PERIOD));


	// // current mode
	// RollerI2C.setMode(ROLLER_MODE_ENCODER);
	// RollerI2C.setCurrent(120000);
	// RollerI2C.setOutput(1);
	// printf("current: %d\n", RollerI2C.getCurrent());
	// delay(100);
	// printf("actualCurrent: %d\n", RollerI2C.getCurrentReadback() / 100.0f);
	// delay(5000);

	// // position mode
	// RollerI2C.setOutput(0);
	// RollerI2C.setMode(ROLLER_MODE_SPEED);
	// RollerI2C.setPos(2000000);
	// RollerI2C.setPosMaxCurrent(100000);
	// RollerI2C.setOutput(1);
	// RollerI2C.getPosPID(&p, &i, &d);
	// printf("PosPID  P: %3.8f  I: %3.8f  D: %3.8f\n", p / 100000.0, i / 10000000.0, d / 100000.0);
	// delay(100);
	// printf("pos: %d\n", RollerI2C.getPos());
	// delay(100);
	// printf("posMaxCurrent: %d\n", RollerI2C.getPosMaxCurrent() / 100.0f);
	// delay(100);
	// printf("actualPos: %d\n", RollerI2C.getPosReadback() / 100.f);
	// delay(5000);

	// // speed mode
	// RollerI2C.setOutput(0);
	// RollerI2C.setMode(ROLLER_MODE_SPEED);
	// RollerI2C.setSpeed(240000);
	// RollerI2C.setSpeedMaxCurrent(100000);
	// RollerI2C.setOutput(1);
	// RollerI2C.getSpeedPID(&p, &i, &d);
	// printf("SpeedPID  P: %3.8f  I: %3.8f  D: %3.8f\n", p / 100000.0, i / 10000000.0, d / 100000.0);
	// delay(100);
	// printf("speed: %d\n", RollerI2C.getSpeed());
	// delay(100);
	// printf("speedMaxCurrent: %d\n", RollerI2C.getSpeedMaxCurrent() / 100.0f);
	// delay(100);
	// printf("actualSpeed: %d\n", RollerI2C.getSpeedReadback() / 100.0f);
	// delay(5000);

	// // encoder mode
	// RollerI2C.setOutput(0);
	// RollerI2C.setMode(ROLLER_MODE_ENCODER);
	// RollerI2C.setDialCounter(240000);
	// RollerI2C.setOutput(1);
	// printf("DialCounter:%d\n", RollerI2C.getDialCounter());
	// delay(5000);
	// printf("temp:%d\n", RollerI2C.getTemp());
	// delay(100);
	// printf("Vin:%3.2f\n", RollerI2C.getVin() / 100.0);
	// delay(100);
	// printf("RGBBrightness:%d\n", RollerI2C.getRGBBrightness());
	// delay(1000);
	// RollerI2C.setRGBBrightness(100);
	// delay(100);
	// RollerI2C.setRGBMode(ROLLER_RGB_MODE_USER_DEFINED);
	// delay(1000);
	// RollerI2C.setRGB(TFT_WHITE);
	// delay(1000);
	// RollerI2C.setRGB(TFT_BLUE);
	// delay(2000);
	// RollerI2C.setRGB(TFT_YELLOW);
	// delay(2000);
	// RollerI2C.setRGB(TFT_RED);
	// delay(2000);
	// RollerI2C.setRGBMode(ROLLER_RGB_MODE_DEFAULT);
	// delay(100);
	// RollerI2C.setKeySwitchMode(1);
	// delay(100);
	// printf("I2CAddress:%d\n", RollerI2C.getI2CAddress());
	// delay(100);
	// printf("485 BPS:%d\n", RollerI2C.getBPS());
	// delay(100);
	// printf("485 motor id:%d\n", RollerI2C.getMotorID());
	// delay(100);
	// printf("motor output:%d\n", RollerI2C.getOutputStatus());
	// delay(100);
	// printf("SysStatus:%d\n", RollerI2C.getSysStatus());
	// delay(100);
	// printf("ErrorCode:%d\n", RollerI2C.getErrorCode());
	// delay(100);
	// printf("Button switching mode enable:%d\n", RollerI2C.getKeySwitchMode());
	// delay(100);
	// RollerI2C.getRGB(&r, &g, &b);
	// printf("RGB-R: 0x%02X  RGB-G: 0x%02X  RGB-B: 0x%02X\n", r, g, b);
	// delay(5000);
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