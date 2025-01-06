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
		ScreenPage *page = this->getCurrent();
		lv_key_t key = page->KeyMap[keyIndex];
		page->lvgl_RecieveKeyEvent(&key);
	}

private:
	ScreenPage	*_current{ nullptr };
	ScreenPage	*_root{ nullptr };
};


extern lv_key_t MenuKeyMap[5];
extern lv_key_t SpinboxKeyMap[5];
extern lv_key_t LeafPageKeyMap[5];
extern lv_key_t NormalKeyMap[5];

extern std::shared_ptr<InteractManager> DDSInteractManager;