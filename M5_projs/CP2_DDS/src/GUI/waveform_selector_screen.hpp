#pragma once

#include <lvgl.h>

#include "InteractManager.hpp"


class RollerScreenPage : public ScreenPage {
public:
	

	RollerScreenPage(
		std::string name,
		ScreenPage *parent = nullptr,
		lv_style_t *style = nullptr
	);
	~RollerScreenPage() {}

	void init();
	void dispose();
	void enterPage(lv_screen_load_anim_t anim);
	void exitPage(lv_screen_load_anim_t anim);

	void addItem(std::string name);
	void setLastSelected(int index) { this->_last_selected = index; }


protected:
	lv_obj_t	*_lvgl_roller;
	std::string	_items_str;
	int			_last_selected = -1;


private:
	using ScreenPage::addChild;
};


extern lv_style_t *createDefualtRollerScreenStyle(void);
extern lv_obj_t *createRoller(lv_obj_t *parent, lv_style_t *style, void (*handler)(lv_event_t *e));