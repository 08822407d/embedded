/* ---------------  src/main.cpp  --------------- */
#include <Arduino.h>

/* LetterShell 是 C 库，需要 extern "C" */
extern "C" {
#include "shell.h"
}

/* 若想把 shellTask 放在独立 FreeRTOS 任务中，打开下面宏 */
#define USE_SHELL_TASK   1

/* ---------- 全局对象 / 缓冲 ---------- */
static Shell shell;
static char  shellBuf[512];

/* ---------- read / write —— 满足 3.1 原型 ---------- */
static short shellWrite(char *data, unsigned short len)
{
	return Serial.write(reinterpret_cast<uint8_t *>(data), len);
}

static short shellRead(char *data, unsigned short len)
{
	return Serial.readBytes(reinterpret_cast<uint8_t *>(data), len);
}

/* ---------- 初始化并可选创建任务 ---------- */
void initLetterShell()
{
	Serial.begin(115200);

	shell.write = shellWrite;
	shell.read  = shellRead;
	shellInit(&shell, shellBuf, sizeof(shellBuf));

	/* shellTask() 已在库里实现；SHELL_TASK_WHILE 默认为 1 */
	xTaskCreatePinnedToCore(
		[](void *arg) {
			shellTask(static_cast<Shell *>(arg));   /* 无限循环 */
		},
		"LetterShell",
		2048,
		&shell,
		10,
		nullptr,
		APP_CPU_NUM              /* 固定到应用核，可改 */
	);
}

/* ---------- Arduino 入口 ---------- */
void setup()
{
	initLetterShell();
	pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
	vTaskDelay(1);               /* 让出 CPU */
}
/* ---------------------------------------------- */