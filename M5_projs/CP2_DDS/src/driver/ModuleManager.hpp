#pragma once

// ModuleManager.h
#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "IModule.hpp"

#include "internal.h"


	// 使用宏定义来避免魔法数字
	#define MODULE_POLL_INTERVAL_MS   5   // 每隔5ms轮询一次
	#define MODULE_TASK_PRIORITY      5   // 优先级设为5(示例)，以保证2ms左右的唤醒响应


	class ModuleManager {
	public:
		void registerModule(std::shared_ptr<IModule> mod) {
			modules.push_back(mod);
		}

		void updateAll() {
			for (auto &m : modules) {
				bool ok = m->update(); 
				// 需要的话这里可以记录/打印失败信息
			}
		}

	private:
		std::vector<std::shared_ptr<IModule>> modules;
	};


	void initModulePollingTask(void);

#endif // MODULE_MANAGER_H