#pragma once

#ifndef _MODULE_MANAGER_H_
#define _MODULE_MANAGER_H_

#include "device/GenericModule.hpp"
#include "internal.h"


	// 使用宏定义来避免魔法数字
	#define EVENT_POLL_INTERVAL_MS	5
	#define EVENT_TASK_PRIORITY		3


	class ModuleManager {
	public:
		/**
		 * @brief 注册一个外设模块(子类实例)。
		 * @param mod 指向派生自 IDeviceModule 的共享指针。
		 */
		void registerModule(std::shared_ptr<IDeviceModule> mod) {
			_modules.push_back(mod);
		}

		/**
		 * @brief 轮询或更新所有已注册的模块。
		 */
		void updateAll() {
			for (auto &m : _modules) {
				bool ok = m->update();
				// 可选：根据 ok 做一些日志或错误处理
				// if (!ok) { Serial.printf("Module %s update failed\n", m->getName()); }
			}
		}

	private:
		std::vector<std::shared_ptr<IDeviceModule>> _modules;
	};


	extern ModuleManager DevModManager;
	void initModulePollTask(void);

#endif /* _MODULE_MANAGER_H_ */