#include <Arduino.h>
#include <esp_console.h>
#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>

/* ---------- 可选：自定义一个命令 ---------- */
int cmd_hello(int, char **) {
    printf("Hello from AtomS3U!\r\n");
    return 0;
}

static esp_console_repl_t *repl = nullptr;

void setup() {
    Serial.begin(115200);

    /* 1. REPL 配置 —— 保持默认 UART0，引脚不用设置 (-1 表示沿用) */
    esp_console_repl_config_t cfg  = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    cfg.prompt                     = ">$ ";
    cfg.max_cmdline_length         = 128;
    cfg.max_history_len            = 20;      // 内存历史 (不持久化)

    esp_console_dev_uart_config_t dev = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    /* dev.tx_gpio_num / rx_gpio_num 均为 -1，表示沿用当前 UART0 */

    /* 2. 创建并启动 REPL (任务在后台跑) */
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&dev, &cfg, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    /* 3. 注册 help 和自定义命令 */
    ESP_ERROR_CHECK(esp_console_register_help_command());

    const esp_console_cmd_t hello_cmd{
        .command = "hello",
        .help    = "print greeting",
        .func    = &cmd_hello,
    };
    esp_console_cmd_register(&hello_cmd);

    printf("\r\nType 'help' for list, UP/DOWN for history.\r\n");
}

void loop(void)
{
	delay(10);
}