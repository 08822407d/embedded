#pragma once

#ifndef JOYSTICK_4WAY_JUDGE_H
#define JOYSTICK_4WAY_JUDGE_H

#include "IEventJudge.hpp"
#include "internal.h"

#include "device/M5-JoystickHAT.hpp"


	enum FourWayDirection {
		DIR_UP,
		DIR_DOWN,
		DIR_LEFT,
		DIR_RIGHT
	};

	class Joystick4WayJudge : public IEventJudge {
	public:
		Joystick4WayJudge(const char* name, Joystick* mod)
			: judgeName(name), _joystick(mod)
		{
			// e.g. set default threshold
			threshold = 0.5;
		}

		void setThreshold(int t) { threshold = t; }

		bool checkEvent() override {
			_joystick->update();

			int x = _joystick->getX();
			int y = _joystick->getY();
			bool triggered = false;

			// if (abs(y) > threshold * _joystick->getY())
			// 	if (y > 0)

			return triggered;
		}

		const char* getName() const override {
			return judgeName;
		}

	private:
		const char*	judgeName;
		Joystick*	_joystick; // 指向摇杆模块
		double		threshold;
	};

#endif // JOYSTICK_4WAY_JUDGE_H