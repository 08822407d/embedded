#pragma once

#include <Arduino.h>
#include <Wire.h>

#include <M5Unified.h>

#include "conf.h"

#include "Unit_DDS.h"
#include "WaveIcon.c"

#include "modules/Number.hpp"
#include "device/M5-JoystickHAT.hpp"
#include "driver/HWModuleManager.hpp"
#include "driver/Joystick4WayButtons.hpp"
#include "GUI/display.hpp"



// 声明一个全局的 std::shared_ptr
void initM5JoystickHAT(void);


void initJoystick4WayButtonsCheckTask(void);
void initJoystick4WayButtons(void);