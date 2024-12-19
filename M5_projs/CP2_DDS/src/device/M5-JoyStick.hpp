#pragma once

#include <Wire.h>


#define JOY_ADDR	0x38


// Joystick数据结构
typedef struct JoystickData {
	int8_t	x;
	int8_t	y;
	int8_t	center;
} JoyStickData_s;


// extern Joystick joystick;

extern void readJoyStick(TwoWire *wire, JoyStickData_s *data);
extern void readJoyStick_16bit(TwoWire *wire, JoyStickData_s *data);
extern void initJoystick(void);