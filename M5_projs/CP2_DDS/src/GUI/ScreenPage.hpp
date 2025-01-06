#pragma once

#include <vector>
#include <memory>
#include <string>

#include <lvgl.h>


typedef lv_obj_t *lvgl_CreateScreenFunc_t(std::string name, lv_style_t *style);
extern lvgl_CreateScreenFunc_t __createScreen;


class ScreenPage {
public:
	lv_key_t		*KeyMap;

	explicit ScreenPage(const std::string& name,
			ScreenPage *parent = nullptr,
			lv_key_t *keyMap = nullptr)
	:	_name(name), _parent(parent), KeyMap(keyMap) {
		this->_lvgl_container = __createScreen(name, nullptr);
		this->_lvgl_group = lv_group_create();
	}
	~ScreenPage() {
		lv_group_del(this->_lvgl_group);
	}

	std::string getName() const { return _name; }

	// 1) 生命周期：由子类去实现
	virtual void init() = 0;		// 在系统初始化阶段调用
	virtual void dispose() = 0;		// 资源或内存回收
	virtual void enterPage(lv_screen_load_anim_t anim) = 0;	// 当切换进本页面
	virtual void exitPage(lv_screen_load_anim_t anim) = 0;	// 当切换离开本页面

	// 2) 接收事件
	void lvgl_RecieveKeyEvent(lv_key_t *key);

	// 3) 子页面树的管理
	void addChild(ScreenPage *child);
	ScreenPage *getChild(int index) const;
	const std::vector<ScreenPage *>& getChildren() const;
	ScreenPage *getParent() const;

	void addToGroup(lv_obj_t *obj) {
		lv_group_add_obj(this->_lvgl_group, obj);
	}

	lv_obj_t *lvgl_GetScreen() const { return this->_lvgl_container; }


protected:
	std::string		_name;
	ScreenPage		*_parent{nullptr};
	std::vector<ScreenPage *>	_children;

	lv_obj_t		*_lvgl_container;
	lv_group_t		*_lvgl_group;


	virtual void __lvgl_KeyEventSpecial(lv_key_t *key);
};
