#pragma once

// JoystickToButton.h
#ifndef JOYSTICK_TO_BUTTON_H
#define JOYSTICK_TO_BUTTON_H

#include <functional>
#include <memory>
#include <vector>
#include <map>
#include "Joystick.hpp"
#include "modules/Events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


	/**
	 * @brief JoystickToButton 类，用于将摇杆方向事件转换为虚拟按钮事件。
	 */
	class JoystickToButton {
	public:
		/**
		 * @brief 定义虚拟按钮事件回调类型。
		 */
		using VirtualButtonCallback = std::function<void(const ButtonEvent&)>;

		/**
		 * @brief 构造函数。
		 * 
		 * @param joystick       共享指针，指向 Joystick 实例。
		 * @param directions     要转换为虚拟按钮的方向列表。
		 * @param longPressDurationMs 长按持续时间（毫秒）。
		 * @param multiclickIntervalMs 多击间隔时间（毫秒）。
		 */
		JoystickToButton(std::shared_ptr<Joystick> joystick, 
						const std::vector<Direction>& directions,
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
		 * @brief 注册虚拟按钮事件回调。
		 */
		void attachCallback(VirtualButtonCallback cb);
		
		/**
		 * @brief 启用或禁用防抖功能。
		 */
		void enableDebounce(bool enable);
		
		/**
		 * @brief 定期调用此方法以更新虚拟按钮状态。
		 */
		void update();

	private:
		std::shared_ptr<Joystick> joystick;
		std::vector<Direction> directions; // 要转换为按钮的方向
		unsigned long longPressDurationMs;
		unsigned long multiclickIntervalMs;
		unsigned long pollingIntervalMs;

		VirtualButtonCallback callback;
		
		// 内部状态机状态
		enum class State {
			IDLE,
			PRESSED,
			WAITING_FOR_MULTICLICK,
			LONG_PRESSED
		};

		struct VirtualButtonState {
			State state;
			TickType_t pressStartTick;
			TickType_t lastClickTick;
			unsigned int clickCount;
		};
		
		// 每个方向对应一个虚拟按钮状态
		std::map<Direction, VirtualButtonState> buttonStates;
		size_t directionCount;

		bool debounceEnabled;
		TickType_t lastUpdateTick;

		// 辅助方法
		void handleJoystickEvent(const JoystickEvent& event);

		// 辅助方法：触发按钮事件
		void triggerButtonEvent(EventType type, uint8_t buttonId);
	};

#endif // JOYSTICK_TO_BUTTON_H