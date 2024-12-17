#pragma once

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <Arduino.h>
#include <functional>
#include <memory>
#include "JoystickReader.hpp"
#include "JoystickConfig.h"

	// 定义事件来源枚举
	enum class EventSource {
		AXIS,
		BUTTON
	};

	// 定义方向枚举
	enum class Direction {
		CENTER = 0,
		UP,
		DOWN,
		LEFT,
		RIGHT
	};

	// 定义事件类型枚举
	enum class EventType {
		SINGLE_CLICK,
		MULTICLICK,
		LONG_PRESS
	};

	// 定义Joystick事件结构体
	struct JoystickEvent {
		EventSource source; // AXIS 或 BUTTON
		union {
			Direction direction; // 如果来源是AXIS
			uint8_t buttonId;    // 如果来源是BUTTON
		};
		EventType type;
	};

	// 定义回调函数类型
	using JoystickCallback = std::function<void(const JoystickEvent&)>;

	template <typename T>
	class Joystick {
	public:
		int x;
		int y;

		Joystick(std::shared_ptr<JoystickReader<T>> reader, 
				unsigned char thresholdPercent = 20,
				unsigned long longPressDurationMs = JOYSTICK_LONG_PRESS_DURATION_MS, 
				unsigned long multiclickIntervalMs = JOYSTICK_MULTICLICK_INTERVAL_MS);
		
		void setThresholdPercent(unsigned char thresholdPercent);
		void setLongPressDuration(unsigned long durationMs);
		void setMulticlickInterval(unsigned long intervalMs);
		void setPollingInterval(unsigned long intervalMs);
		unsigned long getPollingInterval() const { return pollingIntervalMs; }

		void attachCallback(JoystickCallback cb);
		void calibrate(int centerX, int centerY);

		/**
		 * @brief 启用或禁用防抖功能
		 * @param enable true为启用防抖，false为禁用防抖
		 */
		void enableDebounce(bool enable);

		/**
		 * @brief 定期调用此方法以更新状态
		 */
		void update();

	private:
		std::shared_ptr<JoystickReader<T>> reader;
		
		unsigned char thresholdPercent;
		unsigned long longPressDurationMs;
		unsigned long multiclickIntervalMs;
		unsigned long pollingIntervalMs;

		JoystickCallback callback;

		enum class State {
			IDLE,
			PRESSED,
			WAITING_FOR_MULTICLICK,
			LONG_PRESSED
		} state;
		
		Direction currentDirection;
		Direction pressedDirection;

		// 时间记录（都使用 ticks）
		TickType_t pressStartTick;
		TickType_t lastClickTick;
		unsigned int clickCount;

		TickType_t lastUpdateTick;

		int calibratedCenterX;
		int calibratedCenterY;

		struct ButtonState {
			State state;
			TickType_t pressStartTick;
			TickType_t lastClickTick;
			unsigned int clickCount;
		};
		
		ButtonState buttons[JOYSTICK_MAX_BUTTONS];
		uint8_t buttonCount;

		bool debounceEnabled; // 防抖启用标志

		void detectDirection(int x, int y, Direction &detected);
		void handleAxisState(Direction detected, TickType_t currentTick);
		void handleButtonState(uint8_t buttonId, bool pressed, TickType_t currentTick);

		int getThresholdValue(int minVal, int maxVal) const;
	};

#include "Joystick.tpp"

#endif /* _JOYSTICK_H_ */