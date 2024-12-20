#include <M5Unified.h>
#include "M5-JoyStick.hpp"
#include "driver/Joystick.hpp"


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