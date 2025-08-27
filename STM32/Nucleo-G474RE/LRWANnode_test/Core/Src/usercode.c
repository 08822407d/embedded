#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>


/* 由 CubeMX 生成的句柄 */
extern UART_HandleTypeDef huart1;     // USART1 → LoRa AT 口
extern UART_HandleTypeDef hlpuart1;   // LPUART1 → VCP

/* printf 重定向到 VCP */
int __io_putchar(int ch) {
	HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}


/* 向 LoRa 模块发送一行 AT 命令（自动补 \r）并把收到的数据打印到 VCP。
 * 若在timeout_ms 内观察到 "OK" 子串，返回 0；否则返回 -1。
 */
#define UART_BUFFLEN	128
int External_AT_SendAndWaitOK(const char *cmd, uint32_t timeout_ms) {
	char line[UART_BUFFLEN];
	memset(line, 0, sizeof(line));
	size_t n = 0;
	uint8_t ch = 0;
	const uint8_t cr = '\r';

	/* 发送命令 + CR（USI 手册命令以 <CR> 结尾） */
	HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart1, (uint8_t *)&cr, 1, HAL_MAX_DELAY);
	printf(">> %s\r\n", cmd);

	uint32_t t0 = HAL_GetTick();
	while ((HAL_GetTick() - t0) < timeout_ms) {
		if (HAL_UART_Receive(&huart1, &ch, 1, 100) == HAL_OK) {
			/* 透传到 PC 串口监视器 */
			char digit[4];
			memset(digit, 0, sizeof(digit));
			sprintf(digit, "%02X ", ch);
			// HAL_UART_Transmit(&hlpuart1, &ch, 1, HAL_MAX_DELAY);
			HAL_UART_Transmit(&hlpuart1, (uint8_t *)&digit, strlen(digit), HAL_MAX_DELAY);

			if (n < sizeof(line) - 1)
				line[n++] = (char)ch;
			line[n] = '\0';

			/* 简单判定：出现 "OK" 即认为成功 */
			if (strstr(line, "OK") != NULL) {
				return 0;
			}

			/* 换行则清一行，继续收 */
			if (ch == '\n' || ch == '\r') {
				n = 0;
				line[0] = '\0';
			}
		}
	}
	return -1;
}


/* 重新初始化 USART1：修改波特率/帧格式后生效
 * 仅用于阻塞式; 若你在用中断/DMA, 需要先停用相关通道/回调再改
 */
HAL_StatusTypeDef Reinit_USART1(uint32_t baud,
																uint32_t wordlen,   // UART_WORDLENGTH_8B
																uint32_t stopbits,  // UART_STOPBITS_1
																uint32_t parity)    // UART_PARITY_NONE
{
	HAL_StatusTypeDef st;

	/* 停用外设 */
	st = HAL_UART_DeInit(&huart1);
	if (st != HAL_OK) return st;

	/* 更新参数（保留其它设置为默认/安全值） */
	huart1.Init.BaudRate     = baud;
	huart1.Init.WordLength   = wordlen;
	huart1.Init.StopBits     = stopbits;
	huart1.Init.Parity       = parity;
	huart1.Init.Mode         = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
#if defined(UART_ONE_BIT_SAMPLE_DISABLE)
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
#endif

	/* 确保未启用任何“Advanced Feature”以免反相/交换 */
#if defined(HAL_UARTEx_GetTxRxPinsSwap)
	HAL_UARTEx_DisableFifoMode(&huart1); // 若存在 FIFO API
#endif
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	/* 重新初始化外设 */
	st = HAL_UART_Init(&huart1);
	return st;
}

void StartDefaultTask(void *argument)
{
	/* USER CODE BEGIN StartDefaultTask */

	printf("\n[WL55] I-NUCLEO-LRWAN1 AT self-test starting...\n");

	/* 1) 基础连通性：AT */
	const int rates[] = {115200, 57600, 38400, 19200, 9600};
	for (int i = 0; i < sizeof(rates)/sizeof(rates[0]); ++i) {
		Reinit_USART1(rates[i], UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE);
		if (External_AT_SendAndWaitOK("AT", 1000) == 0) {
			printf("Found %d\r\n", rates[i]);
			break;
		}
	}
	if (External_AT_SendAndWaitOK("AT", 2000) == 0) {
		printf("\n[PASS] AT -> OK\n");
	} else {
		printf("\n[FAIL] No OK to AT. Check USART1<-->D1/D0\n");
	}

	/* 2) 可选：确认波特率设置（缺省115200，下面命令会返回OK） */
	if (External_AT_SendAndWaitOK("AT+UART=115200", 2000) == 0) {
		printf("\n[INFO] Baud reaffirmed to 115200.\n");
	}

	/* 3) 可选：读出一些标识（例如 AppEUI/DevEUI，USI AT手册支持无参查询） */
	External_AT_SendAndWaitOK("AT+APPEUI", 2000);
	External_AT_SendAndWaitOK("AT+NJM", 2000);      // Join 模式（仅查询，不入网）
	External_AT_SendAndWaitOK("AT+CLASS", 2000);    // 设备 Class

	printf("\n[WL55] Self-test done.\n");

	/* Infinite loop */
	for(;;)
	{
		printf("Debug Serial test\r\n");
		osDelay(1000);
	}
	/* USER CODE END StartDefaultTask */
}