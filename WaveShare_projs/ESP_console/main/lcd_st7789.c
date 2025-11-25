#include <stdio.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#define LCD_HOST           SPI2_HOST

#define PIN_NUM_MOSI       2
#define PIN_NUM_SCLK       3
#define PIN_NUM_CS         4
#define PIN_NUM_DC         5
#define PIN_NUM_RST        8
#define PIN_NUM_BK_LIGHT   49

#define LCD_H_RES          240
#define LCD_V_RES          240

static const char *TAG = "st7789";
static esp_lcd_panel_handle_t panel_handle = NULL;

void lcd_st7789_init(void)
{
	ESP_LOGI(TAG, "Init backlight GPIO");
	gpio_config_t bk_gpio_config = {
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT,
	};
	ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
	gpio_set_level(PIN_NUM_BK_LIGHT, 1);   // 打开背光

	ESP_LOGI(TAG, "Init SPI bus");
	spi_bus_config_t buscfg = {
		.sclk_io_num = PIN_NUM_SCLK,
		.mosi_io_num = PIN_NUM_MOSI,
		.miso_io_num = -1,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = LCD_H_RES * 40 * 2 + 10, // 40 行缓冲
	};
	ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

	ESP_LOGI(TAG, "Attach LCD to SPI bus");
	esp_lcd_panel_io_handle_t io_handle = NULL;
	esp_lcd_panel_io_spi_config_t io_config = {
		.dc_gpio_num = PIN_NUM_DC,
		.cs_gpio_num = PIN_NUM_CS,
		.pclk_hz = 40 * 1000 * 1000,       // 根据屏规格书调整
		.lcd_cmd_bits = 8,
		.lcd_param_bits = 8,
		.spi_mode = 0,
		.trans_queue_depth = 10,
		.on_color_trans_done = NULL,
		.user_ctx = NULL,
	};
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
											 &io_config, &io_handle));

	ESP_LOGI(TAG, "Install ST7789 panel driver");
	esp_lcd_panel_dev_config_t panel_config = {
		.reset_gpio_num = PIN_NUM_RST,
		.color_space = ESP_LCD_COLOR_SPACE_RGB,
		.bits_per_pixel = 16,
	};
	ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

	// 根据实际排线做镜像/旋转
	ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
	ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));

	ESP_LOGI(TAG, "ST7789 init done");
}

// 一个简单的清屏函数
void lcd_st7789_fill(uint16_t color)
{
	if (!panel_handle) {
		return;
	}
	static uint16_t line_buf[LCD_H_RES * 20]; // 一次画 20 行
	for (int i = 0; i < LCD_H_RES * 20; i++) {
		line_buf[i] = color;
	}
	for (int y = 0; y < LCD_V_RES; y += 20) {
		int y_end = (y + 20) > LCD_V_RES ? LCD_V_RES : (y + 20);
		ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle,
						0, y, LCD_H_RES, y_end, line_buf));
	}
}