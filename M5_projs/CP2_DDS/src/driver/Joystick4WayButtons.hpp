#pragma once

#ifndef _JOYSTICK_4WAY_BUTTON_H_
#define _JOYSTICK_4WAY_BUTTON_H_

#include <AceButton.h>

#include "internal.h"
#include "device/M5-JoystickHAT.hpp"

using namespace ace_button;


	// 使用宏定义来避免魔法数字
	#define ACEBUTTON_POLL_INTERVAL_MS	10
	#define ACEBUTTON_TASK_PRIORITY		3


	// 1) 自定义一个 MyButtonConfig
	class Joystick4WayButtonConfig : public ButtonConfig {
	public:
		// 构造: 绑定一个Joystick指针 + 具体方向ID
		Joystick4WayButtonConfig(IJoystick* js, double ts, JoystickDirectionID dir)
			: ButtonConfig(), _joystick(js), _threshold(ts), _direction(dir)
		{
			assert(dir != JOY_DIR_CENTER);

			_tsX = ts * js->getXmax();
			_tsY = ts * js->getYmax();

			Serial.printf("xy Max: %d , %d ; Threshold: %d , %d\n", js->getXmax(), js->getYmax(), _tsX, _tsY);
		}

		// 重写 readButton
		// AceButton中在 button->check() 里会调用 _config->readButton(pin)
		// 我们不依赖pin, 而是使用joystick->isXxxPressed()
		virtual int readButton(uint8_t pin) override {
			int xVal = _joystick->getX();
			int yVal = _joystick->getY();

			// 如果已经是触发状态，且触发方向不是本Config负责的方向，就认为当前未触发
			if (_direction != _joystick->CurrDirection &&
				_joystick->CurrDirection != JOY_DIR_CENTER)
				return false;
				
			// 判断当前方向触发或者回正
			switch (_direction) {
			case JOY_DIR_UP:
				if ((abs(yVal) >= abs(xVal)) && (yVal >= _tsY)) {
					_joystick->CurrDirection = JOY_DIR_UP;
					return true;
				}
				break;
				// return (abs(yVal) >= abs(xVal)) && (yVal >= _tsY);
			case JOY_DIR_DOWN:
				if ((abs(yVal) >= abs(xVal)) && (yVal <= -_tsY)) {
					_joystick->CurrDirection = JOY_DIR_DOWN;
					return true;
				}
				break;
				// return (abs(yVal) >= abs(xVal)) && (yVal <= -_tsY);
			case JOY_DIR_LEFT:
				if ((abs(xVal) >= abs(yVal)) && (xVal <= -_tsX)) {
					_joystick->CurrDirection = JOY_DIR_LEFT;
					return true;
				}
				break;
				// return (abs(xVal) >= abs(yVal)) && (xVal <= -_tsX);
			case JOY_DIR_RIGHT:
				if ((abs(xVal) >= abs(yVal)) && (xVal >= _tsX)) {
					_joystick->CurrDirection = JOY_DIR_RIGHT;
					return true;
				}
				break;
				// return (abs(xVal) >= abs(yVal)) && (xVal >= _tsX);
			}
			_joystick->CurrDirection = JOY_DIR_CENTER;
			return false;
		}

		void debugPrintParams() {
			Serial.printf("xy Max: %d , %d ; Threshold: %d , %d\n", _joystick->getXmax(), _joystick->getYmax(), _tsX, _tsY);
		}

	private:
		IJoystick* _joystick;
		double _threshold;
		int _tsX;
		int _tsY;
		JoystickDirectionID _direction;
	};


#endif /* _JOYSTICK_4WAY_BUTTON_H_ */