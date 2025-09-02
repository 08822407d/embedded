#include "app_freertos.h"
#include "cmsis_os2.h"
#include "stream_buffer.h"

#include "main.h"
#include "stm32u5xx_hal.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>


extern UART_HandleTypeDef hlpuart1;
#define UART_AT hlpuart1
extern UART_HandleTypeDef huart1;
#define UART_VCP huart1


/* printf 重定向到 VCP */
int __io_putchar(int ch) {
	HAL_UART_Transmit(&UART_VCP, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}

// #define PRINT_HEX
#define UART_BUFFLEN	128
/* 向 LoRa 模块发送一行 AT 命令（自动补 \r）并把收到的数据打印到 VCP。
 * 若在timeout_ms 内观察到 "OK" 子串，返回 0；否则返回 -1。
 */
int External_AT_SendAndWaitOK(const char *cmd, uint32_t timeout_ms) {
	char line[UART_BUFFLEN];
	memset(line, 0, sizeof(line));
	size_t n = 0;
	uint8_t ch = 0;
	const uint8_t cr = '\r';
	const uint8_t crlf[2] = "\r\n";

	/* 发送命令 + CR（USI 手册命令以 <CR> 结尾） */
	HAL_UART_Transmit(&hlpuart1, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
	// HAL_UART_Transmit(&hlpuart1, (uint8_t *)&cr, 1, HAL_MAX_DELAY);
	HAL_UART_Transmit(&hlpuart1, (uint8_t *)crlf, 2, HAL_MAX_DELAY);
	printf("\n>> %s\r\n", cmd);

	uint32_t t0 = HAL_GetTick();
	while ((HAL_GetTick() - t0) < timeout_ms) {
		if (HAL_UART_Receive(&hlpuart1, &ch, 1, 100) == HAL_OK) {
#ifdef PRINT_HEX
			// 以防乱码直接打印十六进制
			char digit[4];
			memset(digit, 0, sizeof(digit));
			sprintf(digit, "%02X ", ch);
			HAL_UART_Transmit(&huart1, (uint8_t *)&digit, strlen(digit), HAL_MAX_DELAY);
#else
			/* 透传到 PC 串口监视器 */
			HAL_UART_Transmit(&huart1, &ch, 1, HAL_MAX_DELAY);
#endif // PRINT_HEX

			if (n < sizeof(line) - 1)
				line[n++] = (char)ch;
			line[n] = '\0';

			/* 简单判定：出现 "OK" 即认为成功 */
			if (strstr(line, "OK") != NULL) {
				printf("\n");
				return 0;
			}

			/* 换行则清一行，继续收 */
			if (ch == '\n' || ch == '\r') {
				n = 0;
				line[0] = '\0';
			}
		}
	}
	printf("\nERR STR:%s\n", line);
	return -1;
}


void LORA_serial_init(void)
{
	printf("\n\n\n[WL55] I-NUCLEO-LRWAN1 AT self-test starting...\n");
	/* 1) 基础连通性：AT */
	int retries = 3;
	while (retries-- > 0) {
		if (External_AT_SendAndWaitOK("AT", 2000) == 0) {
			break;
		}
		printf("[WARN] No response to AT, retry %d...", 3 - retries);
		HAL_Delay(1000);
		// osDelay(1000);
	}
	if (retries <= 0)
		printf("\n[FAIL] No OK to AT. Check LPUART1<-->D1/D0焊点、波特率、供电/天线。\n");

	/* 2) 可选：确认波特率设置（缺省115200，下面命令会返回OK） */
	if (External_AT_SendAndWaitOK("AT+UART=115200", 2000) == 0) {
		printf("\n[INFO] Baud reaffirmed to 115200.\n");
	}

	/* 3) 可选：读出一些标识（例如 AppEUI/DevEUI，USI AT手册支持无参查询） */
	External_AT_SendAndWaitOK("AT+APPEUI", 2000);
	External_AT_SendAndWaitOK("AT+NJM", 2000);      // Join 模式（仅查询，不入网）
	External_AT_SendAndWaitOK("AT+CLASS", 2000);    // 设备 Class

	External_AT_SendAndWaitOK("AT+APPEUI", 2000);
	External_AT_SendAndWaitOK("AT+NJM", 2000);      // Join 模式（仅查询，不入网）
	External_AT_SendAndWaitOK("AT+CLASS", 2000);    // 设备 Class

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
		// printf("dbg-print test\n");
		osDelay(1000);
	}
	/* USER CODE END defaultTask */
}