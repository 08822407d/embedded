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

	/**
	 * @brief Joystick类，用于处理摇杆和多个按钮的事件检测。
	 * 
	 * @tparam T 数据类型，通常为 int。
	 */
	template <typename T>
	class Joystick {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param reader                共享指针，指向JoystickReader实例
		 * @param thresholdPercent      阈值百分比（0-100）
		 * @param longPressDuration     长按持续时间（毫秒）
		 * @param multiclickInterval    多击间隔时间（毫秒）
		 */
		Joystick(std::shared_ptr<JoystickReader<T>> reader, 
				unsigned char thresholdPercent = 20, // 阈值百分比
				unsigned long longPressDuration = JOYSTICK_LONG_PRESS_DURATION_MS, 
				unsigned long multiclickInterval = JOYSTICK_MULTICLICK_INTERVAL_MS);
		
		// 设置阈值百分比
		void setThresholdPercent(unsigned char thresholdPercent);
		
		// 设置长按持续时间
		void setLongPressDuration(unsigned long duration);
		
		// 设置多击间隔
		void setMulticlickInterval(unsigned long interval);
		
		// 设置轮询间隔
		void setPollingInterval(unsigned long interval);
		
		// 获取轮询间隔
		unsigned long getPollingInterval() const { return pollingInterval; }
		
		// 注册事件回调
		void attachCallback(JoystickCallback cb);
		
		// 校正摇杆中心位置
		void calibrate(int centerX, int centerY);
		
		/**
		 * @brief 定期调用此方法以更新状态，传入当前时间（例如millis()）
		 * 
		 * @param currentMillis 当前时间，通常为millis()
		 */
		void update(TickType_t currentMillis);
		
	private:
		// 数据读取接口
		std::shared_ptr<JoystickReader<T>> reader;
		
		// 阈值百分比（0-100）
		unsigned char thresholdPercent;
		
		// 长按持续时间（毫秒）
		unsigned long longPressDuration;
		
		// 多击间隔（毫秒）
		unsigned long multiclickInterval;
		
		// 轮询间隔（毫秒）
		volatile unsigned long pollingInterval;
		
		// 事件回调
		JoystickCallback callback;
		
		// 内部状态机状态
		enum class State {
			IDLE,
			PRESSED,
			WAITING_FOR_MULTICLICK,
			LONG_PRESSED
		} state;
		
		// 当前方向和按下方向
		Direction currentDirection;
		Direction pressedDirection;
		
		// 时间记录
		TickType_t pressStartTime;
		TickType_t lastClickTime;
		unsigned int clickCount;
		
		// 防抖记录
		TickType_t lastUpdateTime;
		
		// 校正后的中心位置
		int calibratedCenterX;
		int calibratedCenterY;
		
		// 按钮状态结构体
		struct ButtonState {
			State state;
			unsigned long pressStartTime;
			unsigned long lastClickTime;
			unsigned int clickCount;
		};
		
		// 按钮状态数组
		ButtonState buttons[JOYSTICK_MAX_BUTTONS];
		
		// 按钮总数
		uint8_t buttonCount;
		
		// 辅助方法
		void detectDirection(int x, int y, Direction &detected);
		void handleAxisState(Direction detected, unsigned long currentMillis);
		void handleButtonState(uint8_t buttonId, bool pressed, unsigned long currentMillis);
		
		// 获取阈值绝对值
		int getThresholdValue(int minVal, int maxVal) const;
	};

#include "Joystick.tpp"

#endif /* _JOYSTICK_H_ */