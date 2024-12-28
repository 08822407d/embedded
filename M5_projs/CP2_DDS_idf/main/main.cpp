#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "init.hpp"


// 定义 setup() 和 loop() 函数
void setup() {
	// 初始化代码
	printf("Setup function called\n");

	Wire.begin(115200);
}

void loop() {
	while (1) {
		// 循环代码
		printf("Loop function running\n");

		// 模拟 Arduino 的 delay() 函数
		vTaskDelay(pdMS_TO_TICKS(1000)); // 延时 1000ms
	}
}

void app_main() {
	// 调用 setup() 函数
	setup();

	// 创建一个 FreeRTOS 任务，用于运行 loop() 函数
	xTaskCreate(
		[](void *param) {
			loop();
		},
		"loop_task", // 任务名
		2048,        // 堆栈大小
		NULL,        // 任务参数
		1,           // 任务优先级
		NULL         // 任务句柄
	);
}