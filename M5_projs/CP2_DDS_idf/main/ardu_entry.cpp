#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "Wire.hpp"


// 定义 setup() 和 loop() 函数
extern "C" void setup() {
	// 初始化代码
	printf("Setup function called\n");
}

extern "C" void loop() {
	// 循环代码
	printf("Loop function running\n");

	// 模拟 Arduino 的 delay() 函数
	vTaskDelay(pdMS_TO_TICKS(1000)); // 延时 1000ms
}

