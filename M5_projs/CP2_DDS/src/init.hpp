#pragma once

#include <Arduino.h>
#include <Wire.h>

#include <M5Unified.h>

#include "conf.h"

#include "Unit_DDS.h"
#include "WaveIcon.c"
#include "glob.hpp"

#include "modules/Number.hpp"
#include "device/M5-JoystickHAT.hpp"
#include "device/M5Unit-DDS.hpp"
#include "driver/HWModuleManager.hpp"
#include "driver/Joystick4WayButtons.hpp"
#include "GUI/display.hpp"



extern std::shared_ptr<M5JoystickHAT> joystickPtr;
extern std::shared_ptr<AceButton> Joystick4WayButton;
extern std::shared_ptr<M5UnitDDS> ddsPtr;


void initM5UnitDDS(void);
void initM5JoystickHAT(void);

void initJoystick4WayButtonsCheckTask(void);
void initJoystick4WayButtons(void);