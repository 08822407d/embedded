/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * @Hardwares: M5Core + Mini Unit OLED
 * @Platform Version: Arduino M5Stack Board Manager v2.1.3
 * @Dependent Library:
 * M5Stack@^0.4.6: https://github.com/m5stack/M5Stack
 * U8g2_Arduino: https://github.com/olikraus/U8g2_Arduino
 */

#include <Arduino.h>
#include <esp_console.h>
#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>
#include <LittleFS.h>                      // 用于保存历史

#include <Wire.h>
#include <U8g2lib.h>


#define SDA_PIN (2)
#define SCL_PIN (1)

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  // EastRising 0.42" OLED

static esp_console_repl_t *repl = nullptr;


/* ---------- 1. 用户自定义命令示例 ---------- */
int cmd_hello(int, char **)
{
	printf("Hello ESP-Console!\r\n");
	return 0;
}

void register_user_commands()
{
	const esp_console_cmd_t hello_cmd = {
		.command = "hello",
		.help    = "Print greeting",
		.func    = &cmd_hello,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&hello_cmd));
}

void setup(void)
{
	Serial.begin(115200);
	Wire.begin(SDA_PIN, SCL_PIN);
	u8g2.begin();

	// 2. 启用文件系统（保存历史用）
	LittleFS.begin(true);


	/* ---------- 3. console 基本配置 ---------- */
	esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	repl_cfg.prompt            = ">>>";      // 修改提示符
	repl_cfg.max_history_len   = 20;         // 内存中的历史条数
	repl_cfg.history_save_path = "/history.txt";

	esp_console_dev_usb_cdc_config_t dev = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&dev,&repl_cfg,&repl));
	ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void loop(void)
{
	u8g2.clearBuffer();                  // clear the internal memory
	u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
	u8g2.drawStr(10, 20, "Hello");       // write something to the internal memory
	u8g2.drawStr(10, 30, "M5Stack!");
	u8g2.sendBuffer();  // transfer internal memory to the display
	delay(1000);
	
	// Serial.println("Serial print test.");
}