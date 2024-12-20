#pragma once

// ButtonReader.h
#ifndef BUTTON_READER_H
#define BUTTON_READER_H

#include "includes.h"


	/**
	 * @brief 抽象基类，用于读取按钮状态。
	 */
	class ButtonReader {
	public:
		virtual ~ButtonReader() {}
		
		/**
		 * @brief 读取所有按钮的状态。
		 * 
		 * @param buttons 按钮状态，位掩码表示每个按键的状态（1为按下，0为未按下）
		 * @return true   读取成功
		 * @return false 读取失败
		 */
		virtual bool read(uint8_t &buttons) = 0;

		/**
		 * @brief 获取按钮的数量。
		 * 
		 * @return uint8_t 
		 */
		virtual uint8_t getButtonCount() const = 0;
	};



#include <Arduino.h>

	/**
	 * @brief 通过数字引脚读取按钮状态的实现。
	 */
	class DigitalButtonReader : public ButtonReader {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param firstButtonPin 第一个按钮连接的数字引脚
		 * @param buttonCount    按钮的数量（最多 JOYSTICK_MAX_BUTTONS）
		 */
		DigitalButtonReader(int firstButtonPin, uint8_t buttonCount = 1)
			: firstButtonPin(firstButtonPin), buttonCount(buttonCount) {
			for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
				pinMode(firstButtonPin + i, INPUT_PULLUP); // 假设按钮为低电平按下
			}
		}
		
		virtual bool read(uint8_t &buttons) override {
			buttons = 0;
			for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
				if (digitalRead(firstButtonPin + i) == LOW) {
					buttons |= (1 << i);
				}
			}
			return true;
		}

		virtual uint8_t getButtonCount() const override { return buttonCount; }

	private:
		int firstButtonPin;
		uint8_t buttonCount;
	};

#endif // BUTTON_READER_H