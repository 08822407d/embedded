#pragma once

// IModule.h
#ifndef IMODULE_H
#define IMODULE_H


	/**
	 * @brief IModule 为所有外设模组的抽象基类。
	 */
	class IModule {
	public:
		virtual ~IModule() {}

		/**
		 * @brief 在一次轮询中完成: 硬件读写 + 数据解析 + 事件判断 + 回调触发。
		 * @return true 表示本次更新成功，false 表示更新失败(读失败等)。
		 */
		virtual bool update() = 0;

		/**
		 * @brief 可选: 返回模块名称或标识。
		 */
		virtual const char* getName() const = 0;
	};

#endif // IMODULE_H