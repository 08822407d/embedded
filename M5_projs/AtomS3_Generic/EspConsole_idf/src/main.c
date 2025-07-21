#include <stdio.h>
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_chip_info.h"

static const char *TAG = "main";
static esp_console_repl_t *repl = NULL;

/* ---------- 示例命令：print chip info ---------- */
static int chip_info_cmd(int argc, char **argv)
{
	esp_chip_info_t chip;
	esp_chip_info(&chip);
	printf("Model: %s, Cores: %d, Revision: %d, Feature: %sPSRAM\n",
		   CONFIG_IDF_TARGET, chip.cores, chip.revision,
		   (chip.features & CHIP_FEATURE_EMB_PSRAM) ? "" : "no ");
	return 0;
}

static void register_commands(void)
{
	const esp_console_cmd_t cmd = {
		.command = "chip",
		.help    = "print ESP-Chip information",
		.func    = &chip_info_cmd,
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void app_main(void)
{
	/* 1. NVS 必须先初始化，否则后面 Wi-Fi/NVS 命令会失败 */
	ESP_ERROR_CHECK(nvs_flash_init());

	/* 2. 配置 REPL */
	esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	repl_cfg.prompt = "atom> ";
	repl_cfg.max_history_len = 20;

	esp_console_dev_usb_serial_jtag_config_t hw_cfg =
			ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

	/* 3. 创建并启动 REPL 任务 */
	ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_cfg,
														 &repl_cfg,
														 &repl));
	/* 内置 help 命令 */
	ESP_ERROR_CHECK(esp_console_register_help_command());
	/* 示例自定义命令 */
	register_commands();

	ESP_ERROR_CHECK(esp_console_start_repl(repl));

	printf("\nUSB-Serial-JTAG console ready, type 'help'.\n");
	/* REPL 在后台任务运行；app_main 退出即可 */
}