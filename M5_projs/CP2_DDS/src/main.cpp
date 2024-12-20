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


// 定义全局Joystick、ButtonHandler和JoystickToButton实例
std::shared_ptr<Joystick> joystick;
std::shared_ptr<ButtonHandler> buttonHandler;
std::shared_ptr<JoystickToButton> joystickToButton;

// 定义轮询间隔
volatile unsigned long joystickPollingInterval = JOYSTICK_DEBOUNCE_TIME_MS;
volatile unsigned long buttonPollingInterval = JOYSTICK_DEBOUNCE_TIME_MS;
volatile unsigned long virtualButtonPollingInterval = JOYSTICK_DEBOUNCE_TIME_MS;

// 函数原型
void joystickTask(void * parameter);
void buttonTask(void * parameter);
void joystickToButtonTask(void * parameter);
void serialTask(void * parameter);



// Task Parameters
#define JOYSTICK_TASK_STACK_SIZE 2048
#define JOYSTICK_TASK_PRIORITY   1

#define SERIAL_TASK_STACK_SIZE   2048
#define SERIAL_TASK_PRIORITY     1

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

void setup() {
	M5.begin();
	Serial.begin(115200);
	Wire.begin(JOYSTICK_SDA, JOYSTICK_SCL, 100000UL);
	uiInit();
	// dds.begin(&Wire);



	// // 创建JoystickReader实例（I2C）
	// std::shared_ptr<JoystickReader> joystickReader = std::make_shared<I2CJoystickReader>(JOYSTICK_I2C_ADDRESS);
	
	// // 创建Joystick实例
	// joystick = std::make_shared<Joystick>(joystickReader, 20, JOYSTICK_LONG_PRESS_DURATION_MS, JOYSTICK_MULTICLICK_INTERVAL_MS);
	
	// // 创建ButtonReader实例（数字引脚）
	// std::shared_ptr<ButtonReader> buttonReader = std::make_shared<DigitalButtonReader>(BUTTON_FIRST_PIN, BUTTON_COUNT);
	
	// // 创建ButtonHandler实例
	// buttonHandler = std::make_shared<ButtonHandler>(buttonReader, JOYSTICK_LONG_PRESS_DURATION_MS, JOYSTICK_MULTICLICK_INTERVAL_MS);
	
	// // 注册摇杆事件回调
	// joystick->attachCallback([](const JoystickEvent& event) {
	// 	if(event.source == EventSource::AXIS){
	// 		switch(event.type){
	// 			case EventType::SINGLE_CLICK:
	// 				Serial.printf("Axis Single Click Detected: %d\n", static_cast<int>(event.direction));
	// 				break;
	// 			case EventType::MULTICLICK:
	// 				Serial.printf("Axis Multiclick Detected: %d\n", static_cast<int>(event.direction));
	// 				break;
	// 			case EventType::LONG_PRESS:
	// 				Serial.printf("Axis Long Press Detected: %d\n", static_cast<int>(event.direction));
	// 				break;
	// 		}
	// 	}
	// });
	
	// // 注册按钮事件回调
	// buttonHandler->attachCallback([](const ButtonEvent& event) {
	// 	if(event.source == EventSource::BUTTON){
	// 		switch(event.type){
	// 			case EventType::SINGLE_CLICK:
	// 				Serial.printf("Button %d Single Click Detected\n", event.buttonId);
	// 				break;
	// 			case EventType::MULTICLICK:
	// 				Serial.printf("Button %d Multiclick Detected\n", event.buttonId);
	// 				break;
	// 			case EventType::LONG_PRESS:
	// 				Serial.printf("Button %d Long Press Detected\n", event.buttonId);
	// 				break;
	// 		}
	// 	}
	// });
	
	// // 创建VirtualButtonHandler实例，假设将四个方向转换为四个虚拟按钮
	// std::vector<Direction> virtualDirections = { Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT };
	// joystickToButton = std::make_shared<JoystickToButton>(joystick, virtualDirections, JOYSTICK_LONG_PRESS_DURATION_MS, JOYSTICK_MULTICLICK_INTERVAL_MS);
	
	// // 注册虚拟按钮事件回调
	// joystickToButton->attachCallback([](const ButtonEvent& event) {
	// 	if(event.source == EventSource::BUTTON){
	// 		switch(event.type){
	// 			case EventType::SINGLE_CLICK:
	// 				Serial.printf("Virtual Button %d Single Click Detected\n", event.buttonId);
	// 				break;
	// 			case EventType::MULTICLICK:
	// 				Serial.printf("Virtual Button %d Multiclick Detected\n", event.buttonId);
	// 				break;
	// 			case EventType::LONG_PRESS:
	// 				Serial.printf("Virtual Button %d Long Press Detected\n", event.buttonId);
	// 				break;
	// 		}
	// 	}
	// });

	// // 创建FreeRTOS任务
	// xTaskCreate(
	// 	joystickTask,				// 任务函数
	// 	"Joystick Task",			// 任务名称
	// 	2048,						// 堆栈大小
	// 	NULL,						// 任务输入参数
	// 	1,							// 优先级
	// 	NULL						// 任务句柄
	// );
	
	// xTaskCreate(
	// 	buttonTask,					// 任务函数
	// 	"Button Task",				// 任务名称
	// 	2048,						// 堆栈大小
	// 	NULL,						// 任务输入参数
	// 	1,							// 优先级
	// 	NULL						// 任务句柄
	// );
	
	// xTaskCreate(
	// 	joystickToButtonTask,		// 任务函数
	// 	"JoystickToButton Task",	// 任务名称
	// 	2048,						// 堆栈大小
	// 	NULL,						// 任务输入参数
	// 	1,							// 优先级
	// 	NULL						// 任务句柄
	// );
	
	// xTaskCreate(
	// 	serialTask,					// 任务函数
	// 	"Serial Task",				// 任务名称
	// 	2048,						// 堆栈大小
	// 	NULL,						// 任务输入参数
	// 	1,							// 优先级
	// 	NULL						// 任务句柄
	// );
}

void loop() {
	// 主循环可以保持空闲，所有功能由FreeRTOS任务处理
	// 或者在此添加其他非关键任务
	delay(1000); // 防止主循环占用过多CPU
}



// // Joystick任务实现
// void joystickTask(void * parameter) {
// 	(void) parameter; // 避免未使用参数的编译警告
	
// 	while (1) {
// 		joystick->update();
// 		vTaskDelay(pdMS_TO_TICKS(joystickPollingInterval));
// 	}
// }

// // Button任务实现
// void buttonTask(void * parameter) {
// 	(void) parameter; // 避免未使用参数的编译警告
	
// 	while (1) {
// 		buttonHandler->update();
// 		vTaskDelay(pdMS_TO_TICKS(buttonPollingInterval));
// 	}
// }

// // JoystickToButton任务实现
// void joystickToButtonTask(void * parameter) {
// 	(void) parameter; // 避免未使用参数的编译警告
	
// 	while (1) {
// 		joystickToButton->update();
// 		vTaskDelay(pdMS_TO_TICKS(virtualButtonPollingInterval));
// 	}
// }

// // Serial任务实现（用于动态修改轮询间隔和校正）
// void serialTask(void * parameter) {
// 	(void) parameter; // 避免未使用参数的编译警告
	
// 	while (1) {
// 		if (Serial.available() > 0) {
// 			String input = Serial.readStringUntil('\n');
// 			input.trim(); // 去除换行符和空格
			
// 			// 期望命令格式：
// 			// "joystick interval <value>" - 设置摇杆轮询间隔（毫秒）
// 			// "joystick calibrate <x> <y>" - 设置摇杆中心位置
// 			// "button interval <value>" - 设置按钮轮询间隔（毫秒）
// 			// "virtualbutton interval <value>" - 设置虚拟按钮轮询间隔（毫秒）
// 			if (input.startsWith("joystick interval")) {
// 				int spaceIdx = input.indexOf(' ', 16);
// 				if(spaceIdx > 0){
// 					String valueStr = input.substring(spaceIdx + 1);
// 					unsigned long newInterval = valueStr.toInt();
// 					if(newInterval > 0){
// 						joystickPollingInterval = newInterval;
// 						Serial.printf("Joystick polling interval updated to %lu ms\n", newInterval);
// 					}
// 					else{
// 						Serial.println("Invalid joystick interval value.");
// 					}
// 				}
// 				else{
// 					Serial.println("Invalid command format. Use: joystick interval <value>");
// 				}
// 			}
// 			else if(input.startsWith("joystick calibrate")) {
// 				// 期望格式： "joystick calibrate <x> <y>"
// 				int firstSpace = input.indexOf(' ', 17);
// 				int secondSpace = input.indexOf(' ', firstSpace + 1);
// 				if(firstSpace > 0 && secondSpace > firstSpace + 1){
// 					String xStr = input.substring(firstSpace + 1, secondSpace);
// 					String yStr = input.substring(secondSpace + 1);
// 					int calibratedX = xStr.toInt();
// 					int calibratedY = yStr.toInt();
// 					joystick->calibrate(calibratedX, calibratedY);
// 					Serial.printf("Joystick calibrated to center X: %d, Y: %d\n", calibratedX, calibratedY);
// 				}
// 				else{
// 					Serial.println("Invalid calibrate command. Use: joystick calibrate <x> <y>");
// 				}
// 			}
// 			else if (input.startsWith("button interval")) {
// 				int spaceIdx = input.indexOf(' ', 15);
// 				if(spaceIdx > 0){
// 					String valueStr = input.substring(spaceIdx + 1);
// 					unsigned long newInterval = valueStr.toInt();
// 					if(newInterval > 0){
// 						buttonPollingInterval = newInterval;
// 						Serial.printf("Button polling interval updated to %lu ms\n", newInterval);
// 					}
// 					else{
// 						Serial.println("Invalid button interval value.");
// 					}
// 				}
// 				else{
// 					Serial.println("Invalid command format. Use: button interval <value>");
// 				}
// 			}
// 			else if (input.startsWith("virtualbutton interval")) {
// 				int spaceIdx = input.indexOf(' ', 21);
// 				if(spaceIdx > 0){
// 					String valueStr = input.substring(spaceIdx + 1);
// 					unsigned long newInterval = valueStr.toInt();
// 					if(newInterval > 0){
// 						virtualButtonPollingInterval = newInterval;
// 						Serial.printf("Virtual Button polling interval updated to %lu ms\n", newInterval);
// 					}
// 					else{
// 						Serial.println("Invalid virtual button interval value.");
// 					}
// 				}
// 				else{
// 					Serial.println("Invalid command format. Use: virtualbutton interval <value>");
// 				}
// 			}
// 			else {
// 				Serial.println("Unknown command.");
// 			}
// 		}
// 		vTaskDelay(pdMS_TO_TICKS(100)); // 每100ms检查一次
// 	}
// }