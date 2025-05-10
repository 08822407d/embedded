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

	void lvgl_restore_style(lv_style_t *style);

	void setDataReciever(int32_t *data) { this->_data_reciever = data; }

protected:
	lv_obj_t	*_lvgl_spinbox;
	bool		_edit_mode = false;
	lv_style_t	_lvgl_noneditable_style;
	lv_style_t	_lvgl_editable_style;
	int32_t		*_data_reciever = nullptr;

	void __lvgl_KeyEventSpecial(lv_key_t *key) override;
};