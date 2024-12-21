#include "ModuleManager.hpp"

// 创建并初始化 Manager（示例）
static ModuleManager manager;

// 轮询任务函数
static void ModulePollingTask(void *pvParameters) {
	// 从参数中取出 manager 指针
	ModuleManager *manager = static_cast<ModuleManager*>(pvParameters);

	// vTaskDelayUntil() 需要用到 tickCount 进行基准时间
	TickType_t lastWakeTime = xTaskGetTickCount();
	// 将毫秒转换为 FreeRTOS tick
	const TickType_t pollPeriodTicks = pdMS_TO_TICKS(MODULE_POLL_INTERVAL_MS);

	for (;;) {
		// 在这里进行模块的统一轮询
		if (manager) {
			manager->updateAll();  
		}

		// 阻塞直到下一个周期
		vTaskDelayUntil(&lastWakeTime, pollPeriodTicks);
	}
}

void initModulePollingTask() {
	// 通过 xTaskCreate() 创建轮询任务
	xTaskCreate(
		ModulePollingTask,			// 任务函数
		"ModulePollingTask",		// 任务名称
		2048,						// 堆栈大小(根据需求调整)
		&manager,					// 传递给任务的参数(指向manager)
		MODULE_TASK_PRIORITY,		// 任务优先级(宏定义)
		NULL						// 任务句柄(可选)
	);
}