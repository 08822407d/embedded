#include <M5Unified.h>
#include "M5-JoyStick.hpp"
#include "driver/Joystick.hpp"

// 定义全局Joystick实例
std::shared_ptr<Joystick<int>> joystick;

void initJoystick()
{
	// 创建JoystickReader实例
	// 例如，使用I2C读取，假设有3个按键
	std::shared_ptr<JoystickReader<int>> joystickReader = std::make_shared<M5JoystickReader<int>>(JOY_ADDR, 0x02, 1);

	// 创建Joystick实例
	joystick = std::make_shared<Joystick<int>>(joystickReader, 40, 750, 500);

	// 禁用防抖
	joystick->enableDebounce(false);


	Wire.beginTransmission(JOY_ADDR);  // 开始与设备通信
	Wire.write(0x03);  // 写入寄存器地址
	Wire.write(0x02);  // 写入要写入的数据
	Wire.endTransmission();  // 结束传输

	// delay(3000);

	// M5.Lcd.clear();
	// M5.Lcd.drawString("Start Calibration...", 65, 10);
	// delay(5000);

	// M5.Lcd.clear();
	// M5.Lcd.drawString("Finishing Calibration...", 65, 10);
	// delay(1000);

	// Wire.beginTransmission(JOY_ADDR);  // 开始与设备通信
	// Wire.write(0x03);  // 写入寄存器地址
	// Wire.write(0x03);  // 写入要写入的数据
	// Wire.endTransmission();  // 结束传输

	// M5.Lcd.clear();
	// M5.Lcd.drawString("Finished", 65, 10);
	// delay(500);
}

void readJoyStick(TwoWire *wire, JoyStickData_s *data)
{
	wire->beginTransmission(JOY_ADDR);
	wire->write(0x02);
	wire->endTransmission(false);
	wire->requestFrom(JOY_ADDR, 3);
	if (Wire.available()) {
		data->x			= wire->read();
		data->y			= wire->read();
		data->center	= wire->read();
	}
	// wire->endTransmission();
}

void readJoyStick_16bit(TwoWire *wire, JoyStickData_s *data)
{
	wire->beginTransmission(JOY_ADDR);
	wire->write(0x01);
	wire->endTransmission();
	wire->requestFrom(JOY_ADDR, 4);
	if (Wire.available()) {
		data->x			= wire->read();
		data->x			|= wire->read() << 8;
		data->y			= wire->read();
		data->y			|= wire->read() << 8;
	}
}