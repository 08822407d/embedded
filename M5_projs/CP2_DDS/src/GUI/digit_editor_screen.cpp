#include "digit_editor_screen.hpp"


void lv_spinbox_toggle_cursor_visibility(lv_obj_t * spinbox, bool en) {
	lv_obj_set_style_text_color(spinbox, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_text_color(spinbox, (en)? lv_color_white() : lv_color_black(), LV_PART_CURSOR);
	lv_obj_set_style_bg_opa(spinbox, (en)? 255 : 0, LV_PART_CURSOR);
	// lv_obj_invalidate(spinbox);
}

static void event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
	if(code == LV_EVENT_KEY) {
		lv_key_t key = static_cast<lv_key_t>(lv_event_get_key(e));
		// Serial.printf("Key: %d\n", static_cast<int>(key));

		// Serial.printf("Children count: %d\n", currPage->getChildren().size());
		lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE;
		if (key == LV_KEY_PREV) {
			ScreenPage *curr = DDSInteractManager->getCurrent();
			ScreenPage *parent = curr->getParent();
			assert (parent != nullptr);
			// Serial.printf("Selected Page: %s\n", selectedPage->getName().c_str());
			curr->exitPage(LV_SCR_LOAD_ANIM_OVER_RIGHT);
			parent->enterPage(LV_SCR_LOAD_ANIM_OVER_RIGHT);
		}
	}
}

lv_obj_t *createSpinbox(int min = 0, int max = 100,
		lv_obj_t *parent = nullptr, lv_style_t *style = nullptr) {

	int digits = 0;
	int temp_max = max;
	while (temp_max != 0) {
		digits++;
		temp_max /= 10;
	}

	lv_obj_t *spinbox = lv_spinbox_create(parent);
	lv_spinbox_set_range(spinbox, min, max);
	lv_spinbox_set_digit_format(spinbox, digits, 0);
	lv_spinbox_step_prev(spinbox);
	lv_obj_set_width(spinbox, lv_pct(60));
	lv_obj_center(spinbox);

	lv_spinbox_set_cursor_pos(spinbox, 0);
	lv_spinbox_set_rollover(spinbox, true);
	lv_obj_add_event_cb(spinbox, event_handler, LV_EVENT_KEY, NULL);

	// 设置字号
	lv_obj_set_style_text_align(spinbox, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_text_font(spinbox, &lv_font_montserrat_22,
		(lv_style_selector_t)((int)LV_PART_MAIN | (int)LV_STATE_DEFAULT));

	return spinbox;
}

DigitEditorScreenPage::DigitEditorScreenPage(
		std::string name, ScreenPage *parent, int min, int max, lv_style_t *style)
:	ScreenPage(name, parent, LeafPageKeyMap) {
	this->_lvgl_spinbox = createSpinbox(min, max, this->_lvgl_container, style);
	lv_spinbox_toggle_cursor_visibility(this->_lvgl_spinbox, false);

	// 将默认滚轮添加到组
	lv_group_add_obj(this->_lvgl_group, this->_lvgl_spinbox);
	// 设置组的焦点到默认按钮
	lv_group_focus_obj(this->_lvgl_spinbox);
}

void DigitEditorScreenPage::init() {
}

void DigitEditorScreenPage::dispose() {
}

void DigitEditorScreenPage::enterPage(lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE) {
	DDSInteractManager->setCurrent(this);
	// lv_scr_load(this->lvgl_GetScreen());
	// 2) 使用带动画的方式切换到 new_scr
	lv_scr_load_anim(
		this->lvgl_GetScreen(), 
		anim,						// 动画类型: 旧屏幕从右往左滑动出去, 新屏幕从右往左滑入
		500,						// 动画时长 500ms
		0,							// 无延时
		false						// 动画结束后不自动删除旧screen
	);
	// lv_roller_set_selected(this->_lvgl_menu, this->_last_selected, LV_ANIM_OFF);
}

void DigitEditorScreenPage::exitPage(lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE) {
}

void DigitEditorScreenPage::__lvgl_KeyEventSpecial(lv_key_t *key) {
	if (*key != LV_KEY_ESC) return;

	this->_edit_mode = !this->_edit_mode;
	Serial.printf("Edit mode = %s\n", this->_edit_mode ? "true" : "false");
	if (this->_edit_mode) {
		this->KeyMap = SpinboxKeyMap;
	} else {
		this->KeyMap = LeafPageKeyMap;
	}
	lv_spinbox_toggle_cursor_visibility(this->_lvgl_spinbox, this->_edit_mode);
}