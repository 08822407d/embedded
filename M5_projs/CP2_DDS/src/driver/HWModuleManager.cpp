#include "HWModuleManager.hpp"
#include "debug_utils.hpp"

// 创建并初始化 Manager（示例）
HWModuleManager DevModManager;

// 轮询任务函数
static void HardWarePollTask(void *pvParam) {
	HWModuleManager* mgr = static_cast<HWModuleManager*>(pvParam);
	const TickType_t period = pdMS_TO_TICKS(EVENT_POLL_INTERVAL_MS);
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		mgr->updateAll();
		vTaskDelayUntil(&lastWakeTime, period);
	}
}

void initHardWarePollTask() {
	MODULE_LOG_HEAD( HardWare Polling Task );

	// 通过 xTaskCreate() 创建轮询任务
	xTaskCreate(
		HardWarePollTask,				// 任务函数
		"HardWarePollingTask",		// 任务名称
		2048,						// 堆栈大小(根据需求调整)
		&DevModManager,				// 传递给任务的参数(指向manager)
		EVENT_TASK_PRIORITY,		// 任务优先级(宏定义)
		NULL						// 任务句柄(可选)
	);

	MODULE_LOG_TAIL( " ... Setup done\n" );
}