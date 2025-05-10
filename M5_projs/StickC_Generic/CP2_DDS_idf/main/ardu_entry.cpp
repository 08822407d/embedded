#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "ArduUtils.hpp"
#include "Wire.hpp"
#include "M5-JoystickHAT.hpp"

#include "conf.h"


// 全局智能指针定义并初始化为nullptr
std::shared_ptr<M5JoystickHAT> joystickPtr = nullptr;

void initM5JoystickHAT(void) {
	// 初始化摇杆
    joystickPtr = std::make_shared<M5JoystickHAT>();
	M5JoystickHAT *joystick = joystickPtr.get();

	joystick->begin(&Wire, JOYSTICKHAT_ADDR,
		STICKC_GPIO_SDA, STICKC_GPIO_SCL, 100000UL);
	joystick->setRotation(GLOBAL_ROTATION);

	// joystick->debugPrintParams();
	printf("Joystick XY Max: %d , %d\n", joystickPtr->getXmax(), joystickPtr->getYmax());
}


// 定义 setup() 和 loop() 函数
extern "C" void setup() {
	// 初始化代码
	printf("Setup function called\n");

	// Wire.begin();

	initM5JoystickHAT();
}

extern "C" void loop() {
	// 循环代码
	joystickPtr->update();
	printf("Joystick XY: %d , %d\n", joystickPtr->getX(), joystickPtr->getY());

	delay(100);
}

