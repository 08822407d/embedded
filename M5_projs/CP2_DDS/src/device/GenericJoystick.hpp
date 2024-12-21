#pragma once

#ifndef _GENERIC_JOYSTICK_MODULE_HPP_
#define _GENERIC_JOYSTICK_MODULE_HPP_


	class Joystick
	{
	private:
		/* data */
		int _xVal;
		int _yVal;

	public:
		int Xmax;
		int Ymax;

		// virtual Joystick(/* args */) {}
		// virtual ~Joystick() {}

		virtual int getX() = 0;
		virtual int getY() = 0;
		virtual bool update() = 0;
	};

#endif /* _GENERIC_JOYSTICK_MODULE_HPP_ */