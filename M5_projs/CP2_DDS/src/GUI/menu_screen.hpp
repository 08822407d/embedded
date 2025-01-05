#pragma once

#include <lvgl.h>

#include "InteractManager.hpp"


class MenuScreenPage : public ScreenPage {
public:

	// 1) 生命周期
	virtual void init() = 0;		// 在系统初始化阶段调用
	virtual void dispose() = 0;		// 资源或内存回收
	virtual void enterPage() = 0;	// 当切换进本页面
	virtual void exitPage() = 0;	// 当切换离开本页面

	MenuScreenPage(std::string name,
			ScreenPage *parent = nullptr)
	:	ScreenPage(name, parent, MenuKeyMap) {}

	~MenuScreenPage() {}

	virtual void addItem(ScreenPage *ItemPage) {}

	lv_obj_t *getMenu() { return this->_lvgl_menu; }

protected:
	lv_obj_t	*_lvgl_menu;

	using ScreenPage::addChild;
};



class RollerMenuScreenPage : public MenuScreenPage {
public:
	RollerMenuScreenPage(
		std::string name,
		ScreenPage *parent = nullptr,
		lv_style_t *style = nullptr);

	~RollerMenuScreenPage() {}

	void init();
	void dispose();
	void enterPage();
	void exitPage();

	void addItem(ScreenPage *child);


protected:
	std::string	_items_str;

	lv_obj_t *__createRollerMenu(std::string name, lv_style_t *style);


private:
	using ScreenPage::addChild;
};