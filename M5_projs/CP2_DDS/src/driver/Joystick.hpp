#pragma once

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <Arduino.h>
#include <functional>
#include <memory>
#include "JoystickReader.hpp"
#include "JoystickConfig.h"
#include "modules/Events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


	/**
	 * @brief Joystick类，用于处理摇杆的X和Y轴事件检测。
	 */
	class Joystick {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param reader                共享指针，指向JoystickReader实例
		 * @param thresholdPercent      阈值百分比（0-100）
		 * @param longPressDurationMs  长按持续时间（毫秒）
		 * @param multiclickIntervalMs  多击间隔时间（毫秒）
		 */
		Joystick(std::shared_ptr<JoystickReader> reader, 
				unsigned char thresholdPercent = 20,
				unsigned long longPressDurationMs = JOYSTICK_LONG_PRESS_DURATION_MS, 
				unsigned long multiclickIntervalMs = JOYSTICK_MULTICLICK_INTERVAL_MS);
		
		// 设置阈值百分比
		void setThresholdPercent(unsigned char thresholdPercent);
		
		// 设置长按持续时间
		void setLongPressDuration(unsigned long durationMs);
		
		// 设置多击间隔
		void setMulticlickInterval(unsigned long intervalMs);
		
		// 设置轮询间隔
		void setPollingInterval(unsigned long intervalMs);
		
		// 获取轮询间隔
		unsigned long getPollingInterval() const { return pollingIntervalMs; }
		
		// 注册事件回调
		void attachCallback(std::function<void(const JoystickEvent&)> cb);
		
		// 校正摇杆中心位置
		void calibrate(int centerX, int centerY);
		
		/**
		 * @brief 启用或禁用防抖功能
		 * @param enable true为启用防抖，false为禁用防抖
		 */
		void enableDebounce(bool enable);
		
		/**
		 * @brief 定期调用此方法以更新摇杆状态
		 */
		void update();

	private:
		// 数据读取接口
		std::shared_ptr<JoystickReader> reader;
		
		// 阈值百分比（0-100）
		unsigned char thresholdPercent;
		
		// 长按持续时间（毫秒）
		unsigned long longPressDurationMs;
		
		// 多击间隔（毫秒）
		unsigned long multiclickIntervalMs;
		
		// 轮询间隔（毫秒）
		volatile unsigned long pollingIntervalMs;
		
		// 事件回调
		std::function<void(const JoystickEvent&)> callback;
		
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
		
		// 时间记录（ticks）
		TickType_t pressStartTick;
		TickType_t lastClickTick;
		unsigned int clickCount;
		
		// 防抖记录
		TickType_t lastUpdateTick;
		
		// 校正后的中心位置
		int calibratedCenterX;
		int calibratedCenterY;
		
		// 辅助方法
		void detectDirection(int x, int y, Direction &detected);
		void handleAxisState(Direction detected, TickType_t currentTick);
		
		// 获取阈值绝对值
		int getThresholdValue(int minVal, int maxVal) const;
	};

#include "Joystick.tpp"

#endif /* _JOYSTICK_H_ */