#pragma once

// ButtonReader.h
#ifndef BUTTON_READER_H
#define BUTTON_READER_H

#include <stdint.h>

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

#endif // BUTTON_READER_H