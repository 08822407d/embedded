#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "device/M5-JoystickHAT.hpp"
#include "driver/ModuleManager.hpp"
#include "driver/Joystick4WayButtons.hpp"


// 声明一个全局的 std::shared_ptr
extern M5JoystickHAT joystick;
void initM5JoystickHAT(void);


void initJoystick4WayButtonsCheckTask(void);
void initJoystick4WayButtons(void);