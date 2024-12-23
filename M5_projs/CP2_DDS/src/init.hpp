#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "modules/Number.hpp"
#include "device/M5-JoystickHAT.hpp"
#include "driver/ModuleManager.hpp"
#include "driver/Joystick4WayButtons.hpp"


#define GLOBAL_ROTATION				3
#define JOYSTICK_TRIGGER_THRESAHOLD	0.3


// 声明一个全局的 std::shared_ptr
void initM5JoystickHAT(void);


void initJoystick4WayButtonsCheckTask(void);
void initJoystick4WayButtons(void);