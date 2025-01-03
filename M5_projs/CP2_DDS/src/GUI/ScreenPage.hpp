#pragma once

#include <vector>
#include <memory>
#include <string>

#include <lvgl.h>


class ScreenPage {
public:
	lv_obj_t	*lvgl_widget { nullptr };


	explicit ScreenPage(const std::string& name)
	: _name(name) {}

	~ScreenPage() {}

	std::string getName() const { return _name; }

	// 1) 生命周期：由子类去实现
	void init();		// 在系统初始化阶段调用
	void dispose();		// 资源或内存回收
	void enterPage();	// 当切换进本页面
	void exitPage();	// 当切换离开本页面

	// 3) 子页面树的管理
	void addChild(std::shared_ptr<ScreenPage> child);
	const std::vector<std::shared_ptr<ScreenPage>>& getChildren() const;
	ScreenPage *getParent() const;

protected:
	std::string		_name;
	ScreenPage		*_parent{nullptr};
	std::vector<std::shared_ptr<ScreenPage>>	_children;
};
