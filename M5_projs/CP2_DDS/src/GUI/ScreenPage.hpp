#pragma once

#include <vector>
#include <memory>
#include <string>

#include <lvgl.h>


class ScreenPage {
public:
	explicit ScreenPage(const std::string& name,
		ScreenPage *parent = nullptr,
		lv_key_t *keyMap = nullptr)
	: _name(name), _parent(parent), _keyMap(keyMap) {}


	~ScreenPage() {}

	std::string getName() const { return _name; }

	// 1) 生命周期：由子类去实现
	virtual void init() = 0;		// 在系统初始化阶段调用
	virtual void dispose() = 0;		// 资源或内存回收
	virtual void enterPage() = 0;	// 当切换进本页面
	virtual void exitPage() = 0;	// 当切换离开本页面

	// 3) 子页面树的管理
	void addChild(ScreenPage *child);
	const std::vector<ScreenPage *>& getChildren() const;
	ScreenPage *getParent() const;

protected:
	std::string		_name;
	ScreenPage		*_parent{nullptr};
	std::vector<ScreenPage *>	_children;
	lv_key_t		*_keyMap;
};
