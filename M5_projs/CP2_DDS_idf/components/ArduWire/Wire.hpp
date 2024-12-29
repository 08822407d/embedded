#pragma once

#include <vector>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_log.h"


#ifndef ESP_SIMPLE_I2C_BUFFER_LENGTH
#  define ESP_SIMPLE_I2C_BUFFER_LENGTH 128
#endif

/**
 * @brief 一个仿 Arduino TwoWire 风格的 I2C 封装，基于最新 i2c_master_* API
 *
 * - 支持 begin()/end() 控制主机
 * - 支持 beginTransmission()/write()/endTransmission() 写数据
 * - 支持 requestFrom() + read()/available() 读数据
 * - 能够在运行时更改频率 (setClock)
 * - 当切换从机地址时，会相应创建 / 移除 device handle
 *
 * 注意：此示例仅供参考，实际项目可根据需求调整
 */

class IdfTwoWire {
public:
public:
	/**
	 * @param i2cPort I2C 主机端口号 (I2C_NUM_0 / I2C_NUM_1等)
	 */
	explicit IdfTwoWire(i2c_port_t i2cPort = I2C_NUM_0);

	/**
	 * @brief 初始化 I2C 主机 (即 "bus" )，配置 SCL、SDA 引脚及频率
	 * @param sdaPin  SDA引脚
	 * @param sclPin  SCL引脚
	 * @param frequency I2C 频率(Hz)
	 * @return esp_err_t
	 */
	esp_err_t begin(int sdaPin, int sclPin, uint32_t frequency = 100000);

	/**
	 * @brief 结束/卸载 I2C 主机 (和已有从机)，释放资源
	 * @note  若后续还要使用I2C，需要再次 begin()
	 */
	esp_err_t end();

	/**
	 * @brief 修改 I2C 时钟频率
	 * @param frequency 新的频率(Hz)
	 * @return esp_err_t
	 */
	esp_err_t setClock(uint32_t frequency);

	/**
	 * @brief 开始与某个 7bit 从机地址的通信。会清空发送缓存。
	 *        如果地址变化，会自动移除旧的 device handle 并创建新的。
	 */
	void beginTransmission(uint8_t address);

	/**
	 * @brief 写一个字节到发送缓存
	 */
	size_t write(uint8_t data);

	/**
	 * @brief 写多个字节到发送缓存
	 */
	size_t write(const uint8_t *data, size_t length);

	/**
	 * @brief 结束传输，将发送缓存数据通过 i2c_master_transmit(...) 发给从机
	 * @param sendStop 是否在传输后发送STOP
	 * @return 0 表示成功；非0 表示有错误（简化处理）
	 */
	uint8_t endTransmission(bool sendStop = true);

	/**
	 * @brief 请求从指定地址读取 quantity 个字节
	 *        - 内部调用 i2c_master_transmit_receive(...), 其中 transmit部分 = txBuffer
	 *        - 返回可用数据存于 rxBuffer
	 * @param address  7bit 从机地址
	 * @param quantity 请求读取的字节数
	 * @param sendStop 是否读完发送STOP
	 * @return 实际可读取到的字节数
	 */
	size_t requestFrom(uint8_t address, size_t quantity, bool sendStop = true);

	/**
	 * @brief 可用的未读取字节数 (类似 Arduino Wire.available())
	 */
	int available();

	/**
	 * @brief 从rxBuffer中读取一个字节；若无则返回-1
	 */
	int read();

	/**
	 * @brief 获取最近一次操作的错误码 (0 表示成功)
	 */
	uint8_t getLastError() const { return _error; }

private:
	/**
	 * @brief 初始化(或重新初始化) bus_handle
	 */
	esp_err_t initBus();

	/**
	 * @brief 若 address 变化，需要先卸载旧 device handle, 再添加新 device handle
	 */
	esp_err_t reinitDeviceIfNeeded(uint8_t address);

	/**
	 * @brief 移除并清空当前 dev_handle
	 */
	esp_err_t removeDeviceHandle();

private:
	i2c_port_t                  _i2cPort;
	i2c_master_bus_handle_t     _busHandle;
	i2c_master_dev_handle_t     _devHandle;

	int                         _sdaPin;
	int                         _sclPin;
	uint32_t                    _clockHz;

	uint8_t                     _currentAddr; ///< 当前的从机地址(7bit)
	bool                        _busInited;   ///< 是否已初始化bus

	uint8_t                     _error;

	std::vector<uint8_t>        _txBuffer;
	std::vector<uint8_t>        _rxBuffer;
	size_t                      _rxIndex;
};


#define TwoWire IdfTwoWire

extern IdfTwoWire Wire;
extern IdfTwoWire Wire1;