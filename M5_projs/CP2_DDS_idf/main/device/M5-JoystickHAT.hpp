#pragma once

#ifndef _M5_JOYSTICKHAT_MODULE_HPP_
#define _M5_JOYSTICKHAT_MODULE_HPP_

#include <memory>
#include <functional>
#include <map>

#include <Wire.h>

#include "GenericJoystick.hpp"


#define JOYSTICKHAT_ADDR	0x38
#define CENTER_XY_VAL		0x00
#define MAX_XY_VAL			0x7F

#define STICKC_GPIO_SDA		0
#define STICKC_GPIO_SCL		26


	class M5JoystickHAT : public IJoystick {
	public:
		uint8_t		_addr;
		TwoWire		*_wire;
		uint8_t		_scl;
		uint8_t		_sda;
		uint32_t	_speed;


		M5JoystickHAT();
		// virtual ~M5JoystickHAT() {}

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

		int getX() override { return _xVal; }
		int getY() override { return _yVal; }
		int getXmax() override { return _Xmax; }
		int getYmax() override { return _Ymax; }

		bool update() override;

		void setRotation(uint8_t rotation);

		const char* getName() const override {
			return "M5JoystickHAT";
		}
		// 可选: 提供 setLongPressTime, setDoubleClickInterval, setDirectionThreshold 等

		void debugPrintParams() {
			Serial.printf("Joystick addr: 0x%x, XY Max: %d , %d\n", this, _Xmax, _Ymax);
		}

	private:
		uint8_t _rotation;
		// 存储硬件读到的数据
		int _xVal, _yVal, _midBtn;
		int _Xmax, _Ymax;

		// 事件判定相关
		unsigned long lastUpdateMs;
	};

#endif /* _M5_JOYSTICKHAT_MODULE_HPP_ */