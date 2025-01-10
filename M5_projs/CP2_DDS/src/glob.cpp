#include "glob.hpp"

// 静态成员变量的定义与初始化
DDSparams    *DDSparams::instance = nullptr;

// 这三个私有静态成员需要在类外定义
unsigned int DDSparams::_frequency = 1000;
unsigned int DDSparams::_phase     = 0;
WaveForm     DDSparams::_waveForm  = WaveForm::SINE;

// 单例实例获取函数
DDSparams& DDSparams::getInstance() {
	// 如果还没有创建实例，就创建
	if (!instance)
		instance = new DDSparams();

	// 返回静态对象的引用
	return *instance;
}

// 以下是三个成员变量的 Getter
unsigned int DDSparams::getFrequency() const {
	return _frequency;
}

unsigned int DDSparams::getPhase() const {
	return _phase;
}

WaveForm DDSparams::getWaveForm() const {
	return _waveForm;
}

// 以下是三个成员变量的 Setter
void DDSparams::setFrequency(unsigned int freq) {
	_frequency = freq;
}

void DDSparams::setPhase(unsigned int ph) {
	_phase = ph;
}

void DDSparams::setWaveForm(WaveForm form) {
	_waveForm = form;
}

// 定义全局变量（或引用）g_singleton
// 这里的定义与头文件中的声明 extern MySingleton& g_singleton; 对应
DDSparams& g_params = DDSparams::getInstance();