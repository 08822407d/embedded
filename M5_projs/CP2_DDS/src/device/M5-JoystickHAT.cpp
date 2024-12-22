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
#include "driver/ModuleManager.hpp"


M5JoystickHAT joystick;
// 使用 std::shared_ptr 包装现有的对象
std::shared_ptr<M5JoystickHAT> joystickPtr(&joystick, [](M5JoystickHAT*){});  // 禁止删除


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
	this->_wire->requestFrom(this->_addr, 3);
	if (this->_wire->available()) {
		this->_xVal		= (int8_t)this->_wire->read();
		this->_yVal		= (int8_t)this->_wire->read();
		this->_midBtn	= (int8_t)this->_wire->read();
	}
	// Serial.printf("X: %d, Y: %d\n", this->getX(), this->getY());
	return true;
}

void initM5JoystickHAT(void) {
	joystick.begin(&Wire, JOYSTICKHAT_ADDR,
		STICKC_GPIO_SDA, STICKC_GPIO_SCL, 100000UL);
	
	Serial.printf("_addr:0x%x , _sda:%d , _scl:%d\n",
		joystick._addr, joystick._sda, joystick._scl);
	
	DevModManager.registerModule(joystickPtr);
}