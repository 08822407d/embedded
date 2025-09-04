#include "app_freertos.h"
#include "cmsis_os2.h"
#include "stream_buffer.h"
#include "stm32u5xx_hal.h"
#include "usart.h"
#include "main.h"

#include <string.h>
#include <stdio.h>


#define MODEM_STREAM_SIZE   1024    /* 可按需要调大 */
#define MODEM_RX_CHUNK_MAX  256     /* 与 IDLE IT 的一次搬运最大值匹配 */

extern UART_HandleTypeDef hlpuart1;
#define UART_AT hlpuart1
extern UART_HandleTypeDef huart1;
#define UART_VCP huart1


/* —— 由 CubeMX 生成的 CMSIS 句柄 —— */
extern osMessageQueueId_t ModemEvtQHandle;  /* 你在 GUI 里添加的消息队列 */
extern osMutexId_t        ModemCmdLockHandle;
extern osEventFlagsId_t   ModemCmdEvtHandle; /* 若添加了事件标志 */

/* —— 我们自己创建的 StreamBuffer（ISR->任务的字节流通道） —— */
static StreamBufferHandle_t s_modemRxStream;
static uint8_t s_rxChunk[MODEM_RX_CHUNK_MAX]; /* 与 app_freertos 中的宏保持一致 */

/* 一些状态/统计（可选） */
static volatile uint32_t s_isrDropBytes = 0;



void Modem_RxStart_IT(void);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);




/* printf 重定向到 VCP */
int __io_putchar(int ch) {
	HAL_UART_Transmit(&UART_VCP, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}


/* 发送一条 AT 并等待终止状态（OK/ERROR） */
int Modem_SendAT_AndWaitOK(const char *cmd, uint32_t timeout_ms)
{
	/* 串行化命令 */
	osMutexAcquire(ModemCmdLockHandle, osWaitForever);

	/* 清事件标志位 */
	if (ModemCmdEvtHandle)
		osEventFlagsClear(ModemCmdEvtHandle, 0x1);

	/* 发送命令 + CRLF */
	static const uint8_t crlf[] = "\r\n";
	HAL_UART_Transmit(&hlpuart1, (uint8_t*)cmd, (uint16_t)strlen(cmd), 1000);
	HAL_UART_Transmit(&hlpuart1, (uint8_t*)crlf, sizeof(crlf)-1, 1000);

	/* 等待“终止状态行”唤醒 */
	uint32_t flags =
		osEventFlagsWait(ModemCmdEvtHandle, 0x1,
			osFlagsWaitAny | osFlagsNoClear, timeout_ms);
	osMutexRelease(ModemCmdLockHandle);

	if ((int32_t)flags < 0)
		return -2; /* 超时或错误 */
	/* 此处可扩展：在任务里记录最后一行是 OK 还是 ERROR。简单起见，返回 0 表示“看到终止行”。 */
	return 0;
}

/* USER CODE BEGIN Header_StartModemSvc */
/**
* @brief Function implementing the ModemSvc thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartModemSvc */
void StartModemSvc(void *argument)
{
	/* USER CODE BEGIN ModemSvc */

	/* 1) 创建 StreamBuffer（ISR 单生产者 -> 本任务单消费者） */
	s_modemRxStream = xStreamBufferCreate(MODEM_STREAM_SIZE, 1);
	configASSERT(s_modemRxStream);

	/* 2) 启动 LPUART1 的 ReceiveToIdle_IT —— 等会儿在 usart.c 的回调里往 s_modemRxStream 投递 */
extern void Modem_RxStart_IT(void);
	Modem_RxStart_IT();

	/* 3) 任务内：把字节流按 CRLF 切“行”，分流为：应答状态 / URC / 其他 */
	char    line[256];
	size_t  line_len = 0;
	uint8_t tmp[128];

	/* Infinite loop */
	for(;;)
	{
		size_t n = xStreamBufferReceive(s_modemRxStream, tmp, sizeof(tmp), pdMS_TO_TICKS(100));

		if (n == 0)
			continue;

		for (size_t i = 0; i < n; i++) {
			uint8_t ch = tmp[i];

			if (ch == '\r')
				continue;

			if (ch == '\n') {
				line[line_len] = '\0';

				if (line_len > 0) {
					/* —— 开发期：打印到 VCP；部署期：改成你的处理/转发 —— */
extern void Modem_LineSink(const char *s);
					Modem_LineSink(line);

					/* 分类：URC / 终止状态（OK/ERROR） / 普通数据行 */
					if (strncmp(line, "+EVT:", 5) == 0) {
						/* 投递整行到 CMSIS 消息队列（拷贝一份字符串） */
						(void)osMessageQueuePut(ModemEvtQHandle, line, 0, 0);
					} else if (strcmp(line, "OK") == 0 ||
											strstr(line, "ERROR") != NULL ||
											strncmp(line, "AT_", 3) == 0) {
						/* 唤醒正在等待命令完成的任务（事件标志 0x1） */
						if (ModemCmdEvtHandle)
							osEventFlagsSet(ModemCmdEvtHandle, 0x1);
					} else {
						/* 其他行：按需处理/缓存 */
					}
				}

				line_len = 0;
				continue;
			}

			if (line_len < sizeof(line) - 1) {
				line[line_len++] = (char)ch;
			} else {
				/* 行过长：丢弃到行尾 */
			}
		}
	}
	/* USER CODE END ModemSvc */
}


/* USER CODE BEGIN 1 */
void Modem_RxStart_IT(void)
{
	/* 在 ModemSvc 任务启动时调用，保证 s_modemRxStream 已创建 */
	HAL_UARTEx_ReceiveToIdle_IT(&hlpuart1, s_rxChunk, sizeof(s_rxChunk));
}

/* HAL 回调（统一 USART IRQ handler 内部调来）。仅做最小搬运 + 立刻重启 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart == &hlpuart1) {
		BaseType_t xHptw = pdFALSE;

		if (s_modemRxStream) {
			size_t sent = xStreamBufferSendFromISR(s_modemRxStream, s_rxChunk, Size, &xHptw);

			if (sent < Size) {
extern volatile uint32_t s_isrDropBytes;
				s_isrDropBytes += (Size - sent);
			}
		}

		HAL_UARTEx_ReceiveToIdle_IT(&hlpuart1, s_rxChunk, sizeof(s_rxChunk));
		portYIELD_FROM_ISR(xHptw);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == &hlpuart1) {
		/* 清错并继续收；HAL 已处理多数清旗语义 */
		HAL_UARTEx_ReceiveToIdle_IT(&hlpuart1, s_rxChunk, sizeof(s_rxChunk));
	}
}
/* USER CODE END 1 */






void Modem_LineSink(const char *s)  /* 当前是调试期“打印到 VCP”的后端 */
{
	/* USART1 阻塞发送打印；部署期可改为转发 CAN/SPI/以太网/写LCD等 */
	HAL_UART_Transmit(&huart1, (uint8_t*)"[LRWAN1] ", 9, 1000);
	HAL_UART_Transmit(&huart1, (uint8_t*)s, (uint16_t)strlen(s), 1000);
	HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 1000);
}

void LORA_serial_init(void)
{
	printf("\n\n\n[WL55] I-NUCLEO-LRWAN1 AT self-test starting...\n");

	/* 1) 基础连通性：AT */
	int retries = 3;
	while (retries-- > 0) {
		if (Modem_SendAT_AndWaitOK("AT", 2000) == 0) {
			break;
		}
		printf("[WARN] No response to AT, retry %d...", 3 - retries);
		HAL_Delay(1000);
		// osDelay(1000);
	}
	if (retries <= 0)
		printf("\n[FAIL] No OK to AT. Check LPUART1<-->D1/D0焊点、波特率、供电/天线。\n");

	/* 2) 可选：确认波特率设置（缺省115200，下面命令会返回OK） */
	if (Modem_SendAT_AndWaitOK("AT+UART=115200", 2000) == 0) {
		printf("\n[INFO] Baud reaffirmed to 115200.\n");
	}

	/* 3) 可选：读出一些标识（例如 AppEUI/DevEUI，USI AT手册支持无参查询） */
	Modem_SendAT_AndWaitOK("AT+APPEUI", 2000);
	Modem_SendAT_AndWaitOK("AT+NJM", 2000);      // Join 模式（仅查询，不入网）
	Modem_SendAT_AndWaitOK("AT+CLASS", 2000);    // 设备 Class

	printf("\n[WL55] Self-test done.\n");
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the defaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
	/* USER CODE BEGIN defaultTask */

	LORA_serial_init();

	/* Infinite loop */
	for(;;)
	{
		osDelay(100);
	}
	/* USER CODE END defaultTask */
}