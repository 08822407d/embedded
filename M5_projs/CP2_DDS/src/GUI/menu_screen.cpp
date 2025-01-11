#include "menu_screen.hpp"
#include "InteractManager.hpp"

#include "init.hpp"



static void RollerMenu_EventHandler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
	if(code == LV_EVENT_KEY) {
		lv_key_t key = static_cast<lv_key_t>(lv_event_get_key(e));
		int index = lv_roller_get_selected(obj);
		// Serial.printf("Key: %d ; Selected value: %d\n", key, index);


		MenuScreenPage *currPage = dynamic_cast<MenuScreenPage *>(DDSInteractManager->getCurrent());
		currPage->setLastSelected(index);
		// Serial.printf("Children count: %d\n", currPage->getChildren().size());
		ScreenPage *selectedPage = nullptr;
		lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE;
		if (key == LV_KEY_ENTER || key == LV_KEY_NEXT) {
			selectedPage = currPage->getChild(index);
			anim = LV_SCR_LOAD_ANIM_OVER_LEFT;
		} else if (key == LV_KEY_PREV) {
			selectedPage = currPage->getParent();
			anim = LV_SCR_LOAD_ANIM_OVER_RIGHT;
		}
		if (selectedPage != nullptr) {
			// Serial.printf("Selected Page: %s\n", selectedPage->getName().c_str());
			currPage->exitPage(anim);
			selectedPage->enterPage(anim);
		}
	}
}


lv_style_t *createDefualtRollerScreenStyle() {
	/*A style to make the selected option larger*/
	static lv_style_t style_sel;
	lv_style_init(&style_sel);
	lv_style_set_text_font(&style_sel, &lv_font_montserrat_22);
	lv_style_set_bg_color(&style_sel, lv_color_hex3(0xf88));
	lv_style_set_border_width(&style_sel, 2);
	lv_style_set_border_color(&style_sel, lv_color_hex3(0xf00));
	return &style_sel;
}

lv_obj_t *createRoller(lv_obj_t *parent, lv_style_t *style, void (*handler)(lv_event_t *e)) {
	lv_obj_t *roller = lv_roller_create(parent);
	lv_roller_set_options(roller,
		"", LV_ROLLER_MODE_INFINITE);

	lv_obj_set_width(roller, lv_pct(60));
	if (style != nullptr)
		lv_obj_add_style(roller, style, LV_PART_SELECTED);
	lv_roller_set_visible_row_count(roller, 3);
	lv_obj_center(roller);
	lv_obj_add_event_cb(roller, handler, LV_EVENT_KEY, NULL);

	return roller;
}


RollerMenuScreenPage::RollerMenuScreenPage(
		std::string name, ScreenPage *parent, lv_style_t *style)
:	MenuScreenPage(name, parent) {
	this->_lvgl_menu = createRoller(this->_lvgl_container, style, RollerMenu_EventHandler);
	// 将交互控件添加到组
	lv_group_add_obj(this->_lvgl_group, this->_lvgl_menu);
	// 设置组的焦点到交互控件
	lv_group_focus_obj(this->_lvgl_menu);
	this->_items_str = "";
	// Serial.printf("Item_str Length: %d\n", this->_items_str.length());
}

void RollerMenuScreenPage::init() {
}

void RollerMenuScreenPage::dispose() {
}

void RollerMenuScreenPage::enterPage(lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE) {
	DDSInteractManager->setCurrent(this);
	lv_scr_load(this->lvgl_GetScreen());
	// // 使用带动画的方式切换到 new_scr
	// lv_scr_load_anim(
	// 	this->lvgl_GetScreen(), 
	// 	anim,						// 动画类型: 旧屏幕从右往左滑动出去, 新屏幕从右往左滑入
	// 	500,						// 动画时长 500ms
	// 	0,							// 无延时
	// 	false						// 动画结束后不自动删除旧screen
	// );
	lv_roller_set_selected(this->_lvgl_menu, this->_last_selected, LV_ANIM_OFF);
}

void RollerMenuScreenPage::exitPage(lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE) {
}

void RollerMenuScreenPage::addItem(ScreenPage *ItemPage) {
	this->addChild(ItemPage);

	std::string item_name = ItemPage->getName();
	assert(item_name.length() > 0);
	if (this->_items_str.length() > 0)
		item_name = "\n" + item_name;
	this->_items_str += item_name;

	lv_roller_set_options(this->_lvgl_menu,
		this->_items_str.c_str(), LV_ROLLER_MODE_INFINITE);
	
	lv_roller_set_selected(this->_lvgl_menu, 0, LV_ANIM_OFF);
	this->setLastSelected(0);

	// Serial.printf("Item_str Length: %d\n", this->_items_str.length());
}