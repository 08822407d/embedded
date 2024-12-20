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
#include <M5Unified.h>
#include "Unit_DDS.h"
#include "WaveIcon.c"

#include "include.h"



// Task Parameters
#define TASK_STACK_SIZE	2048

#define JOYSTICK_SDA	0
#define JOYSTICK_SCL	26
#define DDS_SDA			32
#define DDS_SCL			33



Unit_DDS dds;

Number<uint64_t> Freq(1000, 100, 10000000);


int sawtooth_freq	= 13600;
int phase			= 0;
int modeIndex		= 0;

String modeName[] = {"Sine", "Square", "Triangle", "Sawtooth"};

// const unsigned char *waveIcon[] = {sine, square, triangle, sawtooth};



void displayInfo() {
	int realfreq = Freq.getValue();

	M5.Lcd.clear();

	// M5.Lcd.drawJpg(waveIcon[modeIndex], sizeof(waveIcon[modeIndex]));

	M5.Lcd.setTextColor(TFT_GREEN);
	M5.Lcd.drawString("Wave:  " + modeName[modeIndex], 10, 110);

	// M5.Lcd.setTextColor(TFT_BLUE);
	// M5.Lcd.drawString("Phase:  " + String(phase), 10, 110);

	if (modeIndex == 3)
		realfreq = sawtooth_freq;
	M5.Lcd.setTextColor(TFT_YELLOW);
	M5.Lcd.drawString("Freq:  " + String(realfreq), 120, 110);
}

void changeWave(int expression) {
	switch (expression) {
		case 0:
			dds.quickOUT(Unit_DDS::kSINUSMode, Freq.getValue(), phase);
			break;
		case 1:
			dds.quickOUT(Unit_DDS::kSQUAREMode, Freq.getValue(), phase);
			break;
		case 2:
			dds.quickOUT(Unit_DDS::kTRIANGLEMode, Freq.getValue(), phase);
			break;
		case 3:
			// SAWTOOTH WAVE Only support 13.6Khz
			dds.quickOUT(Unit_DDS::kSAWTOOTHMode, sawtooth_freq, phase);
			break;
		default:
			break;
	}
	displayInfo();
}

void uiInit() {
	M5.Lcd.setRotation(3);
	M5.Lcd.fillScreen(BLACK);
	M5.Lcd.fillTriangle(110, 50, 110, 80, 130, 65, TFT_GREEN);
	M5.Lcd.setTextFont(2);
}

void drvInit() {

}

void setup() {
	M5.begin();
	Serial.begin(115200);
	Wire.begin(JOYSTICK_SDA, JOYSTICK_SCL, 400000UL);
	
	uiInit();
	// dds.begin(&Wire);

	initM5JoystickHAT();

    // 创建judge: 2) 对joystick的4向
    auto joy4Way = std::make_shared<Joystick4WayJudge>("4Way", &joystick);
    eventMgr.registerJudge(joy4Way);

	initEventPollTask();
}

void loop() {
	// 主循环可以保持空闲，所有功能由FreeRTOS任务处理
	// 或者在此添加其他非关键任务
	Serial.printf("X: %d, Y: %d\n", joystick.getX(), joystick.getY());
	delay(50); // 防止主循环占用过多CPU
}