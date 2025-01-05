#include "menu_screen.hpp"
#include "InteractManager.hpp"


void RollerMenuScreenPage::init() {
}

void RollerMenuScreenPage::dispose() {
}

void RollerMenuScreenPage::enterPage() {
}

void RollerMenuScreenPage::exitPage() {
}



static void event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
	if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        // const char * key_name = lv_key_to_name(key);
		int index = lv_roller_get_selected(obj);
		Serial.printf("Key: %d ; Selected value: %d\n", key, index);
	}
}


static lv_style_t *createDefualtRollerScreenStyle() {
	/*A style to make the selected option larger*/
	static lv_style_t style_sel;
	lv_style_init(&style_sel);
	lv_style_set_text_font(&style_sel, &lv_font_montserrat_22);
	lv_style_set_bg_color(&style_sel, lv_color_hex3(0xf88));
	lv_style_set_border_width(&style_sel, 2);
	lv_style_set_border_color(&style_sel, lv_color_hex3(0xf00));
	return &style_sel;
}

RollerMenuScreenPage::RollerMenuScreenPage(std::string name,
		ScreenPage *parent, lv_style_t *style)
: MenuScreenPage(name, parent)
{
	this->_lvgl_menu = __createRollerMenu(name, style);
	// 将默认滚轮添加到组
	lv_group_add_obj(this->_lvgl_group, this->_lvgl_menu);
	// 设置组的焦点到默认按钮
	lv_group_focus_obj(this->_lvgl_menu);
	this->_items_str = "";
	// Serial.printf("Item_str Length: %d\n", this->_items_str.length());
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

	Serial.printf("Item_str Length: %d\n", this->_items_str.length());
}

lv_obj_t *RollerMenuScreenPage::__createRollerMenu(std::string name, lv_style_t *style) {
	lv_obj_t *roller = lv_roller_create(this->_lvgl_container);
	lv_roller_set_options(roller,
			"", LV_ROLLER_MODE_INFINITE);

	lv_obj_set_width(roller, lv_pct(60));
	if (style != nullptr)
		lv_obj_add_style(roller, style, LV_PART_SELECTED);
	lv_roller_set_visible_row_count(roller, 3);
	lv_obj_center(roller);
	lv_obj_add_event_cb(roller, event_handler, LV_EVENT_KEY, NULL);

	return roller;
}