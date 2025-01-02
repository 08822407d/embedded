#include "menu_screen.hpp"


lv_obj_t *MenuPanel;


lv_obj_t *createMenuItems() {
	lv_obj_t * btn = lv_button_create(MenuPanel);
	lv_obj_set_size(btn, lv_pct(50), lv_pct(100));

	lv_obj_t * label = lv_label_create(btn);
	lv_label_set_text_fmt(label, "Panel");

	lv_obj_center(label);
}

lv_obj_t *createMenuScreen() {
	MenuPanel = lv_obj_create(lv_screen_active());
	lv_obj_set_size(MenuPanel, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_scroll_snap_y(MenuPanel, LV_SCROLL_SNAP_CENTER);
	lv_obj_set_flex_flow(MenuPanel, LV_FLEX_FLOW_COLUMN_WRAP);
	// lv_obj_align(MenuPanel, LV_ALIGN_CENTER, 0, 20);
}