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


M5JoystickHAT joystick(JOY_ADDR);


M5JoystickHAT::M5JoystickHAT(uint8_t i2cAddr)
	: address(i2cAddr), xVal(0), yVal(0),
	  midBtnPressed(false),
	  lastUpdateMs(0)
{
}

bool M5JoystickHAT::update() {
	unsigned long now = millis(); // or xTaskGetTickCount() etc

	if (!readFromHardware()) {
		return false;
	}

	handleButtons(now);
	handleDirections(now);

	lastUpdateMs = now;
	return true;
}

void M5JoystickHAT::attachCallback(M5JoystickHATEventType eventType, M5JoystickHATCallback cb) {
	callbacks[eventType] = cb;
}

bool M5JoystickHAT::readFromHardware() {
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

	Wire.beginTransmission(this->address);
	Wire.write(0x02);
	Wire.endTransmission();
	Wire.requestFrom(this->address, 3);
	if (Wire.available()) {
		this->xVal		= (int8_t)Wire.read();
		this->yVal		= (int8_t)Wire.read();
		this->midBtn	= (int8_t)Wire.read();
	}

	return true;
}

void M5JoystickHAT::handleButtons(unsigned long now) {
	// 这里可对 midBtnPressed, topBtnPressed 做防抖、多击、长按等判定
	// 示例：若 midBtnPressed 刚从false->true, 记下pressStartTime
	// 若从true->false, 计算时间决定click/doubleClick
	// 若按住超过longPress阈值 -> triggerEvent(EVT_BTN_MIDDLE_LONGPRESS)

	// 这里只是示例性的一个触发
	static bool prevMid = false;
	if (midBtnPressed && !prevMid) {
		// 按下
		triggerEvent(EVT_BTN_MIDDLE_CLICK);
	}
	prevMid = midBtnPressed;

	// // 同理topBtn
	// static bool prevTop = false;
	// if (topBtnPressed && !prevTop) {
	// 	triggerEvent(EVT_BTN_TOP_CLICK);
	// }
	// prevTop = topBtnPressed;
}

void M5JoystickHAT::handleDirections(unsigned long now) {
	// 例：判断上、下、左、右4向
	// 同样可做防抖、多击、长按 => 这里仅示例简单触发
	if (isDirectionPressed(DIR_UP)) {
		// do something or triggerEvent(EVT_DIR_UP_CLICK)...
	}
	// 同理 DIR_DOWN, DIR_LEFT, DIR_RIGHT ...
}

bool M5JoystickHAT::isDirectionPressed(JoyDirection dir) {
	// 简单阈值判断, 例: x,y in 0~65535 => center=32767 => threshold=10000
	int center = 32767;
	int threshold = 10000;
	int dx = xVal - center;
	int dy = yVal - center;

	switch (dir) {
		case DIR_UP:    return (dy > threshold);
		case DIR_DOWN:  return (dy < -threshold);
		case DIR_LEFT:  return (dx < -threshold);
		case DIR_RIGHT: return (dx > threshold);
		default: return false;
	}
}

void M5JoystickHAT::triggerEvent(M5JoystickHATEventType et) {
	auto it = callbacks.find(et);
	if (it != callbacks.end()) {
		// call user callback
		if (it->second) {
			it->second();
		}
	}
}

void initM5JoystickHAT(void) {
	// Wire.begin(JOYSTICK_SDA, JOYSTICK_SCL, 100000UL);
}