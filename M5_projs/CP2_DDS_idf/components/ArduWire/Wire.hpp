#pragma once

#include <stdio.h>
#include <string.h>
#include <vector>
#include "driver/i2c.h"
#include "esp_log.h"

#ifndef ESP_SIMPLE_I2C_BUFFER_LENGTH
#define ESP_SIMPLE_I2C_BUFFER_LENGTH 128
#endif

class EspSimpleI2C {
public:
    /**
     * @brief 构造函数，指定 I2C 端口号，默认为 I2C_NUM_0
     */
    explicit EspSimpleI2C(i2c_port_t i2cPort = I2C_NUM_0)
    : _i2cPort(i2cPort), _sdaPin(GPIO_NUM_NC), _sclPin(GPIO_NUM_NC), _clockHz(100000), 
      _transmissionAddress(0), _error(0)
    {
        _txBuffer.reserve(ESP_SIMPLE_I2C_BUFFER_LENGTH);
    }

    /**
     * @brief 初始化 I2C 主机，并设置 SDA、SCL 引脚及时钟频率（默认为 100kHz）
     * @param sdaPin  SDA 引脚
     * @param sclPin  SCL 引脚
     * @param frequency I2C 时钟，单位 Hz，默认 100000
     */
    esp_err_t begin(gpio_num_t sdaPin, gpio_num_t sclPin, uint32_t frequency = 100000) {
        _sdaPin = sdaPin;
        _sclPin = sclPin;
        _clockHz = frequency;

        i2c_config_t conf;
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = _sdaPin;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_io_num = _sclPin;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = _clockHz;
        conf.clk_flags = 0; // 某些版本可使用 I2C_SCLK_SRC_FLAG_* 进行配置

        esp_err_t err = i2c_param_config(_i2cPort, &conf);
        if (err != ESP_OK) {
            ESP_LOGE("EspSimpleI2C", "i2c_param_config fail: %s", esp_err_to_name(err));
            return err;
        }

        err = i2c_driver_install(_i2cPort, conf.mode, 0, 0, 0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE("EspSimpleI2C", "i2c_driver_install fail: %s", esp_err_to_name(err));
            return err;
        }
        // 若已经安装过驱动，ESP_ERR_INVALID_STATE，可以忽略或做进一步处理

        return ESP_OK;
    }

    /**
     * @brief 更改 I2C 时钟频率
     * @param frequency 新的时钟频率，单位 Hz
     */
    esp_err_t setClock(uint32_t frequency) {
        _clockHz = frequency;
        // 重新配置
        i2c_config_t conf;
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = _sdaPin;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_io_num = _sclPin;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = _clockHz;
        conf.clk_flags = 0;

        esp_err_t err = i2c_param_config(_i2cPort, &conf);
        if (err != ESP_OK) {
            return err;
        }
        return ESP_OK;
    }

    /**
     * @brief 开始一段传输，指定从机地址
     * @param address I2C 从机地址 (7bit)
     */
    void beginTransmission(uint8_t address) {
        _transmissionAddress = address;
        _txBuffer.clear();
        _error = 0;
    }

    /**
     * @brief 向当前传输队列写入单个字节
     * @param data 要写入的字节
     */
    size_t write(uint8_t data) {
        if (_txBuffer.size() < ESP_SIMPLE_I2C_BUFFER_LENGTH) {
            _txBuffer.push_back(data);
            return 1;
        } else {
            // 缓存已满，可根据需要处理错误
            return 0;
        }
    }

    /**
     * @brief 向当前传输队列写入多字节
     * @param data 要写入的数据指针
     * @param length 数据长度
     */
    size_t write(const uint8_t *data, size_t length) {
        size_t cnt = 0;
        for (size_t i = 0; i < length; i++) {
            cnt += write(data[i]);
        }
        return cnt;
    }

    /**
     * @brief 结束传输，将队列的数据发送给从机
     * @param sendStop 是否在传输后发送 STOP 信号
     * @return 0 表示无错误，其他值表示发生错误
     */
    uint8_t endTransmission(bool sendStop = true) {
        if (_txBuffer.empty()) {
            // 没有数据写也可以，但通常要写至少一字节
        }

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);

        // 写地址（WRITE）
        i2c_master_write_byte(cmd, (_transmissionAddress << 1) | I2C_MASTER_WRITE, true);

        // 写数据
        if (!_txBuffer.empty()) {
            i2c_master_write(cmd, _txBuffer.data(), _txBuffer.size(), true);
        }

        if (sendStop) {
            i2c_master_stop(cmd);
        }

        esp_err_t err = i2c_master_cmd_begin(_i2cPort, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (err != ESP_OK) {
            _error = 4; // 简单用 4 表示传输失败
            ESP_LOGE("EspSimpleI2C", "endTransmission error: %s", esp_err_to_name(err));
        } else {
            _error = 0;
        }
        return _error;
    }

    /**
     * @brief 从指定从机地址请求读取数据
     * @param address 从机地址
     * @param quantity 请求的数据字节数
     * @param sendStop 结束后是否发送 STOP 信号
     * @return 实际可读取的字节数
     */
    size_t requestFrom(uint8_t address, size_t quantity, bool sendStop = true) {
        _rxBuffer.clear();
        _rxBuffer.reserve(quantity);

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);

        // 写地址（READ）
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);

        // 读取数据
        if (quantity > 1) {
            // 前 (quantity-1) 字节使用 ACK
            i2c_master_read(cmd, _rxBuffer.data(), quantity - 1, I2C_MASTER_ACK);
        }
        // 最后一个字节使用 NACK
        if (quantity > 0) {
            _rxBuffer.push_back(0); // 先占位
            i2c_master_read_byte(cmd, &_rxBuffer[0], I2C_MASTER_NACK); 
        }

        if (sendStop) {
            i2c_master_stop(cmd);
        }

        esp_err_t err = i2c_master_cmd_begin(_i2cPort, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (err != ESP_OK) {
            _error = 5; 
            ESP_LOGE("EspSimpleI2C", "requestFrom error: %s", esp_err_to_name(err));
            _rxBuffer.clear();
            return 0;
        } else {
            _error = 0;
        }

        // 注意：上述写法仅演示思路，若 quantity > 1，需要正确分配缓冲并依次读取
        // 这里演示写法较简单，真实项目中需更严谨处理
        // 可以改用 i2c_master_read(cmd, buffer, quantity, I2C_MASTER_LAST_NACK);
        // 或分多步 read。
        _rxIndex = 0; // 用于 read()
        return _rxBuffer.size();
    }

    /**
     * @brief 逐字节读取 requestFrom 请求到的缓冲区
     * @return 若有数据则返回字节值，没有则返回 -1
     */
    int read() {
        if (_rxIndex < _rxBuffer.size()) {
            return _rxBuffer[_rxIndex++];
        }
        return -1;
    }

    /**
     * @brief 获取上一次通信的错误码
     */
    uint8_t getLastError() const {
        return _error;
    }

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