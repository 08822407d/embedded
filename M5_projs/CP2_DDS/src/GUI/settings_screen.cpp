#include "settings_screen.hpp"


lv_obj_t *SettingsPanel;


// lv_obj_t *createSettingsScreen() {
// 	SettingsPanel = lv_obj_create(lv_screen_active());
// 	lv_obj_set_size(SettingsPanel, LV_HOR_RES, LV_VER_RES);
// 	lv_obj_set_scroll_snap_y(SettingsPanel, LV_SCROLL_SNAP_CENTER);
// 	lv_obj_set_flex_flow(SettingsPanel, LV_FLEX_FLOW_COLUMN_WRAP);
// 	// lv_obj_align(SettingsPanel, LV_ALIGN_CENTER, 0, 20);
// }