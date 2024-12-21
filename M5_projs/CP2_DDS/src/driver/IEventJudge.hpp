#pragma once

#ifndef I_EVENT_JUDGE_H
#define I_EVENT_JUDGE_H


	/**
	 * @brief 抽象基类，用于在轮询时判断是否需要触发特定事件
	 */
	class IEventJudge {
	public:
		virtual ~IEventJudge() {}

		/**
		 * @brief 在轮询线程中被调用，用于判断是否需触发事件
		 * @return true 表示事件触发(或者已设置事件flag)，false表示未触发
		 */
		virtual bool checkEvent() = 0;

		/**
		 * @brief 返回此事件判断器的名称或类型(用于调试)
		 */
		virtual const char* getName() const = 0;
	};

#endif // I_EVENT_JUDGE_H