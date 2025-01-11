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
	static uint32_t		WaveForm;
	static uint32_t		Frequency;
	static uint32_t		Phase;

	// 获取单例实例的接口，返回引用或指针都可以，这里演示返回引用
	static DDSparams& getInstance();

private:
	// 私有化构造函数，禁止从外部实例化
	DDSparams() = default;

	// 禁用拷贝构造和复制赋值运算符
	DDSparams(const DDSparams&) = delete;
	DDSparams& operator=(const DDSparams&) = delete;

	// 唯一静态实例指针
	static DDSparams		*instance;
};


extern DDSparams& globalDDSparams;