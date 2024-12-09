#include "JoyStick.hpp"

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