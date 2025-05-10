#pragma once

#ifndef _GENERIC_MODULE_HPP_
#define _GENERIC_MODULE_HPP_


	class IDeviceModule {
	private:

	public:
		virtual bool update() = 0;

		/**
		 * @brief 返回模块名称或标识(可用于调试或日志输出)。
		 */
		virtual const char* getName() const = 0;
	};

#endif /* _GENERIC_MODULE_HPP_ */