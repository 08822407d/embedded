#pragma once

#include <stdio.h>
#include <string.h>
#include <vector>
#include "driver/i2c.h"
#include "esp_log.h"

#ifndef ESP_SIMPLE_I2C_BUFFER_LENGTH
#  define ESP_SIMPLE_I2C_BUFFER_LENGTH 128
#endif


class IdfTwoWire {
public:
	/**
	 * @brief 构造函数，指定 I2C 端口号，默认为 I2C_NUM_0
	 */
	explicit IdfTwoWire(i2c_port_t i2cPort);

	/**
	 * @brief 初始化 I2C 主机，并设置 SDA、SCL 引脚及时钟频率（默认为 100kHz）
	 * @param sdaPin  SDA 引脚
	 * @param sclPin  SCL 引脚
	 * @param frequency I2C 时钟，单位 Hz，默认 100000
	 */
	esp_err_t begin(gpio_num_t sdaPin, gpio_num_t sclPin, uint32_t frequency);

	/**
	 * @brief 更改 I2C 时钟频率
	 * @param frequency 新的时钟频率，单位 Hz
	 */
	esp_err_t setClock(uint32_t frequency);

	/**
	 * @brief 开始一段传输，指定从机地址
	 * @param address I2C 从机地址 (7bit)
	 */
	void beginTransmission(uint8_t address);

	/**
	 * @brief 向当前传输队列写入单个字节
	 * @param data 要写入的字节
	 */
	size_t write(uint8_t data);

	/**
	 * @brief 向当前传输队列写入多字节
	 * @param data 要写入的数据指针
	 * @param length 数据长度
	 */
	size_t write(const uint8_t *data, size_t length);

	/**
	 * @brief 结束传输，将队列的数据发送给从机
	 * @param sendStop 是否在传输后发送 STOP 信号
	 * @return 0 表示无错误，其他值表示发生错误
	 */
	uint8_t endTransmission(bool sendStop);

	/**
	 * @brief 从指定从机地址请求读取数据
	 * @param address 从机地址
	 * @param quantity 请求的数据字节数
	 * @param sendStop 结束后是否发送 STOP 信号
	 * @return 实际可读取的字节数
	 */
	size_t requestFrom(uint8_t address, size_t quantity, bool sendStop);

	/**
	 * @brief 逐字节读取 requestFrom 请求到的缓冲区
	 * @return 若有数据则返回字节值，没有则返回 -1
	 */
	int read();

	/**
	 * @brief 获取上一次通信的错误码
	 */
	uint8_t getLastError() const;

private:
	i2c_port_t _i2cPort;
	gpio_num_t _sdaPin;
	gpio_num_t _sclPin;
	uint32_t   _clockHz;

	uint8_t    _transmissionAddress;
	uint8_t    _error;

	// 写出时的缓存
	std::vector<uint8_t> _txBuffer;
	// 读入时的缓存
	std::vector<uint8_t> _rxBuffer;
	size_t               _rxIndex = 0;
};



extern IdfTwoWire Wire;
extern IdfTwoWire Wire1;