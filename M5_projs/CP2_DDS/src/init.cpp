#include "init.hpp"
#include "debug_utils.hpp"


// 全局智能指针定义并初始化为nullptr
std::shared_ptr<M5JoystickHAT> joystickPtr = nullptr;
std::shared_ptr<AceButton> Joystick4WayButton = nullptr;
std::shared_ptr<M5UnitDDS> ddsPtr = nullptr;

void handleEvent4Ways(AceButton* button, uint8_t eventType, uint8_t buttonState);


void initM5UnitDDS(void) {
	MODULE_LOG_HEAD( M5UnitDDS );

	ddsPtr = std::make_shared<M5UnitDDS>();
	ddsPtr->begin(&Wire);
	ddsPtr->setWave(globalDDSparams.WaveForm,
		globalDDSparams.Frequency,
		globalDDSparams.Phase);

	ddsPtr->end();
	MODULE_LOG_TAIL( " ... Setup done\n" );
}

void initM5JoystickHAT(void) {
	MODULE_LOG_HEAD( M5JoystickHAT );

	// 初始化摇杆
    joystickPtr = std::make_shared<M5JoystickHAT>();
	joystickPtr->begin(&Wire);
	joystickPtr->setRotation(GLOBAL_ROTATION);
	
	// Serial.printf("_addr:0x%x , _sda:%d , _scl:%d\n",
	// 	joystick->_addr, joystick->_sda, joystick->_scl);
	
	DevModManager.registerModule(joystickPtr);

	MODULE_LOG_TAIL( " ... Setup done\n" );
}

void initJoystick4WayButtons(void) {
	MODULE_LOG_HEAD( Joystick4WayButtons );

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

	MODULE_LOG_TAIL( " ... Setup done\n" );
}
