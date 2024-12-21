#pragma once

#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "IEventJudge.hpp"
#include "internal.h"


	// 使用宏定义来避免魔法数字
	#define EVENT_POLL_INTERVAL_MS	5
	#define EVENT_TASK_PRIORITY		5


	class EventManager {
		public:
			// 注册事件判断器
			void registerJudge(std::shared_ptr<IEventJudge> judge) {
				judges.push_back(judge);
			}

			// 由轮询线程周期调用
			void updateAll() {
				for (auto &j : judges) {
					bool triggered = j->checkEvent();
					// 这里可以做一些统计或日志
				}
			}

		private:
			std::vector<std::shared_ptr<IEventJudge>> judges;
	};


	extern EventManager eventMgr;
	void initEventPollTask(void);

#endif // MODULE_MANAGER_H