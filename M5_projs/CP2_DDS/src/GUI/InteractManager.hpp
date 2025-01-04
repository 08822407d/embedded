#pragma once

#include <vector>
#include <memory>
#include <string>

#include <lvgl.h>
#include "driver/Joystick4WayButtons.hpp"

#include "ScreenPage.hpp"



class InteractManager {
public:
	explicit InteractManager(ScreenPage *RootPage)
	: _root(RootPage) {
		this->setCurrent(RootPage);
	}

	ScreenPage *getCurrent() const { return _current; }
	void setCurrent(ScreenPage *page) { _current = page; }

	void handleEvent4Ways(int keyIndex) {
		// ScreenPage *page = this->getCurrent();
		// lv_obj_t *lv_evt_target = page->lvgl_event_receiver;
		// // Serial.printf("key: %d\n", key);
		// lv_key_t key = page->KeyMap[keyIndex];
		// lv_obj_send_event(lv_evt_target, LV_EVENT_KEY, &key);
	}

private:
	ScreenPage	*_current{ nullptr };
	ScreenPage	*_root{ nullptr };
};


extern lv_key_t MenuKeyMap[5];
extern lv_key_t NormalKeyMap[5];

extern std::shared_ptr<InteractManager> DDSInteractManager;