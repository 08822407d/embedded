#include "Wire.hpp"


IdfTwoWire Wire(I2C_NUM_0);
IdfTwoWire Wire1(I2C_NUM_1);


static const char *TAG = "IdfTwoWire";

IdfTwoWire::IdfTwoWire(i2c_port_t i2cPort)
: _i2cPort(i2cPort),
  _busHandle(nullptr),
  _devHandle(nullptr),
  _sdaPin(-1),
  _sclPin(-1),
  _clockHz(100000),
  _currentAddr(0),
  _busInited(false),
  _error(0),
  _rxIndex(0)
{
	_txBuffer.reserve(ESP_SIMPLE_I2C_BUFFER_LENGTH);
	_rxBuffer.reserve(ESP_SIMPLE_I2C_BUFFER_LENGTH);
}

esp_err_t IdfTwoWire::begin(int sdaPin, int sclPin, uint32_t frequency)
{
	_sdaPin  = sdaPin;
	_sclPin  = sclPin;
	_clockHz = frequency;

	return initBus();
}

esp_err_t IdfTwoWire::end()
{
	// 若有devHandle, 移除
	if (_devHandle) {
		i2c_master_bus_rm_device(_devHandle);
		_devHandle = nullptr;
	}
	// 若有busHandle, 删除
	if (_busHandle) {
		i2c_del_master_bus(_busHandle);
		_busHandle = nullptr;
	}
	_busInited = false;
	_currentAddr = 0;
	return ESP_OK;
}

esp_err_t IdfTwoWire::setClock(uint32_t frequency)
{
	_clockHz = frequency;
	// 若 bus 已经 init, 需要重新配置
	if (_busInited) {
		// 先把devHandle移除
		removeDeviceHandle();

		// 删除bus
		i2c_del_master_bus(_busHandle);
		_busHandle = nullptr;
		_busInited = false;

		// 重新 init
		esp_err_t err = initBus();
		if (err != ESP_OK) {
			return err;
		}
	}
	return ESP_OK;
}

esp_err_t IdfTwoWire::initBus()
{
	if (_busHandle) {
		// 若已存在，不再重复
		return ESP_OK;
	}

	i2c_master_bus_config_t bus_conf = {
		.i2c_port = _i2cPort,
		.sda_io_num = static_cast<gpio_num_t>(_sdaPin),
		.scl_io_num = static_cast<gpio_num_t>(_sclPin),
		.clk_source = I2C_CLK_SRC_DEFAULT, // 或根据需要配置
		.glitch_ignore_cnt = 7,
		.flags = {
			.enable_internal_pullup = true,
		}
	};
	esp_err_t err = i2c_new_master_bus(&bus_conf, &_busHandle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "i2c_new_master_bus fail: %s", esp_err_to_name(err));
		return err;
	}
	_busInited = true;
	return ESP_OK;
}

void IdfTwoWire::beginTransmission(uint8_t address)
{
	_txBuffer.clear();
	_error = 0;

	// 如果当前地址与新地址不一致，需要重新init device
	if (address != _currentAddr) {
		_error = reinitDeviceIfNeeded(address);
	}
}

size_t IdfTwoWire::write(uint8_t data)
{
	if (_txBuffer.size() < ESP_SIMPLE_I2C_BUFFER_LENGTH) {
		_txBuffer.push_back(data);
		return 1;
	}
	return 0;
}

size_t IdfTwoWire::write(const uint8_t *data, size_t length)
{
	size_t count = 0;
	for (size_t i = 0; i < length; i++) {
		count += write(data[i]);
	}
	return count;
}

uint8_t IdfTwoWire::endTransmission(bool /*sendStop*/)
{
	// 对于新 API，不存在“sendStop”可选项，这里仅保留兼容，但不使用
	if (_devHandle == nullptr) {
		_error = 10; // 没有有效的dev handle
		return _error;
	}
	if (_txBuffer.empty()) {
		// 允许空发送
	}

	esp_err_t err = i2c_master_transmit(_devHandle,
										_txBuffer.data(),
										_txBuffer.size(),
										1000 / portTICK_PERIOD_MS);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "endTransmission: i2c_master_transmit fail: %s", esp_err_to_name(err));
		_error = 11;
		return _error;
	}

	_error = 0;
	return 0;
}

size_t IdfTwoWire::requestFrom(uint8_t address, size_t quantity, bool /*sendStop*/)
{
	_rxBuffer.clear();
	_rxBuffer.resize(quantity, 0);
	_rxIndex = 0;

	// 如果地址与当前不符，则重配
	if (address != _currentAddr) {
		_error = reinitDeviceIfNeeded(address);
		if (_error != 0) {
			return 0; // 设备初始化失败
		}
	}

	if (_devHandle == nullptr) {
		_error = 20;
		return 0;
	}

	// 把当前 _txBuffer 当做“要先写出的数据”。若用户只是想读寄存器，_txBuffer里通常放寄存器地址。
	esp_err_t err = ESP_OK;
	if (!_txBuffer.empty()) {
		// 同时写再读
		err = i2c_master_transmit_receive(_devHandle,
										  _txBuffer.data(), _txBuffer.size(),
										  _rxBuffer.data(), quantity,
										  1000 / portTICK_PERIOD_MS);
	} else {
		// 如果没有要写的，则纯读 => 需要另外的API? 
		// 事实上 i2c_master_transmit_receive不写也行，但要传空指针
		err = i2c_master_transmit_receive(_devHandle,
										  nullptr, 0,
										  _rxBuffer.data(), quantity,
										  1000 / portTICK_PERIOD_MS);
	}

	// 清空 _txBuffer，因为一次 requestFrom 通常就结束了
	_txBuffer.clear();

	if (err != ESP_OK) {
		ESP_LOGE(TAG, "requestFrom: i2c_master_transmit_receive fail: %s", esp_err_to_name(err));
		_rxBuffer.clear();
		_error = 21;
		return 0;
	}

	_error = 0;
	return _rxBuffer.size();
}

int IdfTwoWire::available()
{
	return (_rxBuffer.size() - _rxIndex);
}

int IdfTwoWire::read()
{
	if (_rxIndex < _rxBuffer.size()) {
		return _rxBuffer[_rxIndex++];
	}
	return -1;
}

esp_err_t IdfTwoWire::reinitDeviceIfNeeded(uint8_t address)
{
	if (!_busInited) {
		esp_err_t e = initBus();
		if (e != ESP_OK) return e;
	}

	// 如果已有 devHandle，则先移除
	if (_devHandle) {
		i2c_master_bus_rm_device(_devHandle);
		_devHandle = nullptr;
	}

	_currentAddr = address;

	// 创建一个新的 device handle
	i2c_device_config_t dev_conf = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address  = address,
		.scl_speed_hz    = _clockHz,
	};
	esp_err_t err = i2c_master_bus_add_device(_busHandle, &dev_conf, &_devHandle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "reinitDevice: i2c_master_bus_add_device fail: %s", esp_err_to_name(err));
		_devHandle = nullptr;
		return err;
	}
	return ESP_OK;
}

esp_err_t IdfTwoWire::removeDeviceHandle()
{
	if (_devHandle) {
		esp_err_t e = i2c_master_bus_rm_device(_devHandle);
		_devHandle = nullptr;
		return e;
	}
	return ESP_OK;
}