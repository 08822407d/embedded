/*
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with M5Core sample source code
*                          配套  M5Core 示例源代码
* Visit more information: https://docs.m5stack.com/en/unit/dds
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/dds
*
* Product: Unit DDS.
* Date: 2022/7/8
*******************************************************************************
*/
#define LGFX_AUTODETECT

#include "init.hpp"


// 定义全局单例实例引用
DDSparams& globalDDSparams = DDSparams::getInstance();

String modeName[] = {"Sine", "Square", "Triangle", "Sawtooth"};


void setup() {
	M5.begin();
	Serial.begin(115200);
	M5.Display.setRotation(GLOBAL_ROTATION);

	Wire.end();
	Wire1.end();
	Wire.begin(0, 26, 400000UL);
	Wire1.begin(32, 33, 400000UL);


	initLvglDisplay();
	delay(500);
	initScreenPages();
	delay(500);

	initM5UnitDDS();
	delay(500);

	initM5JoystickHAT();
	delay(500);

	initJoystick4WayButtons();
	delay(500);

	initJoystick4WayButtonsCheckTask();
	initHardWarePollTask();

	globalDDSparams.Frequency = 100;
}

void loop() {
	// 主循环可以保持空闲，所有功能由FreeRTOS任务处理
	// 或者在此添加其他非关键任务

	lv_timer_handler();		/* let the GUI do its work */

	delay(50); // 防止主循环占用过多CPU
}