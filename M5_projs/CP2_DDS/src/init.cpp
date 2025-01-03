#include "init.hpp"


// 全局智能指针定义并初始化为nullptr
std::shared_ptr<M5JoystickHAT> joystickPtr = nullptr;
std::shared_ptr<AceButton> Joystick4WayButton = nullptr;

void handleEvent4Ways(AceButton* button, uint8_t eventType, uint8_t buttonState);


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
	// Serial.printf("Joystick XY Max: %d , %d\n", joystickPtr->getXmax(), joystickPtr->getYmax());
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
}
