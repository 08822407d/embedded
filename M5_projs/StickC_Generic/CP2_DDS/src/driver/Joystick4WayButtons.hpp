#pragma once

#ifndef _JOYSTICK_4WAY_BUTTON_H_
#define _JOYSTICK_4WAY_BUTTON_H_

#include <AceButton.h>

#include "internal.h"
#include "device/M5-JoystickHAT.hpp"

using namespace ace_button;


	// 使用宏定义来避免魔法数字
	#define ACEBUTTON_POLL_INTERVAL_MS	20
	#define ACEBUTTON_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)


	enum JoystickDirectionID {
		JOY_MID_BUTTON,
		JOY_DIR_UP,
		JOY_DIR_DOWN,
		JOY_DIR_LEFT,
		JOY_DIR_RIGHT,
		JOY_NONE,
	};


	// 1) 自定义一个 MyButtonConfig
	class Joystick4WayButtonConfig : public ButtonConfig {
	public:
		// 构造: 绑定一个Joystick指针 + 具体方向ID
		Joystick4WayButtonConfig(IJoystick* js, double ts)
			: ButtonConfig(), _joystick(js)
		{
			_direction = JOY_NONE;

			int Xmax = js->getXmax();
			int Ymax = js->getYmax();
			_threshold = (pow(Xmax, 2) + pow(Ymax, 2)) * pow(ts, 2);

			// Serial.printf("xy Max: %d , %d ; Threshold: %d\n", js->getXmax(), js->getYmax(), _threshold);
		}

		// 重写 readButton
		// AceButton中在 button->check() 里会调用 _config->readButton(pin)
		// 我们不依赖pin, 而是使用joystick->isXxxPressed()
		virtual int readButton(uint8_t pin) override {
			int xVal = _joystick->getX();
			int yVal = _joystick->getY();
			int midBtn = _joystick->getMid();
			int dist = pow(xVal, 2) + pow(yVal, 2);

			// Serial.printf("Threshold: %d ; Dist %d ; Direction: %d\n", _threshold, dist, _direction);
			// Serial.printf("Joy : %d , %d ; Mid: %d\n", xVal, yVal, midBtn);

			if ((dist > _threshold)) {
				if (_direction == JOY_NONE) {	// 摇杆重新触发四向按钮
					if (abs(xVal) > abs(yVal))
						_direction = (xVal > 0) ? JOY_DIR_RIGHT : JOY_DIR_LEFT;
					else
						_direction = (yVal > 0) ? JOY_DIR_UP : JOY_DIR_DOWN;
				}								// 或者已触发不再处理
				return true;
			} else if (midBtn > 0) {			// 中键按下
				_direction = JOY_MID_BUTTON;
				return true;
			} else {							// 摇杆回正(四向按钮松开)
				_direction = JOY_NONE;
				return false;
			}
		}

		JoystickDirectionID getCurrDirection() { return _direction; }

		void debugPrintParams() {
			Serial.printf("xy Max: %d , %d ; Threshold: %d\n", _joystick->getXmax(), _joystick->getYmax(), _threshold);
		}

	private:
		IJoystick* _joystick;
		int _threshold;
		JoystickDirectionID _direction;
	};


#endif /* _JOYSTICK_4WAY_BUTTON_H_ */