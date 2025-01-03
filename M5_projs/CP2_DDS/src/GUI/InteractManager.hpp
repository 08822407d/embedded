#pragma once

#include <vector>
#include <memory>
#include <string>

#include <lvgl.h>
// #include <AceButton.h>
#include "driver/Joystick4WayButtons.hpp"

#include "ScreenPage.hpp"



class InteractManager {
public:
	explicit InteractManager(std::shared_ptr<ScreenPage> RootPage)
	: _root(RootPage) {
		this->setCurrent(RootPage);
	}

	std::shared_ptr<ScreenPage> getCurrent() const { return _current; }
	void setCurrent(std::shared_ptr<ScreenPage> page) { _current = page; }

    void PeripheralEventRepeater(lv_key_t key);

private:
	std::shared_ptr<ScreenPage>	_current{ nullptr };
	std::shared_ptr<ScreenPage>	_root{ nullptr };
};


extern std::shared_ptr<InteractManager> DDSInteractManager;