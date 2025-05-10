#include "Joystick4WayButtons.hpp"
#include "debug_utils.hpp"


extern std::shared_ptr<AceButton> Joystick4WayButton;
TaskHandle_t AceButtonCheckTaskHandle = nullptr;

static void checkAceButton(void) {
	// 添加回调函数
	Joystick4WayButton->check();
}

// 轮询任务函数
static void AceButtonCheckTask(void *pvParam) {
	const TickType_t period = pdMS_TO_TICKS(ACEBUTTON_POLL_INTERVAL_MS);
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(AceButtonCheckTaskHandle);
		size_t freeHeap = xPortGetFreeHeapSize();
		Serial.print("AceButtonCheckTask stack high water mark: ");
		Serial.print(stackHighWaterMark);
		Serial.print(", free heap: ");
		Serial.println(freeHeap);

		checkAceButton();
		vTaskDelayUntil(&lastWakeTime, period);
	}
}

void initJoystick4WayButtonsCheckTask() {
	MODULE_LOG_HEAD( 4WayButtons Polling Task );

	// 通过 xTaskCreate() 创建轮询任务
	xTaskCreate(
		AceButtonCheckTask,			// 任务函数
		"AceButtonCheckTask",		// 任务名称
		4096,						// 堆栈大小(根据需求调整)
		NULL,						// 传递给任务的参数(指向manager)
		ACEBUTTON_TASK_PRIORITY,	// 任务优先级(宏定义)
		&AceButtonCheckTaskHandle	// 任务句柄(可选)
	);

	MODULE_LOG_TAIL( " ... Setup done\n" );
}

