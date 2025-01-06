#include <Arduino.h> // 或 esp-idf 头
#include "M5-JoystickHAT.hpp"
#include "driver/HWModuleManager.hpp"


M5JoystickHAT::M5JoystickHAT()
	:	_addr(0x00), _xVal(0), _yVal(0),
		_Xmax(MAX_XY_VAL), _Ymax(MAX_XY_VAL),
		_rotation(0), lastUpdateMs(0)
{
}

bool M5JoystickHAT::begin(TwoWire *wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed)
{
	_wire  = wire;
	_addr  = addr;
	_sda   = sda;
	_scl   = scl;
	_speed = speed;
	_wire->begin(_sda, _scl, speed);
	_wire->setClock(speed);
	delay(10);
	_wire->beginTransmission(_addr);
	uint8_t error = _wire->endTransmission();
	if (error == 0) {
		return true;
	} else {
		Serial.printf("M5-JoystickHAT I2C begin error: %d\n", error);
		delay(1000);
		return false;
	}
}

bool M5JoystickHAT::update() {
	this->_wire->beginTransmission(this->_addr);
	this->_wire->write(0x02);
	this->_wire->endTransmission();
	this->_wire->requestFrom(this->_addr, (uint8_t)3);
	if (this->_wire->available()) {
		int _rawX		= (int8_t)this->_wire->read();
		int _rawY		= (int8_t)this->_wire->read();
		this->_midBtn	= !(int8_t)this->_wire->read();

		switch (this->_rotation) {
			case 0:
				this->_xVal	= -_rawX;
				this->_yVal	= -_rawY;
				break;
			case 1:
				this->_xVal	= -_rawY;
				this->_yVal	= _rawX;
				break;
			case 2:
				this->_xVal	= _rawX;
				this->_yVal	= _rawY;
				break;
			case 3:
				this->_xVal	= _rawY;
				this->_yVal	= -_rawX;
				break;
			default:
				this->_xVal	= -_rawX;
				this->_yVal	= -_rawY;
				break;
		}
	}
	// Serial.printf("X: %d, Y: %d\n", this->getX(), this->getY());
	return true;
}

void M5JoystickHAT::setRotation(uint8_t rotation) {
	this->_rotation = rotation;
	if (this->_rotation == 1 || this->_rotation == 3) {
		int swap = this->_Xmax;
		this->_Xmax = this->_Ymax;
		this->_Ymax = swap;
	}
}