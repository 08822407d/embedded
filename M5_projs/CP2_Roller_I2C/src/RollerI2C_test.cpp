#include "unit_rolleri2c.hpp"
#include <M5Unified.h>


UnitRollerI2C RollerI2C;  // Create a UNIT_ROLLERI2C object
uint32_t p, i, d;         // Defines a variable to store the PID value
uint8_t r, g, b;


void setup()
{
	auto cfg = M5.config();
	M5.begin(cfg);
	Serial.begin(115200);
	RollerI2C.begin(&Wire, 0x64, 32, 33, 400000);


	RollerI2C.setOutput(0);
	RollerI2C.setMode(ROLLER_MODE_SPEED);
	RollerI2C.setOutput(1);
}

void loop()
{
	auto imu_update = M5.Imu.update();
	if (imu_update) {
		M5.Lcd.setCursor(0, 40);
		M5.Lcd.clear();  // Delay 100ms 延迟100ms

		auto data = M5.Imu.getImuData();

		// The data obtained by getImuData can be used as follows.
		data.accel.x;      // accel x-axis value.
		data.accel.y;      // accel y-axis value.
		data.accel.z;      // accel z-axis value.
		data.accel.value;  // accel 3values array [0]=x / [1]=y / [2]=z.

		data.gyro.x;      // gyro x-axis value.
		data.gyro.y;      // gyro y-axis value.
		data.gyro.z;      // gyro z-axis value.
		data.gyro.value;  // gyro 3values array [0]=x / [1]=y / [2]=z.

		data.value;  // all sensor 9values array [0~2]=accel / [3~5]=gyro /
					 // [6~8]=mag

		Serial.printf("ax:%f  ay:%f  az:%f\r\n", data.accel.x, data.accel.y,
					  data.accel.z);
		Serial.printf("gx:%f  gy:%f  gz:%f\r\n", data.gyro.x, data.gyro.y,
					  data.gyro.z);

		M5.Lcd.printf("IMU:\r\n");
		M5.Lcd.printf("%0.2f %0.2f %0.2f\r\n", data.accel.x, data.accel.y,
					  data.accel.z);
		M5.Lcd.printf("%0.2f %0.2f %0.2f\r\n", data.gyro.x, data.gyro.y,
					  data.gyro.z);
		M5.Lcd.printf("%0.2f %0.2f %0.2f\r\n", data.mag.x, data.mag.y,
					  data.mag.z);
		

		RollerI2C.setOutput(0);
		RollerI2C.setSpeed(240000 * data.accel.x);
		RollerI2C.setOutput(1);
	}
	delay(10);


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