#pragma once

#ifndef _JOYSTICK_4WAY_BUTTON_H_
#define _JOYSTICK_4WAY_BUTTON_H_

#include <AceButton.h>

#include "internal.h"
#include "device/M5-JoystickHAT.hpp"

using namespace ace_button;


	// 使用宏定义来避免魔法数字
	#define ACEBUTTON_POLL_INTERVAL_MS	20
	#define ACEBUTTON_TASK_PRIORITY		3


	enum JoystickDirectionID {
	JOY_DIR_UP,
	JOY_DIR_DOWN,
	JOY_DIR_LEFT,
	JOY_DIR_RIGHT
	};

	// 1) 自定义一个 MyButtonConfig
	class Joystick4WayButtonConfig : public ButtonConfig {
	public:
		// 构造: 绑定一个Joystick指针 + 具体方向ID
		Joystick4WayButtonConfig(IJoystick* js, double ts, JoystickDirectionID dir)
			: ButtonConfig(), _joystick(js), _threshold(ts), _direction(dir)
		{
			// activeLow 表示AceButton是否当"true=按下" or "false=按下"
			// 后续可 setFeature(ButtonConfig::kFeatureClick) 等
			// setActiveLow(activeLow);
		}

		// 重写 readButton
		// AceButton中在 button->check() 里会调用 _config->readButton(pin)
		// 我们不依赖pin, 而是使用joystick->isXxxPressed()
		virtual int readButton(uint8_t pin) override {
			int xVal = _joystick->getX();
			int yVal = _joystick->getY();
			int xMax = _joystick->Xmax;
			int yMax = _joystick->Ymax;

			// 注意: pin参数无用, 因为是虚拟引脚
			switch (_direction) {
			case JOY_DIR_UP:
				return (abs(yVal) > abs(xVal)) && (yVal >= yMax * _threshold);
			case JOY_DIR_DOWN:
				return (abs(yVal) > abs(xVal)) && (yVal <= -yMax * _threshold);
			case JOY_DIR_LEFT:
				return (abs(xVal) > abs(yVal)) && (xVal <= -xMax * _threshold);
			case JOY_DIR_RIGHT:
				return (abs(xVal) > abs(yVal)) && (xVal >= xMax * _threshold);
			}
			return false;
		}

	private:
		IJoystick* _joystick;
		double _threshold;
		JoystickDirectionID _direction;
	};


	
	void initJoystick4WayButtons(void);

#endif /* _JOYSTICK_4WAY_BUTTON_H_ */