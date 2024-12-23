#include "glo.hpp"

M5JoystickHAT joystick;
// 使用 std::shared_ptr 包装现有的对象
std::shared_ptr<M5JoystickHAT> joystickPtr(&joystick, [](M5JoystickHAT*){});  // 禁止删除

void initM5JoystickHAT(void) {
	joystick.begin(&Wire, JOYSTICKHAT_ADDR,
		STICKC_GPIO_SDA, STICKC_GPIO_SCL, 100000UL);
	
	Serial.printf("_addr:0x%x , _sda:%d , _scl:%d\n",
		joystick._addr, joystick._sda, joystick._scl);
	
	DevModManager.registerModule(joystickPtr);
}





Joystick4WayButtonConfig JoystickUpConfig(&joystick, 0.3, JOY_DIR_UP);
AceButton JoystickUpButton(&JoystickUpConfig, 0 /*dummy pin*/);

Joystick4WayButtonConfig JoystickDownConfig(&joystick, 0.3, JOY_DIR_DOWN);
AceButton JoystickDownButton(&JoystickDownConfig, 0 /*dummy pin*/);

Joystick4WayButtonConfig JoystickLeftConfig(&joystick, 0.3, JOY_DIR_LEFT);
AceButton JoystickLeftButton(&JoystickLeftConfig, 0);

Joystick4WayButtonConfig JoystickRightConfig(&joystick, 0.3, JOY_DIR_RIGHT);
AceButton JoystickRightButton(&JoystickRightConfig, 0);

// 回调函数
void handleEventUp(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	Serial.printf("Joystick Up Event");
	Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	// if (eventType == AceButton::kEventClicked) {
	// 	Serial.println("Up direction clicked");
	// } else if (eventType == AceButton::kEventLongPressed) {
	// 	Serial.println("Up direction long pressed");
	// }
	// similarly for downButton, leftButton, rightButton ...
}

void handleEventDown(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	Serial.printf("Joystick Down Event");
	Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	// if (eventType == AceButton::kEventClicked) {
	// 	Serial.println("Down direction clicked");
	// } else if (eventType == AceButton::kEventLongPressed) {
	// 	Serial.println("Down direction long pressed");
	// }
	// similarly for downButton, leftButton, rightButton ...
}

void handleEventLeft(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	Serial.printf("Joystick Left Event");
	Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	// if (eventType == AceButton::kEventClicked) {
	// 	Serial.println("Left direction clicked");
	// } else if (eventType == AceButton::kEventLongPressed) {
	// 	Serial.println("Left direction long pressed");
	// }
	// similarly for downButton, leftButton, rightButton ...
}

void handleEventRight(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	Serial.printf("Joystick Right Event");
	Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	// if (eventType == AceButton::kEventClicked) {
	// 	Serial.println("Right direction clicked");
	// } else if (eventType == AceButton::kEventLongPressed) {
	// 	Serial.println("Right direction long pressed");
	// }
	// similarly for downButton, leftButton, rightButton ...
}



static void checkAceButton(void) {
	// 添加回调函数
	JoystickUpButton.check();
	JoystickDownButton.check();
	JoystickLeftButton.check();
	JoystickRightButton.check();
}

// 轮询任务函数
static void AceButtonCheckTask(void *pvParam) {
	const TickType_t period = pdMS_TO_TICKS(ACEBUTTON_POLL_INTERVAL_MS);
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		checkAceButton();
		vTaskDelayUntil(&lastWakeTime, period);
	}
}

void initJoystick4WayButtonsCheckTask() {
	// 通过 xTaskCreate() 创建轮询任务
	xTaskCreate(
		AceButtonCheckTask,			// 任务函数
		"AceButtonCheckTask",		// 任务名称
		2048,						// 堆栈大小(根据需求调整)
		NULL,						// 传递给任务的参数(指向manager)
		ACEBUTTON_TASK_PRIORITY,	// 任务优先级(宏定义)
		NULL						// 任务句柄(可选)
	);
}


void initJoystick4WayButtons(void) {
	// 可setFeature
	// JoystickUpConfig.postInit();
	JoystickUpConfig.setFeature(ButtonConfig::kFeatureClick);
	// JoystickUpConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickUpConfig.setFeature(ButtonConfig::kFeatureLongPress);

	// JoystickDownConfig.postInit();
	JoystickDownConfig.setFeature(ButtonConfig::kFeatureClick);
	// JoystickDownConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickDownConfig.setFeature(ButtonConfig::kFeatureLongPress);

	// JoystickLeftConfig.postInit();
	JoystickLeftConfig.setFeature(ButtonConfig::kFeatureClick);
	// JoystickLeftConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickLeftConfig.setFeature(ButtonConfig::kFeatureLongPress);

	// JoystickRightConfig.postInit();
	JoystickRightConfig.setFeature(ButtonConfig::kFeatureClick);
	// JoystickRightConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickRightConfig.setFeature(ButtonConfig::kFeatureLongPress);


	// JoystickUpConfig.setDebounceDelay(50);
	// JoystickDownConfig.setDebounceDelay(50);
	// JoystickLeftConfig.setDebounceDelay(50);
	// JoystickRightConfig.setDebounceDelay(50);

	// JoystickUpConfig.setClickDelay(100);
	// JoystickDownConfig.setClickDelay(100);
	// JoystickLeftConfig.setClickDelay(100);
	// JoystickRightConfig.setClickDelay(100);


	// 添加回调函数
	JoystickUpButton.setEventHandler(handleEventUp);
	JoystickDownButton.setEventHandler(handleEventDown);
	JoystickLeftButton.setEventHandler(handleEventLeft);
	JoystickRightButton.setEventHandler(handleEventRight);


	joystick.debugPrintParams();
	JoystickUpConfig.debugPrintParams();
}
