#include "Joystick4WayButtons.hpp"

extern std::shared_ptr<AceButton> JoystickUpButton;
extern std::shared_ptr<AceButton> JoystickDownButton;
extern std::shared_ptr<AceButton> JoystickLeftButton;
extern std::shared_ptr<AceButton> JoystickRightButton;


static void checkAceButton(void) {
	// 添加回调函数
	JoystickUpButton->check();
	JoystickDownButton->check();
	JoystickLeftButton->check();
	JoystickRightButton->check();
}

// 轮询任务函数
static void AceButtonCheckTask(void *pvParam) {
	const TickType_t period = pdMS_TO_TICKS(ACEBUTTON_POLL_INTERVAL_MS);
	TickType_t lastWakeTime = xTaskGetTickCount();

	for (;;) {
		checkAceButton();
		vTaskDelayUntil(&lastWakeTime, period);
	}
}

void initJoystick4WayButtonsCheckTask() {
	// 通过 xTaskCreate() 创建轮询任务
	xTaskCreate(
		AceButtonCheckTask,			// 任务函数
		"AceButtonCheckTask",		// 任务名称
		2048,						// 堆栈大小(根据需求调整)
		NULL,						// 传递给任务的参数(指向manager)
		ACEBUTTON_TASK_PRIORITY,	// 任务优先级(宏定义)
		NULL						// 任务句柄(可选)
	);
}

