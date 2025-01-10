#pragma once

#include <iostream>
#include <array>
#include <memory>
#include <mutex>


enum WaveForm {
	SINE,
	SQUARE,
	TRIANGLE,
	SAWTOOTH,
};

class DDSparams {
public:
	// 获取单例实例的接口，返回引用或指针都可以，这里演示返回引用
	static DDSparams& getInstance();

	// 下面是对三个私有静态成员的访问接口
	unsigned int getFrequency() const;
	unsigned int getPhase() const;
	WaveForm getWaveForm() const;
	void setFrequency(unsigned int freq);
	void setPhase(unsigned int ph);
	void setWaveForm(WaveForm form);


private:
	// 私有化构造函数，禁止从外部实例化
	DDSparams() = default;

	// 禁用拷贝构造和复制赋值运算符
	DDSparams(const DDSparams&) = delete;
	DDSparams& operator=(const DDSparams&) = delete;

	// 唯一静态实例指针
	static DDSparams		*instance;


	static unsigned int		_frequency;
	static unsigned int		_phase;
	static WaveForm			_waveForm;
};


extern DDSparams& globalDDSparams;