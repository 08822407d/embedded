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
	joystick->setRotation(GLOBAL_ROTATION);
	
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

// std::shared_ptr<Joystick4WayButtonConfig> JoystickUpConfig = nullptr;
// std::shared_ptr<Joystick4WayButtonConfig> JoystickDownConfig = nullptr;
// std::shared_ptr<Joystick4WayButtonConfig> JoystickLeftConfig = nullptr;
// std::shared_ptr<Joystick4WayButtonConfig> JoystickRightConfig = nullptr;

// 回调函数
void handleEventUp(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	// Serial.printf("Joystick Up Event");
	// Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	if (eventType == AceButton::kEventPressed) {
		Serial.println("Up");
	} else if (eventType == AceButton::kEventLongPressed) {
		Serial.println("Up Long");
	}
	// similarly for downButton, leftButton, rightButton ...
}

void handleEventDown(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	// Serial.printf("Joystick Down Event");
	// Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	if (eventType == AceButton::kEventPressed) {
		Serial.println("Down");
	} else if (eventType == AceButton::kEventLongPressed) {
		Serial.println("Down Long");
	}
	// similarly for downButton, leftButton, rightButton ...
}

void handleEventLeft(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	// Serial.printf("Joystick Left Event");
	// Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	if (eventType == AceButton::kEventPressed) {
		Serial.println("Left");
	} else if (eventType == AceButton::kEventLongPressed) {
		Serial.println("Left Long");
	}
	// similarly for downButton, leftButton, rightButton ...
}

void handleEventRight(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	// Serial.printf("Joystick Right Event");
	// Serial.printf("  %s , %d\n", button->eventName(eventType), buttonState);

	if (eventType == AceButton::kEventPressed) {
		Serial.println("Right");
	} else if (eventType == AceButton::kEventLongPressed) {
		Serial.println("Right Long");
	}
	// similarly for downButton, leftButton, rightButton ...
}

// void handleEvent4Ways(AceButton* button, uint8_t eventType, uint8_t buttonState) {
// 	String direction = "";
// 	if (button == JoystickUpButton.get())
// 		direction = "Up";
// 	else if (button == JoystickDownButton.get())
// 		direction = "Down";
// 	else if (button == JoystickLeftButton.get())
// 		direction = "Left";
// 	else if (button == JoystickRightButton.get())
// 		direction = "Right";
// 	else
// 		direction = "Unknown";
		
// 	String event = "";
// 	if (eventType == AceButton::kEventPressed)
// 		event = "Pressed";
// 	else if (eventType == AceButton::kEventLongPressed)
// 		event = "LongPressed";

// 	Serial.println(direction + " " + event);
// 	// similarly for downButton, leftButton, rightButton ...
// }


void initJoystick4WayButtons(void) {
	auto JoystickUpConfig =
		std::make_shared<Joystick4WayButtonConfig>(
			joystickPtr.get(), JOYSTICK_TRIGGER_THRESAHOLD, JOY_DIR_UP
		);
	auto JoystickDownConfig =
		std::make_shared<Joystick4WayButtonConfig>(
			joystickPtr.get(), JOYSTICK_TRIGGER_THRESAHOLD, JOY_DIR_DOWN
		);
	auto JoystickLeftConfig =
		std::make_shared<Joystick4WayButtonConfig>(
			joystickPtr.get(), JOYSTICK_TRIGGER_THRESAHOLD, JOY_DIR_LEFT
		);
	auto JoystickRightConfig =
		std::make_shared<Joystick4WayButtonConfig>(
			joystickPtr.get(), JOYSTICK_TRIGGER_THRESAHOLD, JOY_DIR_RIGHT
		);

	// // 可setFeature
	// // JoystickUpConfig->resetFeatures();
	// JoystickUpConfig->setFeature(ButtonConfig::kFeatureClick);
	// JoystickUpConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickUpConfig->setFeature(ButtonConfig::kFeatureLongPress);

	// // JoystickDownConfig->resetFeatures();
	// JoystickDownConfig->setFeature(ButtonConfig::kFeatureClick);
	// JoystickDownConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickDownConfig->setFeature(ButtonConfig::kFeatureLongPress);

	// // JoystickLeftConfig->resetFeatures();
	// JoystickLeftConfig->setFeature(ButtonConfig::kFeatureClick);
	// JoystickLeftConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickLeftConfig->setFeature(ButtonConfig::kFeatureLongPress);

	// // JoystickRightConfig->resetFeatures();
	// JoystickRightConfig->setFeature(ButtonConfig::kFeatureClick);
	// JoystickRightConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	JoystickRightConfig->setFeature(ButtonConfig::kFeatureLongPress);


	JoystickUpButton = std::make_shared<AceButton>(JoystickUpConfig.get(), 0, LOW);
	JoystickDownButton = std::make_shared<AceButton>(JoystickDownConfig.get(), 0, LOW);
	JoystickLeftButton = std::make_shared<AceButton>(JoystickLeftConfig.get(), 0, LOW);
	JoystickRightButton = std::make_shared<AceButton>(JoystickRightConfig.get(), 0, LOW);

	// 添加回调函数
	JoystickUpButton->setEventHandler(handleEventUp);
	JoystickDownButton->setEventHandler(handleEventDown);
	JoystickLeftButton->setEventHandler(handleEventLeft);
	JoystickRightButton->setEventHandler(handleEventRight);
	// JoystickUpButton->setEventHandler(handleEvent4Ways);
	// JoystickDownButton->setEventHandler(handleEvent4Ways);
	// JoystickLeftButton->setEventHandler(handleEvent4Ways);
	// JoystickRightButton->setEventHandler(handleEvent4Ways);


	// JoystickUpConfig->debugPrintParams();
}
