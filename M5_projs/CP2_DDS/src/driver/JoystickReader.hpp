#pragma once

#ifndef JOYSTICK_READER_H
#define JOYSTICK_READER_H

#include <stdint.h>

#include <Arduino.h>
#include <Wire.h>

#include "JoystickConfig.h"


	/**
	 * @brief 抽象基类，用于读取摇杆数据和按钮状态。
	 * 
	 * @tparam T 数据类型，通常为 int。
	 */
	template <typename T>
	class JoystickReader {
	public:
		virtual ~JoystickReader() {}
		
		/**
		 * @brief 读取摇杆的X、Y值和按钮状态。
		 * 
		 * @param x         摇杆X轴值
		 * @param y         摇杆Y轴值
		 * @param buttons   按钮状态，位掩码表示每个按键的状态（1为按下，0为未按下）
		 * @return true      读取成功
		 * @return false     读取失败
		 */
		virtual bool read(int &x, int &y, uint8_t &buttons) = 0;

		/**
		 * @brief 获取摇杆X轴的最小值
		 * 
		 * @return int 
		 */
		virtual int getMinX() const = 0;

		/**
		 * @brief 获取摇杆X轴的最大值
		 * 
		 * @return int 
		 */
		virtual int getMaxX() const = 0;

		/**
		 * @brief 获取摇杆Y轴的最小值
		 * 
		 * @return int 
		 */
		virtual int getMinY() const = 0;

		/**
		 * @brief 获取摇杆Y轴的最大值
		 * 
		 * @return int 
		 */
		virtual int getMaxY() const = 0;

		/**
		 * @brief 获取按钮的数量
		 * 
		 * @return uint8_t 
		 */
		virtual uint8_t getButtonCount() const = 0;
	};



	// 定义ADC模式下的最小值和最大值
	#define ADC_JOYSTICK_MIN_VALUE 0
	#define ADC_JOYSTICK_MAX_VALUE 1023

	/**
	 * @brief 通过 ADC 读取摇杆和按钮状态的实现。
	 * 
	 * @tparam T 数据类型，通常为 int。
	 */
	template <typename T>
	class ADCJoystickReader : public JoystickReader<T> {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param xPin          X轴连接的ADC引脚
		 * @param yPin          Y轴连接的ADC引脚
		 * @param buttonPin     第一个按钮连接的数字引脚
		 * @param buttonCount   按钮的数量（最多 JOYSTICK_MAX_BUTTONS）
		 */
		ADCJoystickReader(int xPin, int yPin, int buttonPin, uint8_t buttonCount = 1)
			: xPin(xPin), yPin(yPin), buttonPin(buttonPin), buttonCount(buttonCount) {
			// 初始化按钮引脚为输入，并启用上拉电阻
			for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
				pinMode(buttonPin + i, INPUT_PULLUP);
			}
		}
		
		virtual bool read(int &x, int &y, uint8_t &buttons) override {
			x = analogRead(xPin);
			y = analogRead(yPin);
			// 读取按钮状态
			buttons = 0;
			for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
				// 假设按钮为数字输入，低电平按下，高电平释放
				if (digitalRead(buttonPin + i) == LOW) {
					buttons |= (1 << i);
				}
			}
			return true;
		}

		virtual int getMinX() const override { return ADC_JOYSTICK_MIN_VALUE; }
		virtual int getMaxX() const override { return ADC_JOYSTICK_MAX_VALUE; }
		virtual int getMinY() const override { return ADC_JOYSTICK_MIN_VALUE; }
		virtual int getMaxY() const override { return ADC_JOYSTICK_MAX_VALUE; }

		virtual uint8_t getButtonCount() const override { return buttonCount; }

	private:
		int xPin;
		int yPin;
		int buttonPin; // 起始按钮引脚
		uint8_t buttonCount;
	};

	// 定义I2C模式下的最小值和最大值
	#define I2C_JOYSTICK_MIN_VALUE 0
	#define I2C_JOYSTICK_MAX_VALUE 1023

	/**
	 * @brief 通过 I2C 读取摇杆和按钮状态的实现。
	 * 
	 * @tparam T 数据类型，通常为 int。
	 */
	template <typename T>
	class I2CJoystickReader : public JoystickReader<T> {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param address       I2C设备地址
		 * @param buttonCount   按钮的数量（最多 JOYSTICK_MAX_BUTTONS）
		 */
		I2CJoystickReader(int address, int regstart = 0, uint8_t buttonCount = 0)
			: address(address), regstart(regstart), buttonCount(buttonCount) {}
		
		virtual bool read(int &x, int &y, uint8_t &buttons) override {
			Wire.beginTransmission(address);
			Wire.write(regstart); // 起始寄存器地址，具体根据硬件调整
			if (Wire.endTransmission(false) != 0) { // 重启条件
				return false;
			}
			Wire.requestFrom(address, 2 + ((buttonCount + 7) / 8)); // 2轴数据 + 按钮字节
			if (Wire.available() >= 2) {
				x = Wire.read();
				y = Wire.read();
				buttons = 0;
				for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
					if(Wire.available() > 0){
						uint8_t buttonByte = Wire.read();
						if(buttonByte & (1 << i)){
							buttons |= (1 << i);
						}
					}
				}
				return true;
			}
			return false;
		}

		virtual int getMinX() const override { return I2C_JOYSTICK_MIN_VALUE; }
		virtual int getMaxX() const override { return I2C_JOYSTICK_MAX_VALUE; }
		virtual int getMinY() const override { return I2C_JOYSTICK_MIN_VALUE; }
		virtual int getMaxY() const override { return I2C_JOYSTICK_MAX_VALUE; }

		virtual uint8_t getButtonCount() const override { return buttonCount; }

	private:
		int		address;
		int		regstart;
		uint8_t buttonCount;
	};



	// 定义M5摇杆读数最小值和最大值
	#define M5_JOYSTICK_MIN_VALUE 0
	#define M5_JOYSTICK_MAX_VALUE 127

	/**
	 * @brief 通过 I2C 读取摇杆和按钮状态的实现。
	 * 
	 * @tparam T 数据类型，通常为 int。
	 */
	template <typename T>
	class M5JoystickReader : public JoystickReader<T> {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param address       I2C设备地址
		 * @param buttonCount   按钮的数量（最多 JOYSTICK_MAX_BUTTONS）
		 */
		M5JoystickReader(int address, int regstart = 2, uint8_t buttonCount = 0)
			: address(address), regstart(regstart), buttonCount(buttonCount) {}
		
		virtual bool read(int &x, int &y, uint8_t &buttons) override {
			Wire.beginTransmission(address);
			Wire.write(regstart); // 起始寄存器地址，具体根据硬件调整
			if (Wire.endTransmission(false) != 0) { // 重启条件
				return false;
			}
			Wire.requestFrom(address, 2 + buttonCount); // 2轴数据 + 按钮字节
			if (Wire.available() >= 2) {
				x = (int8_t)Wire.read();
				y = (int8_t)Wire.read();
				buttons = 0;
				for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
					if(Wire.available() > 0){
						uint8_t buttonByte = Wire.read();
						if(buttonByte & (1 << i)){
							buttons |= (1 << i);
						}
					}
				}
				return true;
			}
			return false;
		}

		virtual int getMinX() const override { return M5_JOYSTICK_MIN_VALUE; }
		virtual int getMaxX() const override { return M5_JOYSTICK_MAX_VALUE; }
		virtual int getMinY() const override { return M5_JOYSTICK_MIN_VALUE; }
		virtual int getMaxY() const override { return M5_JOYSTICK_MAX_VALUE; }

		virtual uint8_t getButtonCount() const override { return buttonCount; }

	private:
		int		address;
		int		regstart;
		uint8_t buttonCount;
	};
#endif // JOYSTICK_READER_H