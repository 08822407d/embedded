#include <Arduino.h>
#include <Wire.h>
#include <memory>
#include "driver/Joystick.hpp"
#include "driver/JoystickReader.hpp"
#include "driver/ButtonReader.hpp"
#include "driver/ButtonHandler.hpp"
#include "driver/JoystickToButton.hpp"


    extern void joystickTask(void * parameter);
    extern void serialTask(void * parameter);