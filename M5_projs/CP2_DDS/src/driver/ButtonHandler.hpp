#pragma once

// ButtonHandler.h
#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "ButtonReader.hpp"

#include "includes.h"

#include "modules/includes.h"


	/**
	 * @brief ButtonHandler 类，用于处理按钮事件。
	 */
	class ButtonHandler {
	public:
		/**
		 * @brief 定义按钮事件回调类型。
		 */
		using ButtonCallback = std::function<void(const ButtonEvent&)>;

		/**
		 * @brief 构造函数。
		 * 
		 * @param reader                共享指针，指向 ButtonReader 实例。
		 * @param longPressDurationMs  长按持续时间（毫秒）。
		 * @param multiclickIntervalMs  多击间隔时间（毫秒）。
		 */
		ButtonHandler(std::shared_ptr<ButtonReader> reader, 
					unsigned long longPressDurationMs = JOYSTICK_LONG_PRESS_DURATION_MS, 
					unsigned long multiclickIntervalMs = JOYSTICK_MULTICLICK_INTERVAL_MS);

		/**
		 * @brief 设置长按持续时间。
		 */
		void setLongPressDuration(unsigned long durationMs);
		
		/**
		 * @brief 设置多击间隔时间。
		 */
		void setMulticlickInterval(unsigned long intervalMs);
		
		/**
		 * @brief 设置轮询间隔时间。
		 */
		void setPollingInterval(unsigned long intervalMs);
		
		/**
		 * @brief 获取轮询间隔时间。
		 */
		unsigned long getPollingInterval() const { return pollingIntervalMs; }
		
		/**
		 * @brief 注册按钮事件回调。
		 */
		void attachCallback(ButtonCallback cb);
		
		/**
		 * @brief 启用或禁用防抖功能。
		 */
		void enableDebounce(bool enable);
		
		/**
		 * @brief 定期调用此方法以更新按钮状态。
		 */
		void update();

	private:
		std::shared_ptr<ButtonReader> reader;

		unsigned long longPressDurationMs;
		unsigned long multiclickIntervalMs;
		unsigned long pollingIntervalMs;

		ButtonCallback callback;

		// 内部状态机状态
		enum class State {
			IDLE,
			PRESSED,
			WAITING_FOR_MULTICLICK,
			LONG_PRESSED
		};

		struct ButtonState {
			State state;
			TickType_t pressStartTick;
			TickType_t lastClickTick;
			unsigned int clickCount;
		};
		
		ButtonState buttons[JOYSTICK_MAX_BUTTONS];
		uint8_t buttonCount;

		bool debounceEnabled;
		TickType_t lastUpdateTick;

		// 辅助方法
		void handleButtonState(uint8_t buttonId, bool pressed, TickType_t currentTick);
	};

#endif // BUTTON_HANDLER_H