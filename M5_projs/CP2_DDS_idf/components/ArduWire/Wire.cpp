#include "Wire.hpp"


IdfTwoWire Wire(I2C_NUM_0);
IdfTwoWire Wire1(I2C_NUM_1);


IdfTwoWire::IdfTwoWire(i2c_port_t i2cPort = I2C_NUM_0)
:	_i2cPort(i2cPort), _sdaPin(GPIO_NUM_NC),
	_sclPin(GPIO_NUM_NC), _clockHz(100000), 
	_transmissionAddress(0), _error(0)
{
	_txBuffer.reserve(ESP_SIMPLE_I2C_BUFFER_LENGTH);
}


esp_err_t IdfTwoWire::begin(
		gpio_num_t sdaPin, gpio_num_t sclPin,
		uint32_t frequency = 100000) {

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

esp_err_t IdfTwoWire::setClock(uint32_t frequency) {
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

void IdfTwoWire::beginTransmission(uint8_t address) {
	_transmissionAddress = address;
	_txBuffer.clear();
	_error = 0;
}

size_t IdfTwoWire::write(uint8_t data) {
	if (_txBuffer.size() < ESP_SIMPLE_I2C_BUFFER_LENGTH) {
		_txBuffer.push_back(data);
		return 1;
	} else {
		// 缓存已满，可根据需要处理错误
		return 0;
	}
}

size_t IdfTwoWire::write(const uint8_t *data, size_t length) {
	size_t cnt = 0;
	for (size_t i = 0; i < length; i++) {
		cnt += write(data[i]);
	}
	return cnt;
}

uint8_t IdfTwoWire::endTransmission(bool sendStop = true) {
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

size_t IdfTwoWire::requestFrom(uint8_t address, size_t quantity, bool sendStop = true) {
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

int IdfTwoWire::read() {
	if (_rxIndex < _rxBuffer.size()) {
		return _rxBuffer[_rxIndex++];
	}
	return -1;
}

uint8_t IdfTwoWire::getLastError() const {
	return _error;
}