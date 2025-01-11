#include "glob.hpp"

// 静态成员变量的定义与初始化
DDSparams    *DDSparams::instance	= nullptr;

// 这三个私有静态成员需要在类外定义
uint32_t DDSparams::Frequency	= 1000;
uint32_t DDSparams::Phase		= 0;
uint32_t DDSparams::WaveForm	= 0;

// 单例实例获取函数
DDSparams& DDSparams::getInstance() {
	// 如果还没有创建实例，就创建
	if (!instance)
		instance = new DDSparams();

	// 返回静态对象的引用
	return *instance;
}