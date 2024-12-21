// void readJoyStick(TwoWire *wire, JoyStickData_s *data)
// {
// 	wire->beginTransmission(JOY_ADDR);
// 	wire->write(0x02);
// 	wire->endTransmission();
// 	wire->requestFrom(JOY_ADDR, 3);
// 	if (Wire.available()) {
// 		data->x			= wire->read();
// 		data->y			= wire->read();
// 		data->center	= wire->read();
// 	}
// }

// void readJoyStick_16bit(TwoWire *wire, JoyStickData_s *data)
// {
// 	wire->beginTransmission(JOY_ADDR);
// 	wire->write(0x01);
// 	wire->endTransmission();
// 	wire->requestFrom(JOY_ADDR, 4);
// 	if (Wire.available()) {
// 		data->x			= wire->read();
// 		data->x			|= wire->read() << 8;
// 		data->y			= wire->read();
// 		data->y			|= wire->read() << 8;
// 	}
// }


#include <Arduino.h> // 或 esp-idf 头
#include "M5-JoystickHAT.hpp"


M5JoystickHAT joystick;


M5JoystickHAT::M5JoystickHAT()
	: _addr(0x00), _xVal(0), _yVal(0), lastUpdateMs(0)
{
}

bool M5JoystickHAT::begin(TwoWire *wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed)
{
    _wire  = wire;
    _addr  = addr;
    _sda   = sda;
    _scl   = scl;
    _speed = speed;
    _wire->begin(_sda, _scl);
    _wire->setClock(speed);
    delay(10);
    _wire->beginTransmission(_addr);
    uint8_t error = _wire->endTransmission();
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

bool M5JoystickHAT::update() {
	// // 假设需要读取 6 字节: Xlow, Xhigh, Ylow, Yhigh, midBtn, topBtn
	// Wire.beginTransmission(address);
	// Wire.write(0x00);
	// if (Wire.endTransmission(false) != 0) {
	// 	return false;
	// }
	// Wire.requestFrom(address, (uint8_t)6);
	// if (Wire.available() < 6) {
	// 	return false;
	// }
	// uint8_t xl = Wire.read();
	// uint8_t xh = Wire.read();
	// uint8_t yl = Wire.read();
	// uint8_t yh = Wire.read();
	// uint8_t mb = Wire.read();
	// uint8_t tb = Wire.read();

	// xVal = (xh << 8) | xl;
	// yVal = (yh << 8) | yl;
	// midBtnPressed = (mb != 0);

	Wire.beginTransmission(this->_addr);
	Wire.write(0x02);
	Wire.endTransmission();
	Wire.requestFrom(this->_addr, 3);
	if (Wire.available()) {
		this->_xVal		= (int8_t)Wire.read();
		this->_yVal		= (int8_t)Wire.read();
		this->_midBtn	= (int8_t)Wire.read();
	}

	return true;
}

void initM5JoystickHAT(void) {
	joystick.begin();
}