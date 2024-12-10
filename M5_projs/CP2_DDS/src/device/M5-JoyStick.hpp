#pragma once

#include <Wire.h>


#define JOY_ADDR	0x38


// Joystick数据结构
typedef struct JoystickData {
	int16_t	x;
	int16_t	y;
	int16_t	center;
} JoyStickData_s;


// extern Joystick joystick;

extern void readJoyStick(TwoWire *wire, JoyStickData_s *data);
extern void initJoystick(void);