#pragma once

#ifndef _M5_JOYSTICKHAT_MODULE_HPP_
#define _M5_JOYSTICKHAT_MODULE_HPP_

#include <Wire.h>
#include <functional>
#include <map>

#include "GenericJoystick.hpp"


#define JOYSTICKHAT_ADDR	0x38
#define CENTER_XY_VAL		0x00
#define MAX_XY_VAL			0x7F

#define STICKC_GPIO_SDA		0
#define STICKC_GPIO_SCL		26

	// // 事件枚举
	// enum M5JoystickHATEventType {
	// 	EVT_BTN_MIDDLE_CLICK,
	// 	EVT_BTN_MIDDLE_DOUBLECLICK,
	// 	EVT_BTN_MIDDLE_LONGPRESS,
	// 	// EVT_BTN_TOP_CLICK,
	// 	// EVT_BTN_TOP_DOUBLECLICK,
	// 	// EVT_BTN_TOP_LONGPRESS,
	// 	EVT_DIR_UP_CLICK,
	// 	EVT_DIR_UP_DOUBLECLICK,
	// 	EVT_DIR_UP_LONGPRESS,
	// 	EVT_DIR_DOWN_CLICK,
	// 	EVT_DIR_DOWN_DOUBLECLICK,
	// 	EVT_DIR_DOWN_LONGPRESS,
	// 	EVT_DIR_LEFT_CLICK,
	// 	EVT_DIR_LEFT_DOUBLECLICK,
	// 	EVT_DIR_LEFT_LONGPRESS,
	// 	EVT_DIR_RIGHT_CLICK,
	// 	EVT_DIR_RIGHT_DOUBLECLICK,
	// 	EVT_DIR_RIGHT_LONGPRESS,
	// 	// ... etc
	// };


	class M5JoystickHAT : public Joystick {
	public:
		uint8_t		_addr;
		TwoWire		*_wire;
		uint8_t		_scl;
		uint8_t		_sda;
		uint32_t	_speed;

		int			Xmax = 127;
		int			Ymax = 127;


		M5JoystickHAT();
		virtual ~M5JoystickHAT() {}

		/**
		 * @brief Unit Joystick2 init
		 * @param wire I2C Wire number
		 * @param addr I2C address
		 * @param sda I2C SDA Pin
		 * @param scl I2C SCL Pin
		 * @param speed I2C clock
		 * @return 1 success, 0 false
		 */
		bool begin(TwoWire *wire = &Wire, uint8_t addr = JOYSTICKHAT_ADDR,
				uint8_t sda = STICKC_GPIO_SDA, uint8_t scl = STICKC_GPIO_SCL,
				uint32_t speed = 400000UL);

		bool update() override;
		int getX() override { return _xVal; }
		int getY() override { return _yVal; }

		// 可选: 提供 setLongPressTime, setDoubleClickInterval, setDirectionThreshold 等

	private:
		// 存储硬件读到的数据
		int _xVal, _yVal, _midBtn;

		// 事件判定相关
		unsigned long lastUpdateMs;
	};


	extern M5JoystickHAT joystick;
	void initM5JoystickHAT(void);

#endif /* _M5_JOYSTICKHAT_MODULE_HPP_ */