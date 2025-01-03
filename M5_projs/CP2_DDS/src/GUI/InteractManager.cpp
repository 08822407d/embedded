#include "InteractManager.hpp"

std::shared_ptr<InteractManager> DDSInteractManager = nullptr;

void InteractManager::PeripheralEventRepeater(lv_key_t key) {
    std::shared_ptr<ScreenPage> page = DDSInteractManager->getCurrent();
    lv_obj_t *lv_page = page->lvgl_widget;
    lv_obj_send_event(lv_page, LV_EVENT_KEY, &key);
}

void handleEvent4Ways(AceButton* button, uint8_t eventType, uint8_t buttonState) {
	// 从 ButtonConfig 获取当前方向
	Joystick4WayButtonConfig* config = static_cast<Joystick4WayButtonConfig*>(button->getButtonConfig());
	if (config) {
		JoystickDirectionID direction = config->getCurrDirection();
		if (direction == JOY_NONE) {
			return;		//	回正事件，不做处理
		}

        lv_key_t key = (lv_key_t)-1;
		switch (direction) {
			case JOY_DIR_UP:
				key = LV_KEY_UP;
				break;
			case JOY_DIR_DOWN:
				key = LV_KEY_DOWN;
				break;
			case JOY_DIR_LEFT:
				key = LV_KEY_LEFT;
				break;
			case JOY_DIR_RIGHT:
				key = LV_KEY_RIGHT;
				break;
			default:
				break;
		}
        
        if (key != (lv_key_t)-1)
            DDSInteractManager->PeripheralEventRepeater(key);
	}
}

