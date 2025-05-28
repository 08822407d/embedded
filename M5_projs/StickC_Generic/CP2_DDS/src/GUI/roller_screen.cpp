#include "roller_screen.hpp"

#include "init.hpp"


static void Roller_EventHandler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
	if(code == LV_EVENT_KEY) {
		lv_key_t key = static_cast<lv_key_t>(lv_event_get_key(e));
		int index = lv_roller_get_selected(obj);
		// Serial.printf("Key: %d ; Selected value: %d\n", key, index);


		RollerScreenPage *currPage = dynamic_cast<RollerScreenPage *>(DDSInteractManager->getCurrent());
		currPage->setLastSelected(index);
		// Serial.printf("Children count: %d\n", currPage->getChildren().size());
		ScreenPage *selectedPage = nullptr;
		lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE;
        if (key == LV_KEY_PREV) {
			selectedPage = currPage->getParent();
            if (selectedPage == nullptr)
                return;
			anim = LV_SCR_LOAD_ANIM_OVER_RIGHT;
			// Serial.printf("Selected Page: %s\n", selectedPage->getName().c_str());
			currPage->exitPage(anim);
			selectedPage->enterPage(anim);
		}
	}
}


RollerScreenPage::RollerScreenPage(
		std::string name, 
		ScreenPage *parent, 
		lv_style_t *style)
:	ScreenPage(name, parent, MenuKeyMap) {
	this->_lvgl_roller = createRoller(this->_lvgl_container, style, Roller_EventHandler);
	// 将交互控件添加到组
	lv_group_add_obj(this->_lvgl_group, this->_lvgl_roller);
	// 设置组的焦点到交互控件
	lv_group_focus_obj(this->_lvgl_roller);
	this->_items_str = "";
	// Serial.printf("Item_str Length: %d\n", this->_items_str.length());
}

void RollerScreenPage::init() {
}

void RollerScreenPage::dispose() {
}

void RollerScreenPage::enterPage(lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE) {
	if (this->_data_reciever != nullptr)
		lv_roller_set_selected(this->_lvgl_roller, *this->_data_reciever, LV_ANIM_OFF);

	DDSInteractManager->setCurrent(this);
	lv_scr_load(this->lvgl_GetScreen());
	// 使用带动画的方式切换到 new_scr
	lv_scr_load_anim(
		this->lvgl_GetScreen(), 
		anim,						// 动画类型: 旧屏幕从右往左滑动出去, 新屏幕从右往左滑入
		500,						// 动画时长 500ms
		0,							// 无延时
		false						// 动画结束后不自动删除旧screen
	);
}

void RollerScreenPage::exitPage(lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE) {
	if (this->_data_reciever != nullptr)
		*this->_data_reciever = this->_last_selected;

	ddsPtr->start();
	ddsPtr->setWave(globalDDSparams.WaveForm,
		globalDDSparams.Frequency,
		globalDDSparams.Phase);
	delay(100);
	joystickPtr->start();
}

void RollerScreenPage::addItem(std::string item_name) {
	assert(item_name.length() > 0);
	if (this->_items_str.length() > 0)
		item_name = "\n" + item_name;
	this->_items_str += item_name;

	lv_roller_set_options(this->_lvgl_roller,
		this->_items_str.c_str(), LV_ROLLER_MODE_INFINITE);
	
	lv_roller_set_selected(this->_lvgl_roller, 0, LV_ANIM_OFF);
	this->setLastSelected(0);

	// Serial.printf("Item_str Length: %d\n", this->_items_str.length());
}