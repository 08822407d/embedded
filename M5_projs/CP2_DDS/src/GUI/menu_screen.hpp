#pragma once

#include <lvgl.h>

#include "InteractManager.hpp"


class MenuScreenPage : public ScreenPage {
public:
	MenuScreenPage(std::string name,
			ScreenPage *parent = nullptr)
	:	ScreenPage(name, parent, MenuKeyMap) {}
	~MenuScreenPage() {}

	// 1) 生命周期
	virtual void init() = 0;		// 在系统初始化阶段调用
	virtual void dispose() = 0;		// 资源或内存回收
	virtual void enterPage(lv_screen_load_anim_t anim) = 0;	// 当切换进本页面
	virtual void exitPage(lv_screen_load_anim_t anim) = 0;	// 当切换离开本页面

	// 2) 接收事件
	// virtual void lvgl_RecieveKeyEvent(int keyIndex) = 0;

	// 3) 子页面树的管理
	virtual void addItem(ScreenPage *ItemPage) {}
	void setLastSelected(int index) { this->_last_selected = index; }

	lv_obj_t *getMenu() { return this->_lvgl_menu; }


protected:
	lv_obj_t	*_lvgl_menu;
	int			_last_selected = -1;

	using ScreenPage::addChild;
};



class RollerMenuScreenPage : public MenuScreenPage {
public:
	RollerMenuScreenPage(
		std::string name,
		ScreenPage *parent = nullptr,
		lv_style_t *style = nullptr
	);

	~RollerMenuScreenPage() {}

	void init();
	void dispose();
	void enterPage(lv_screen_load_anim_t anim);
	void exitPage(lv_screen_load_anim_t anim);

	void addItem(ScreenPage *child);


protected:
	std::string	_items_str;


private:
	using ScreenPage::addChild;
};