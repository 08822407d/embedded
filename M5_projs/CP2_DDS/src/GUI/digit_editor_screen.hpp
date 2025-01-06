#pragma once

#include <lvgl.h>

#include "InteractManager.hpp"


class DigitEditorScreenPage : public ScreenPage {
public:
	// 1) 生命周期
	void init();		// 在系统初始化阶段调用
	void dispose();		// 资源或内存回收
	void enterPage(lv_screen_load_anim_t anim);	// 当切换进本页面
	void exitPage(lv_screen_load_anim_t anim);	// 当切换离开本页面

	DigitEditorScreenPage(
		std::string name,
		ScreenPage *parent = nullptr,
		int min = 0,
		int max = 100,
		lv_style_t *style = nullptr
	);
	~DigitEditorScreenPage() {}

	void addChild(ScreenPage *child) = delete;

	void setLastSelected(int index) { this->_last_selected = index; }

protected:
	lv_obj_t	*_lvgl_spinbox;
	bool		_edit_mode = false;
	int			_last_selected = -1;

	void __lvgl_KeyEventSpecial(lv_key_t *key) override;
};