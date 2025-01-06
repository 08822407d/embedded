#include "InteractManager.hpp"


lv_key_t MenuKeyMap[5] = {
	[JOY_MID_BUTTON] = LV_KEY_ENTER,
	[JOY_DIR_UP] = LV_KEY_UP,
	[JOY_DIR_DOWN] = LV_KEY_DOWN,
	[JOY_DIR_LEFT] = LV_KEY_PREV,
	[JOY_DIR_RIGHT] = LV_KEY_NEXT,
};

lv_key_t SpinboxKeyMap[5] = {
	[JOY_MID_BUTTON] = LV_KEY_ESC,
	[JOY_DIR_UP] = LV_KEY_UP,
	[JOY_DIR_DOWN] = LV_KEY_DOWN,
	[JOY_DIR_LEFT] = LV_KEY_LEFT,
	[JOY_DIR_RIGHT] = LV_KEY_RIGHT,
};

lv_key_t LeafPageKeyMap[5] = {
	[JOY_MID_BUTTON] = LV_KEY_ESC,
	[JOY_DIR_UP] = LV_KEY_ESC,
	[JOY_DIR_DOWN] = LV_KEY_ESC,
	[JOY_DIR_LEFT] = LV_KEY_PREV,
	[JOY_DIR_RIGHT] = LV_KEY_NEXT,
};

lv_key_t NormalKeyMap[5] = {
	[JOY_MID_BUTTON] = LV_KEY_ENTER,
	[JOY_DIR_UP] = LV_KEY_UP,
	[JOY_DIR_DOWN] = LV_KEY_DOWN,
	[JOY_DIR_LEFT] = LV_KEY_LEFT,
	[JOY_DIR_RIGHT] = LV_KEY_RIGHT,
};


std::shared_ptr<InteractManager> DDSInteractManager = nullptr;


void handleEvent4Ways(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	// 从 ButtonConfig 获取当前方向
	Joystick4WayButtonConfig* config = static_cast<Joystick4WayButtonConfig*>(button->getButtonConfig());
	if (config) {
		JoystickDirectionID direction = config->getCurrDirection();
		if (direction == JOY_NONE) {
			return;		//	回正事件，不做处理
		}

		// Serial.printf("Direction: %d\n", direction);
		DDSInteractManager->handleEvent4Ways(static_cast<int>(direction));
	}
}

