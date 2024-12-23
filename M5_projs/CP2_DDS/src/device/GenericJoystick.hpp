#pragma once

#ifndef _GENERIC_JOYSTICK_MODULE_HPP_
#define _GENERIC_JOYSTICK_MODULE_HPP_

#include "GenericModule.hpp"


	enum JoystickDirectionID {
		JOY_DIR_CENTER,
		JOY_DIR_UP,
		JOY_DIR_DOWN,
		JOY_DIR_LEFT,
		JOY_DIR_RIGHT
	};

	class IJoystick : public IDeviceModule {
	private:
		/* data */
		int _xVal;
		int _yVal;
		int _Xmax;
		int _Ymax;

	public:
		JoystickDirectionID CurrDirection = JOY_DIR_CENTER;

		virtual int getX() = 0;
		virtual int getY() = 0;
		virtual int getXmax() = 0;
		virtual int getYmax() = 0;
		virtual bool update() = 0;

		/**
		 * @brief 返回模块名称或标识(可用于调试或日志输出)。
		 */
		virtual const char* getName() const = 0;
	};

#endif /* _GENERIC_JOYSTICK_MODULE_HPP_ */