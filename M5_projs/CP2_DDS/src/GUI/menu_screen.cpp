#include "menu_screen.hpp"
#include "InteractManager.hpp"


void MenuScreenPage::init() {
}

void MenuScreenPage::dispose() {
}

void MenuScreenPage::enterPage() {
}

void MenuScreenPage::exitPage() {
}



static void event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
	if(code == LV_EVENT_VALUE_CHANGED) {
		char buf[32];
		int index = lv_roller_get_selected(obj);
		lv_roller_get_selected_str(obj, buf, sizeof(buf));
		LV_LOG_USER("Selected value: %s", buf);
	}
}

lv_style_t *createDefualtMenuScreenStyle() {
	// 创建一个新的样式
	static lv_style_t style;
	lv_style_init(&style);
	// 设置背景颜色
	lv_style_set_bg_color(&style, lv_color_hex(0x000000));
	lv_style_set_bg_opa(&style, LV_OPA_COVER); // 完全不透明
	return &style;
}


lv_obj_t *createSettingsScreen(std::string name) {
	lv_obj_t *ScreenObj = lv_obj_create(NULL);

	/*A style to make the selected option larger*/
	static lv_style_t style_sel;
	lv_style_init(&style_sel);
	lv_style_set_text_font(&style_sel, &lv_font_montserrat_22);
	lv_style_set_bg_color(&style_sel, lv_color_hex3(0xf88));
	lv_style_set_border_width(&style_sel, 2);
	lv_style_set_border_color(&style_sel, lv_color_hex3(0xf00));

	const char *opts =
		"Wave\n"
		"Others"
		;
	lv_obj_t *roller = lv_roller_create(ScreenObj);
	lv_roller_set_options(roller,
			opts, LV_ROLLER_MODE_INFINITE);

	lv_obj_set_width(roller, lv_pct(60));
	lv_roller_set_visible_row_count(roller, 3);
	lv_obj_center(roller);
	lv_obj_add_event_cb(roller, event_handler, LV_EVENT_VALUE_CHANGED, NULL);

	return roller;
}

lv_obj_t *createWaveSettingsScreen(std::string name) {
	lv_obj_t *ScreenObj = lv_obj_create(NULL);
	// 创建一个新的样式
	static lv_style_t style_bg;
	lv_style_init(&style_bg);
	// 设置背景颜色
	lv_style_set_bg_color(&style_bg, lv_color_hex(0x000000));
	lv_style_set_bg_opa(&style_bg, LV_OPA_COVER); // 完全不透明
	// 应用样式到屏幕对象
	lv_obj_add_style(ScreenObj, &style_bg, LV_PART_MAIN);

	/*A style to make the selected option larger*/
	static lv_style_t style_sel;
	lv_style_init(&style_sel);
	lv_style_set_text_font(&style_sel, &lv_font_montserrat_22);
	lv_style_set_bg_color(&style_sel, lv_color_hex3(0xf88));
	lv_style_set_border_width(&style_sel, 2);
	lv_style_set_border_color(&style_sel, lv_color_hex3(0xf00));

	const char *opts =
		"WaveForm\n"
		"Frequency\n"
		"Phase"
		;
	lv_obj_t *roller = lv_roller_create(ScreenObj);
	lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);

	lv_obj_set_width(roller, lv_pct(60));
	lv_roller_set_visible_row_count(roller, 3);
	lv_obj_center(roller);
	lv_obj_add_event_cb(roller, event_handler, LV_EVENT_VALUE_CHANGED, NULL);

	return roller;
}

lv_obj_t *scrollerMenuAddItem(lv_obj_t *roller, const char *itemName) {
	char *opts = (char *)lv_roller_get_options(roller);
	opts = strcat(opts, itemName);
	lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
	return roller;
}

lv_obj_t *__createScrollerMenu(std::string name, lv_style_t *style = nullptr) {
	lv_obj_t *ScreenObj = lv_obj_create(NULL);

	// 应用样式到屏幕对象
	if (style == nullptr)
		style = createDefualtMenuScreenStyle();
	lv_obj_add_style(ScreenObj, style, LV_PART_MAIN);

	/*A style to make the selected option larger*/
	static lv_style_t style_sel;
	lv_style_init(&style_sel);
	lv_style_set_text_font(&style_sel, &lv_font_montserrat_22);
	lv_style_set_bg_color(&style_sel, lv_color_hex3(0xf88));
	lv_style_set_border_width(&style_sel, 2);
	lv_style_set_border_color(&style_sel, lv_color_hex3(0xf00));

	lv_obj_t *roller = lv_roller_create(ScreenObj);
	lv_roller_set_options(roller,
			"", LV_ROLLER_MODE_INFINITE);

	lv_obj_set_width(roller, lv_pct(60));
	lv_roller_set_visible_row_count(roller, 3);
	lv_obj_center(roller);
	lv_obj_add_event_cb(roller, event_handler, LV_EVENT_VALUE_CHANGED, NULL);

	return roller;
}

MenuScreenPage *createScrollerMenu(std::string name, MenuScreenPage *parent) {
	MenuScreenPage *screen = new MenuScreenPage(name, parent);
	// screen->lvgl_event_receiver = __createScrollerMenu(name);
	// screen->lvgl_container = lv_obj_get_parent(screen->lvgl_event_receiver);

	if (parent != nullptr) {
		parent->addChild(screen);
	}
	return screen;
}