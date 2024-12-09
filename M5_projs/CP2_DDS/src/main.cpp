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

#include "modules/Number.hpp"


#define STICKC_SDA 32
#define STICKC_SCL 33


Unit_DDS dds;

int sawtooth_freq	= 13600;
int phase			= 0;
int freq			= 1000;
int modeIndex		= 0;

String modeName[] = {"Sine", "Square", "Triangle", "Sawtooth"};

const unsigned char *waveIcon[] = {sine, square, triangle, sawtooth};


void displayInfo() {
	int realfreq = freq;

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
			dds.quickOUT(Unit_DDS::kSINUSMode, freq, phase);
			break;
		case 1:
			dds.quickOUT(Unit_DDS::kSQUAREMode, freq, phase);
			break;
		case 2:
			dds.quickOUT(Unit_DDS::kTRIANGLEMode, freq, phase);
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

void setup() {
	M5.begin();
	Wire.begin(STICKC_SDA, STICKC_SCL);

	uiInit();

	dds.begin(&Wire);
}

void loop() {
	M5.update();

	if (M5.BtnA.wasPressed()) {
		freq += 500;
		changeWave(modeIndex);
	}
	if (M5.BtnB.wasPressed()) {
		modeIndex++;
		modeIndex %= 4;
		changeWave(modeIndex);
	}
}