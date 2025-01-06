#include "ScreenPage.hpp"


static lv_style_t *createDefualtMenuScreenStyle() {
	// 创建一个新的样式
	static lv_style_t style;
	lv_style_init(&style);
	// 设置背景颜色
	lv_style_set_bg_color(&style, lv_color_hex(0x000000));
	lv_style_set_bg_opa(&style, LV_OPA_COVER); // 完全不透明
	return &style;
}

lv_obj_t *__createScreen(std::string name, lv_style_t *style = nullptr) {
	lv_obj_t *ScreenObj = lv_obj_create(NULL);

	// 应用样式到屏幕对象
	if (style == nullptr)
		style = createDefualtMenuScreenStyle();
	lv_obj_add_style(ScreenObj, style, LV_PART_MAIN);

	return ScreenObj;
}


void ScreenPage::addChild(ScreenPage *child) {
	child->_parent = this;
	this->_children.push_back(child);
}

ScreenPage *ScreenPage::getChild(int index) const {
	if (index < 0 || this->_children.size() <= 0 || index >= this->_children.size()) 
		return nullptr;
	else
		return this->_children[index];
}

const std::vector<ScreenPage *>& ScreenPage::getChildren() const {
	return this->_children;
}

ScreenPage* ScreenPage::getParent() const {
	return this->_parent;
}