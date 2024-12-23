#include "init.hpp"


// 全局智能指针定义并初始化为nullptr
std::shared_ptr<M5JoystickHAT> joystickPtr = nullptr;
M5JoystickHAT *joystick;

void initM5JoystickHAT(void) {
	// 初始化摇杆
    joystickPtr = std::make_shared<M5JoystickHAT>();
	joystick = joystickPtr.get();

	joystick->begin(&Wire, JOYSTICKHAT_ADDR,
		STICKC_GPIO_SDA, STICKC_GPIO_SCL, 100000UL);
	
	// Serial.printf("_addr:0x%x , _sda:%d , _scl:%d\n",
	// 	joystick->_addr, joystick->_sda, joystick->_scl);
	
	DevModManager.registerModule(joystickPtr);

	// joystick->debugPrintParams();
	Serial.printf("Joystick XY Max: %d , %d\n", joystickPtr->getXmax(), joystickPtr->getYmax());
}



std::shared_ptr<AceButton> JoystickUpButton = nullptr;
std::shared_ptr<AceButton> JoystickDownButton = nullptr;
std::shared_ptr<AceButton> JoystickLeftButton = nullptr;
std::shared_ptr<AceButton> JoystickRightButton = nullptr;

std::shared_ptr<Joystick4WayButtonConfig> JoystickUpConfig = nullptr;
std::shared_ptr<Joystick4WayButtonConfig> JoystickDownConfig = nullptr;
std::shared_ptr<Joystick4WayButtonConfig> JoystickLeftConfig = nullptr;
std::shared_ptr<Joystick4WayButtonConfig> JoystickRightConfig = nullptr;

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
	JoystickUpButton->check();
	JoystickDownButton->check();
	JoystickLeftButton->check();
	JoystickRightButton->check();
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
	JoystickUpConfig = std::make_shared<Joystick4WayButtonConfig>(joystickPtr.get(), 0.3, JOY_DIR_UP);
	JoystickDownConfig = std::make_shared<Joystick4WayButtonConfig>(joystickPtr.get(), 0.3, JOY_DIR_DOWN);
	JoystickLeftConfig = std::make_shared<Joystick4WayButtonConfig>(joystickPtr.get(), 0.3, JOY_DIR_LEFT);
	JoystickRightConfig = std::make_shared<Joystick4WayButtonConfig>(joystickPtr.get(), 0.3, JOY_DIR_RIGHT);

	// 可setFeature
	// JoystickUpConfig->resetFeatures();
	JoystickUpConfig->setFeature(ButtonConfig::kFeatureClick);
	JoystickUpConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickUpConfig->setFeature(ButtonConfig::kFeatureLongPress);

	// JoystickDownConfig->resetFeatures();
	JoystickDownConfig->setFeature(ButtonConfig::kFeatureClick);
	JoystickDownConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickDownConfig->setFeature(ButtonConfig::kFeatureLongPress);

	// JoystickLeftConfig->resetFeatures();
	JoystickLeftConfig->setFeature(ButtonConfig::kFeatureClick);
	JoystickLeftConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickLeftConfig->setFeature(ButtonConfig::kFeatureLongPress);

	// JoystickRightConfig->resetFeatures();
	JoystickRightConfig->setFeature(ButtonConfig::kFeatureClick);
	JoystickRightConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickRightConfig->setFeature(ButtonConfig::kFeatureLongPress);


	JoystickUpButton = std::make_shared<AceButton>(JoystickUpConfig.get(), 0);
	JoystickDownButton = std::make_shared<AceButton>(JoystickDownConfig.get(), 0);
	JoystickLeftButton = std::make_shared<AceButton>(JoystickLeftConfig.get(), 0);
	JoystickRightButton = std::make_shared<AceButton>(JoystickRightConfig.get(), 0);

	// 添加回调函数
	JoystickUpButton->setEventHandler(handleEventUp);
	JoystickDownButton->setEventHandler(handleEventDown);
	JoystickLeftButton->setEventHandler(handleEventLeft);
	JoystickRightButton->setEventHandler(handleEventRight);


	// JoystickUpConfig->debugPrintParams();
}
