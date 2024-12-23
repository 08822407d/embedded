#include "init.hpp"


// 全局智能指针定义并初始化为nullptr
std::shared_ptr<M5JoystickHAT> joystickPtr = nullptr;

void initM5JoystickHAT(void) {
	// 初始化摇杆
    joystickPtr = std::make_shared<M5JoystickHAT>();
	M5JoystickHAT *joystick = joystickPtr.get();

	joystick->begin(&Wire, JOYSTICKHAT_ADDR,
		STICKC_GPIO_SDA, STICKC_GPIO_SCL, 100000UL);
	joystick->setRotation(GLOBAL_ROTATION);
	
	// Serial.printf("_addr:0x%x , _sda:%d , _scl:%d\n",
	// 	joystick->_addr, joystick->_sda, joystick->_scl);
	
	DevModManager.registerModule(joystickPtr);

	// joystick->debugPrintParams();
	Serial.printf("Joystick XY Max: %d , %d\n", joystickPtr->getXmax(), joystickPtr->getYmax());
}


std::shared_ptr<AceButton> Joystick4WayButton = nullptr;

void handleEvent4Ways(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	// String direction_str = "";

	// 从 ButtonConfig 获取当前方向
	Joystick4WayButtonConfig* config = static_cast<Joystick4WayButtonConfig*>(button->getButtonConfig());
	if (config) {
		JoystickDirectionID direction = config->getCurrDirection();
		if (direction == JOY_NONE) {
			return;		//	回正事件，不做处理
		}

		M5.Lcd.clear();
		switch (direction) {
			case JOY_DIR_UP:
				// direction_str = "Up";
				M5.Lcd.drawTriangle(120, 45, 110, 55, 130, 55, TFT_GREEN);
				break;
			case JOY_DIR_DOWN:
				// direction_str = "Down";
				M5.Lcd.drawTriangle(120, 85, 110, 75, 130, 75, TFT_GREEN);
				break;
			case JOY_DIR_LEFT:
				// direction_str = "Left";
				M5.Lcd.drawTriangle(100, 65, 110, 55, 110, 75, TFT_GREEN);
				break;
			case JOY_DIR_RIGHT:
				// direction_str = "Right";
				M5.Lcd.drawTriangle(140, 65, 130, 55, 130, 75, TFT_GREEN);
				break;
			default:
				// direction_str = "Unknown";
				M5.Lcd.clear();
				break;
		}
	}
		
	// String event = "";
	// if (eventType == AceButton::kEventPressed)
	// 	event = "Pressed";
	// else if (eventType == AceButton::kEventLongPressed)
	// 	event = "LongPressed";

	// Serial.println(direction_str + " " + event);
}


void initJoystick4WayButtons(void) {
	auto Joystick4WayConfig =
		std::make_shared<Joystick4WayButtonConfig>(
			joystickPtr.get(), JOYSTICK_TRIGGER_THRESAHOLD
		);

	// Joystick4WayConfig->resetFeatures();
	// Joystick4WayConfig->setFeature(ButtonConfig::kFeatureClick);
	// Joystick4WayConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
	Joystick4WayConfig->setFeature(ButtonConfig::kFeatureLongPress);

	Joystick4WayButton = std::make_shared<AceButton>(Joystick4WayConfig.get(), 0, LOW);

	// 添加回调函数
	Joystick4WayButton->setEventHandler(handleEvent4Ways);


	// JoystickUpConfig->debugPrintParams();
}
