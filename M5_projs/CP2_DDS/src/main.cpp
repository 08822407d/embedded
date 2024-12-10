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
#define JOYSTICK_TASK_STACK_SIZE 2048
#define JOYSTICK_TASK_PRIORITY   1

#define SERIAL_TASK_STACK_SIZE   2048
#define SERIAL_TASK_PRIORITY     1

#define JOYSTICK_SDA	0
#define JOYSTICK_SCL	26
#define DDS_SDA			32
#define DDS_SCL			33


// Default polling interval in milliseconds
volatile uint32_t joystickPollingInterval = 50; // 可通过其他方式修改

Unit_DDS dds;

Number<uint64_t> Freq(1000, 100, 10000000);

JoyStickData_s JoyStick;
extern std::shared_ptr<Joystick<int>> joystick;

int sawtooth_freq	= 13600;
int phase			= 0;
int modeIndex		= 0;

String modeName[] = {"Sine", "Square", "Triangle", "Sawtooth"};

const unsigned char *waveIcon[] = {sine, square, triangle, sawtooth};


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

void setup() {
	M5.begin();
	Serial.begin(115200);
	// Wire.begin(DDS_SDA, DDS_SCL);
	Wire.begin(JOYSTICK_SDA, JOYSTICK_SCL, 100000UL);
	initJoystick();

	uiInit();

	// dds.begin(&Wire);

	// 注册事件回调
	joystick->attachCallback([](const JoystickEvent& event) {
		if(event.source == EventSource::AXIS){
			switch(event.type){
				case EventType::SINGLE_CLICK:
					// Serial.printf("Axis Single Click Detected: %d\n", static_cast<int>(event.direction));
					M5.Lcd.clear();
					switch (static_cast<int>(event.direction))
					{
						case 1:
						M5.Lcd.fillTriangle(120, 45, 110, 55, 130, 55, TFT_GREEN);
							break;

						case 2:
						M5.Lcd.fillTriangle(120, 85, 110, 75, 130, 75, TFT_GREEN);
							break;

						case 3:
						M5.Lcd.fillTriangle(100, 65, 110, 55, 110, 75, TFT_GREEN);
							break;

						case 4:
						M5.Lcd.fillTriangle(140, 65, 130, 55, 130, 75, TFT_GREEN);
							break;
						
						default:
							break;
					}
					break;
				case EventType::MULTICLICK:
					// Serial.printf("Axis Multiclick Detected: %d\n", static_cast<int>(event.direction));
					break;
				case EventType::LONG_PRESS:
					// Serial.printf("Axis Long Press Detected: %d\n", static_cast<int>(event.direction));
					break;
			}
		}
		else if(event.source == EventSource::BUTTON){
			switch(event.type){
				case EventType::SINGLE_CLICK:
					Serial.printf("Button %d Single Click Detected\n", event.buttonId);
					break;
				case EventType::MULTICLICK:
					Serial.printf("Button %d Multiclick Detected\n", event.buttonId);
					break;
				case EventType::LONG_PRESS:
					Serial.printf("Button %d Long Press Detected\n", event.buttonId);
					break;
			}
		}
	});

	// 创建FreeRTOS任务
	xTaskCreate(
		joystickTask,               // Task function
		"Joystick Task",            // Task name
		JOYSTICK_TASK_STACK_SIZE,   // Stack size
		NULL,                       // Task input parameter
		JOYSTICK_TASK_PRIORITY,     // Priority
		NULL                        // Task handle
	);

	// 创建Serial任务
	xTaskCreate(
		serialTask,                 // Task function
		"Serial Task",              // Task name
		SERIAL_TASK_STACK_SIZE,     // Stack size
		NULL,                       // Task input parameter
		SERIAL_TASK_PRIORITY,       // Priority
		NULL                        // Task handle
	);
}

void loop() {
	// 主循环可以保持空闲，所有功能由FreeRTOS任务处理
	// 或者在此添加其他非关键任务
	delay(1000); // 防止主循环占用过多CPU
}

// void loop() {
// 	M5.update();

// 	if (M5.BtnA.wasPressed()) {
// 		Freq += 100;
// 		changeWave(modeIndex);
// 	}
// 	if (M5.BtnB.wasPressed()) {
// 		modeIndex++;
// 		modeIndex %= 4;
// 		changeWave(modeIndex);
// 	}
// }


// 任务实现
void joystickTask(void * parameter) {
	(void) parameter; // 避免未使用参数的编译警告

	while (1) {
		// 记录任务开始时间
		TickType_t taskStart = xTaskGetTickCount();


		joystick->update(taskStart);
		// 读取摇杆数据
		// readJoyStick(&Wire, &JoyStick);
		// Serial.printf("Joystick ( x:%d , y:%d )\n", JoyStick.x, JoyStick.y);


		// 计算任务执行时间
		TickType_t taskEnd = xTaskGetTickCount();
		TickType_t elapsedTime = taskEnd - taskStart;
		// 计算剩余延迟时间
		uint32_t delayTime =
				(joystickPollingInterval > (elapsedTime * portTICK_PERIOD_MS)) ? 
					joystickPollingInterval - (elapsedTime * portTICK_PERIOD_MS) : 0;
		// 延迟指定时间后再次执行
		vTaskDelay(pdMS_TO_TICKS(delayTime));
	}
}

// Serial任务实现
void serialTask(void * parameter) {
	(void) parameter;

	while (1) {
		// if (Serial.available() > 0) {
		// 	String input = Serial.readStringUntil('\n');
		// 	input.trim(); // 去除换行符和空格

		// 	// 期望输入格式为: "interval 100"
		// 	if (input.startsWith("interval")) {
		// 		int newInterval = input.substring(9).toInt();
		// 		if (newInterval > 0) {
		// 			joystickPollingInterval = newInterval;
		// 			Serial.printf("Polling interval updated to %d ms\n", joystickPollingInterval);
		// 		} else {
		// 			Serial.println("Invalid interval value.");
		// 		}
		// 	} else {
		// 		Serial.println("Unknown command.");
		// 	}
		// }

		Serial.printf("MultiThread test\n");

		vTaskDelay(pdMS_TO_TICKS(1000)); // 每100ms检查一次
	}
}