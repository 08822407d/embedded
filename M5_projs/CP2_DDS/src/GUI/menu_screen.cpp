#include "menu_screen.hpp"
#include "display.hpp"


lv_obj_t *MenuPanel;


lv_obj_t *createMenuItems(lv_obj_t *parent, std::shared_ptr<ScreenPage> page) {
	lv_obj_t *btn = lv_button_create(parent);
	lv_obj_set_size(btn, lv_pct(80), lv_pct(50));

	lv_obj_t *label = lv_label_create(btn);
	lv_label_set_text_fmt(label, page->getName().c_str());

	lv_obj_center(label);

	return btn;
}

lv_obj_t *createMenuScreen() {
	MenuPanel = lv_obj_create(lv_screen_active());
	lv_obj_set_size(MenuPanel, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_scroll_snap_y(MenuPanel, LV_SCROLL_SNAP_CENTER);
	lv_obj_set_flex_flow(MenuPanel, LV_FLEX_FLOW_COLUMN);
	lv_obj_align(MenuPanel, LV_ALIGN_CENTER, 0, 0);

	lv_obj_update_snap(MenuPanel, LV_ANIM_ON);

	return MenuPanel;
}

void initMenuScreens(std::shared_ptr<ScreenPage> RootPage) {
	RootPage->lvgl_widget = createMenuScreen();
	for (auto &child : RootPage->getChildren()) {
		child->lvgl_widget = createMenuItems(RootPage->lvgl_widget, child);
	}


	/*Update the buttons position manually for first*/
	lv_obj_send_event(RootPage->lvgl_widget, LV_EVENT_SCROLL, NULL);

	/*Be sure the fist button is in the middle*/
	lv_obj_scroll_to_view(lv_obj_get_child(RootPage->lvgl_widget, 0), LV_ANIM_OFF);
}