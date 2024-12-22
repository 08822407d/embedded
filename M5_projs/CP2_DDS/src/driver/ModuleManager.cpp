#include "ModuleManager.hpp"

// 创建并初始化 Manager（示例）
ModuleManager DevModManager;

// 轮询任务函数
static void EventPollTask(void *pvParam) {
	ModuleManager* mgr = static_cast<ModuleManager*>(pvParam);
	const TickType_t period = pdMS_TO_TICKS(EVENT_POLL_INTERVAL_MS);
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		mgr->updateAll();
		vTaskDelayUntil(&lastWakeTime, period);
	}
}

void initEventPollTask() {
	// 通过 xTaskCreate() 创建轮询任务
	xTaskCreate(
		EventPollTask,				// 任务函数
		"EventPollingTask",			// 任务名称
		2048,						// 堆栈大小(根据需求调整)
		&DevModManager,				// 传递给任务的参数(指向manager)
		EVENT_TASK_PRIORITY,		// 任务优先级(宏定义)
		NULL						// 任务句柄(可选)
	);
}