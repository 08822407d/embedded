#include <Arduino.h>
extern "C" {
#include "driver/spi_slave.h"
#include "esp_log.h"
}

// #define PIN_MISO 12
// #define PIN_MOSI 11
// #define PIN_SCLK 13
// #define PIN_CS   10

#define PIN_MISO	37
#define PIN_MOSI	36
#define PIN_SCLK	35
#define PIN_CS		34

// 接收缓冲区大小
#define RX_BUF_SIZE 64

static const char* TAG = "SPI_SLAVE";
static uint8_t rx_buffer[RX_BUF_SIZE];

// 事务描述符
static spi_slave_transaction_t trans;

static void IRAM_ATTR spi_post_trans_cb(spi_slave_transaction_t *trans) {
	// 当一次SPI事务完成时会调用此回调（在中断上下文）
	// 在这里不直接打印（中断中尽量少做复杂操作），将数据标记后在loop中处理
}

void setup(void) {
	Serial.begin(115200);
	Serial.println("SPI Slave Interrupt Example Start...\n");

	// SPI总线配置
	spi_bus_config_t buscfg = {
		.mosi_io_num = PIN_MOSI,
		.miso_io_num = PIN_MISO,
		.sclk_io_num = PIN_SCLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = RX_BUF_SIZE
	};

	// SPI从机接口配置
	spi_slave_interface_config_t slvcfg = {
		.spics_io_num = PIN_CS,
		.flags = 0,
		.queue_size = 1, // 队列中可挂起的事务数
		.mode = 0,       // SPI模式0
		.post_setup_cb = NULL,
		.post_trans_cb = spi_post_trans_cb
	};

	// 未使用DMA
	esp_err_t ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, 0);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize SPI slave: %s", esp_err_to_name(ret));
		while(1);
	}

	// 设置一次接收事务
	memset(&trans, 0, sizeof(trans));
	memset(rx_buffer, 0, RX_BUF_SIZE);
	trans.length = RX_BUF_SIZE * 8; // 单位bit
	trans.rx_buffer = rx_buffer;
	trans.tx_buffer = NULL; // 从机此例不发送数据给主机

	// 将此事务队列入SPI驱动
	ret = spi_slave_queue_trans(SPI2_HOST, &trans, portMAX_DELAY);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to queue SPI transaction: %s", esp_err_to_name(ret));
	}
}


void loop() {
	// 轮询完成的事务
	spi_slave_transaction_t *rtrans;
	esp_err_t ret = spi_slave_get_trans_result(SPI2_HOST, &rtrans, 0);

	if (ret == ESP_OK && rtrans == &trans) {
		// 有数据接收完成
		Serial.print("Received: ");
		Serial.println((char*)rx_buffer);

		// 准备下一次事务
		memset(rx_buffer, 0, RX_BUF_SIZE);
		memset(&trans, 0, sizeof(trans));
		trans.length = RX_BUF_SIZE * 8;
		trans.rx_buffer = rx_buffer;
		trans.tx_buffer = NULL;
		spi_slave_queue_trans(SPI2_HOST, &trans, portMAX_DELAY);
	}

	delay(10);
}