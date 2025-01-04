#pragma once

#include <lvgl.h>

#include "InteractManager.hpp"


class MenuScreenPage : public ScreenPage {
public:
	// 1) 生命周期
	void init();		// 在系统初始化阶段调用
	void dispose();		// 资源或内存回收
	void enterPage();	// 当切换进本页面
	void exitPage();	// 当切换离开本页面

	MenuScreenPage(std::string name,
		ScreenPage *parent = nullptr)
	: ScreenPage(name, parent, MenuKeyMap) {
		lv_group_create();
	}

	~MenuScreenPage() {
		lv_group_delete(this->_group);
	}

	void addChild(ScreenPage *child) {
		ScreenPage::addChild(child);
		// lv_group_add_obj(this->_group, child->lvgl_event_receiver);
	}

	void addToGroup(lv_obj_t *obj) {
		lv_group_add_obj(this->_group, obj);
	}

private:
	lv_obj_t	*_lvgl_container;
	lv_obj_t	*_lvgl_menu;
	lv_group_t	*_group;
};


extern lv_obj_t *createSettingsScreen(std::string name);
extern lv_obj_t *createWaveSettingsScreen(std::string name);