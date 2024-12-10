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
    // 阈值百分比为20%，长按时间为1000ms，多击间隔为500ms
    joystick = std::make_shared<Joystick<int>>(joystickReader, 40, 750, 500);
}

void readJoyStick(TwoWire *wire, JoyStickData_s *data)
{
	wire->beginTransmission(JOY_ADDR);
	wire->write(0x02);
	wire->endTransmission();
	wire->requestFrom(JOY_ADDR, 3);
	if (Wire.available()) {
		data->x			= wire->read();
		data->y			= wire->read();
		data->center	= wire->read();
	}
}