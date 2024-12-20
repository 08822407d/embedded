#pragma once

// JoystickReader.h
#ifndef JOYSTICK_READER_H
#define JOYSTICK_READER_H

#include "includes.h"


	/**
	 * @brief 抽象基类，用于读取摇杆数据。
	 */
	class JoystickReader {
	public:
		virtual ~JoystickReader() {}
		
		/**
		 * @brief 读取摇杆的X、Y值。
		 * 
		 * @param x 摇杆X轴值
		 * @param y 摇杆Y轴值
		 * @return true 读取成功
		 * @return false 读取失败
		 */
		virtual bool read(int &x, int &y) = 0;

		/**
		 * @brief 读取原始数据缓冲区。
		 * 
		 * @param buffer 数据缓冲区
		 * @param length 缓冲区长度
		 * @return true 读取成功
		 * @return false 读取失败
		 */
		virtual bool readRaw(uint8_t* buffer, size_t length) = 0;
		
		/**
		 * @brief 获取摇杆X轴的最小值。
		 * 
		 * @return int 
		 */
		virtual int getMinX() const = 0;

		/**
		 * @brief 获取摇杆X轴的最大值。
		 * 
		 * @return int 
		 */
		virtual int getMaxX() const = 0;

		/**
		 * @brief 获取摇杆Y轴的最小值。
		 * 
		 * @return int 
		 */
		virtual int getMinY() const = 0;

		/**
		 * @brief 获取摇杆Y轴的最大值。
		 * 
		 * @return int 
		 */
		virtual int getMaxY() const = 0;
	};



#include <Arduino.h>

	// 定义ADC模式下的最小值和最大值
	#define ADC_JOYSTICK_MIN_VALUE 0
	#define ADC_JOYSTICK_MAX_VALUE 1023

	/**
	 * @brief 通过 ADC 读取摇杆数据的实现。
	 */
	class ADCJoystickReader : public JoystickReader {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param xPin X轴连接的ADC引脚
		 * @param yPin Y轴连接的ADC引脚
		 */
		ADCJoystickReader(int xPin, int yPin)
			: xPin(xPin), yPin(yPin) {}
		
		virtual bool read(int &x, int &y) override {
			x = analogRead(xPin);
			y = analogRead(yPin);
			return true;
		}

		virtual bool readRaw(uint8_t* buffer, size_t length) override {
			if(length < 4){
				return false;
			}
			int rawX = analogRead(xPin);
			int rawY = analogRead(yPin);
			buffer[0] = rawX & 0xFF;
			buffer[1] = (rawX >> 8) & 0xFF;
			buffer[2] = rawY & 0xFF;
			buffer[3] = (rawY >> 8) & 0xFF;
			return true;
		}
		
		virtual int getMinX() const override { return ADC_JOYSTICK_MIN_VALUE; }
		virtual int getMaxX() const override { return ADC_JOYSTICK_MAX_VALUE; }
		virtual int getMinY() const override { return ADC_JOYSTICK_MIN_VALUE; }
		virtual int getMaxY() const override { return ADC_JOYSTICK_MAX_VALUE; }

	private:
		int xPin;
		int yPin;
	};



#include <Wire.h>

	// 定义I2C模式下的最小值和最大值
	#define I2C_JOYSTICK_MIN_VALUE 0
	#define I2C_JOYSTICK_MAX_VALUE 1023

	/**
	 * @brief 通过 I2C 读取摇杆数据的实现。
	 */
	class I2CJoystickReader : public JoystickReader {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param address I2C设备地址
		 */
		I2CJoystickReader(uint8_t address)
			: address(address) {}
		
		virtual bool read(int &x, int &y) override {
			Wire.beginTransmission(address);
			Wire.write(0x00); // 起始寄存器地址，具体根据硬件调整
			if (Wire.endTransmission(false) != 0) { // 重启条件
				return false;
			}
			Wire.requestFrom(address, 4); // 假设前两个字节为X，接下来的两个字节为Y
			if (Wire.available() >= 4) {
				uint16_t rawX = Wire.read() | (Wire.read() << 8);
				uint16_t rawY = Wire.read() | (Wire.read() << 8);
				x = rawX;
				y = rawY;
				return true;
			}
			return false;
		}

		virtual bool readRaw(uint8_t* buffer, size_t length) override {
			Wire.beginTransmission(address);
			Wire.write(0x00); // 起始寄存器地址
			if (Wire.endTransmission(false) != 0) { // 重启条件
				return false;
			}
			Wire.requestFrom(address, length); // 读取指定长度的数据
			if (Wire.available() >= length) {
				for(size_t i = 0; i < length; i++) {
					buffer[i] = Wire.read();
				}
				return true;
			}
			return false;
		}
		
		virtual int getMinX() const override { return I2C_JOYSTICK_MIN_VALUE; }
		virtual int getMaxX() const override { return I2C_JOYSTICK_MAX_VALUE; }
		virtual int getMinY() const override { return I2C_JOYSTICK_MIN_VALUE; }
		virtual int getMaxY() const override { return I2C_JOYSTICK_MAX_VALUE; }

	private:
		uint8_t address;
	};



#include <SPI.h>

	// 定义SPI模式下的最小值和最大值
	#define SPI_JOYSTICK_MIN_VALUE 0
	#define SPI_JOYSTICK_MAX_VALUE 1023

	/**
	 * @brief 通过 SPI 读取摇杆数据的实现。
	 */
	class SPIJoystickReader : public JoystickReader {
	public:
		/**
		 * @brief 构造函数
		 * 
		 * @param csPin SPI CS引脚
		 */
		SPIJoystickReader(int csPin)
			: csPin(csPin) {
			pinMode(csPin, OUTPUT);
			digitalWrite(csPin, HIGH); // CS高电平禁用
			SPI.begin();
		}
		
		virtual bool read(int &x, int &y) override {
			digitalWrite(csPin, LOW); // 选中设备
			
			// 发送读取命令，假设设备返回四个字节：X低、X高、Y低、Y高
			uint8_t response[4];
			response[0] = SPI.transfer(0x00); // X低
			response[1] = SPI.transfer(0x00); // X高
			response[2] = SPI.transfer(0x00); // Y低
			response[3] = SPI.transfer(0x00); // Y高
			
			digitalWrite(csPin, HIGH); // 取消选中设备
			
			uint16_t rawX = response[0] | (response[1] << 8);
			uint16_t rawY = response[2] | (response[3] << 8);
			x = rawX;
			y = rawY;
			
			return true;
		}

		virtual bool readRaw(uint8_t* buffer, size_t length) override {
			digitalWrite(csPin, LOW); // 选中设备
			
			// 发送读取命令
			for(size_t i = 0; i < length; i++) {
				buffer[i] = SPI.transfer(0x00); // 读取数据
			}
			
			digitalWrite(csPin, HIGH); // 取消选中设备
			
			return true;
		}
		
		virtual int getMinX() const override { return SPI_JOYSTICK_MIN_VALUE; }
		virtual int getMaxX() const override { return SPI_JOYSTICK_MAX_VALUE; }
		virtual int getMinY() const override { return SPI_JOYSTICK_MIN_VALUE; }
		virtual int getMaxY() const override { return SPI_JOYSTICK_MAX_VALUE; }

	private:
		int csPin;
	};

#endif // JOYSTICK_READER_H