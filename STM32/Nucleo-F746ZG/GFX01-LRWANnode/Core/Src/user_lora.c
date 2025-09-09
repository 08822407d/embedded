#include "main.h"
#include "usart.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>



/* === 配置参数（可按需调整） === */
#ifndef MODEM_LPUART_RX_DMA_BUF_SIZE
#	define MODEM_LPUART_RX_DMA_BUF_SIZE   2048u    /* 环形 DMA 缓冲大小（建议 2~4KB 起） */
#endif

/* 初始化：在 MX_GPIO_Init()/MX_LPUART1_UART_Init() 之后调用一次 */
void ModemUartDma_Init(void);

/* 供 IRQ 回调调用：DMA 半满/全满、UART IDLE/RTO/错误 */
void ModemUartDma_OnDmaHalf(void);
void ModemUartDma_OnDmaFull(void);
void ModemUartDma_OnUartIdleOrRto(void);
void ModemUartDma_OnUartError(uint32_t errcode);

/* === 上层读取接口（无协议，仅字节流） === */

/* 当前可读的字节总数 */
size_t ModemUartRx_Available(void);

/* 简单拷贝式读取：把最多 max 字节拷到 dst，返回实际拷贝数 */
size_t ModemUartRx_Read(uint8_t *dst, size_t max);

/* 零拷贝获取：给出“本次新增区间”的两个连续段（可能第二段为 0）
 * 用完后必须调用 Release(consumed) 告知消费了多少字节
 */
size_t ModemUartRx_Claim(const uint8_t **p1,
		size_t *n1, const uint8_t **p2, size_t *n2);
void   ModemUartRx_Release(size_t consumed);

/* 发送 AT 命令：阻塞式 TX（DMA 对 AT 没必要；如需可扩展） */
HAL_StatusTypeDef ModemUart_TxBlocking(const uint8_t *data,
		uint16_t len, uint32_t timeout_ms);




/* 把环形 DMA 缓冲放到非缓存段（F7 有 D-Cache） */
#if defined ( __ICCARM__ )
__no_init __root static uint8_t s_rxDmaBuf[MODEM_LPUART_RX_DMA_BUF_SIZE] @ ".dma_buffer";
#elif defined ( __GNUC__ )
__attribute__((section(".dma_buffer"))) static uint8_t s_rxDmaBuf[MODEM_LPUART_RX_DMA_BUF_SIZE];
#else
static uint8_t s_rxDmaBuf[MODEM_LPUART_RX_DMA_BUF_SIZE];
#endif

static volatile size_t s_rIdx = 0;
static volatile uint32_t s_overwriteWarn = 0, s_errorCount = 0;

static inline size_t dma_write_index(void)
{
	DMA_HandleTypeDef *hdma = huart6.hdmarx;
	size_t ndtr = __HAL_DMA_GET_COUNTER(hdma);
	size_t w = MODEM_LPUART_RX_DMA_BUF_SIZE - ndtr;
	if (w >= MODEM_LPUART_RX_DMA_BUF_SIZE) w = 0;
	return w;
}

static inline size_t contiguous1_len(size_t r, size_t w)
{
	if (w >= r) return w - r;
	return MODEM_LPUART_RX_DMA_BUF_SIZE - r;
}

void ModemUartDma_Init(void)
{
	/* 启动 USART6 RX DMA（Circular） */
	HAL_UART_Receive_DMA(&huart6, s_rxDmaBuf, MODEM_LPUART_RX_DMA_BUF_SIZE);

	/* 开 IDLE 中断 */
	__HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);

#if defined(UART_IT_RTO) && defined(UART_FLAG_RTOF)
	/* 可选：Receiver Timeout（若 HAL/芯片支持） */
	__HAL_UART_CLEAR_FLAG(&huart6, UART_CLEAR_RTOF);
	/* 若有 Ex API 更好：HAL_UARTEx_ReceiverTimeout_Config(&huart6, ticks); HAL_UARTEx_EnableReceiverTimeout(&huart6); */
	__HAL_UART_ENABLE_IT(&huart6, UART_IT_RTO);
#endif

	s_rIdx = 0;
}

void ModemUartDma_OnDmaHalf(void) { /* 可在此发任务通知 */ }
void ModemUartDma_OnDmaFull(void) { /* 可在此发任务通知 */ }
void ModemUartDma_OnUartIdleOrRto(void) { /* 同上，留空或发通知 */ }
void ModemUartDma_OnUartError(uint32_t errcode) { (void)errcode; s_errorCount++; }

size_t ModemUartRx_Available(void)
{
	size_t w = dma_write_index(), r = s_rIdx;
	if (w >= r)
		return w - r;
	return (MODEM_LPUART_RX_DMA_BUF_SIZE - r) + w;
}

size_t ModemUartRx_Claim(const uint8_t **p1, size_t *n1,
						 const uint8_t **p2, size_t *n2)
{
	size_t w = dma_write_index(), r = s_rIdx;
	size_t c1 = contiguous1_len(r, w);
	size_t total = ModemUartRx_Available();

#if defined (SCB_InvalidateDCache_by_Addr)
	/* 若该段是 cacheable，需要在读前做 invalidate；更推荐把 .dma_buffer 放到 DTCM/非缓存区 */
	/* SCB_InvalidateDCache_by_Addr((void*)&s_rxDmaBuf[0], MODEM_LPUART_RX_DMA_BUF_SIZE); */
#endif

	if (c1) {
		*p1 = &s_rxDmaBuf[r]; *n1 = c1;
	} else {
		*p1 = NULL; *n1 = 0;
	}

	if (w < r) {
		*p2 = &s_rxDmaBuf[0]; *n2 = w;
	} else {
		*p2 = NULL; *n2 = 0;
	}

	return total;
}

void ModemUartRx_Release(size_t consumed)
{
	size_t avail = ModemUartRx_Available();
	if (consumed > avail)
		consumed = avail;
	s_rIdx += consumed;
	if (s_rIdx >= MODEM_LPUART_RX_DMA_BUF_SIZE)
		s_rIdx -= MODEM_LPUART_RX_DMA_BUF_SIZE;
}

size_t ModemUartRx_Read(uint8_t *dst, size_t max)
{
	if (!dst || !max)
		return 0;
	const uint8_t *p1,*p2; size_t n1,n2;
	size_t total = ModemUartRx_Claim(&p1,&n1,&p2,&n2);

	size_t need = (total < max) ? total : max, cpy = 0;
	size_t k = (n1 < need) ? n1 : need;

	if (k && p1){
		memcpy(dst, p1, k);
		cpy += k;
	}

	if (cpy < need && n2 && p2) {
		size_t k2 = need - cpy;
		if (k2>n2)
			k2=n2;
		memcpy(dst+cpy, p2, k2);
		cpy += k2;
	}

	ModemUartRx_Release(cpy);

	return cpy;
}

HAL_StatusTypeDef ModemUart_TxBlocking(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
	if (!data || !len)
		return HAL_OK;
	return HAL_UART_Transmit(&huart6, (uint8_t*)data, len, timeout_ms);
}


/* 在 MX_USART6_UART_Init() 后某处调用一次： */
void Modem_LowLevel_Start(void)
{
	ModemUartDma_Init();
}

/* DMA 回调（HAL 从 DMA IRQ 调用） */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart6)
		ModemUartDma_OnDmaHalf();
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart6)
		ModemUartDma_OnDmaFull();
}

/* 错误回调 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart6)
		ModemUartDma_OnUartError(huart->ErrorCode);
}

// /* IDLE/RTO 标志处理：放在 USART6_IRQHandler 的 USER CODE 区 */
// void USART6_IRQHandler(void)
// {
// 	/* USER CODE BEGIN USART6_IRQn 0 */
// 	if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE) &&
// 			__HAL_UART_GET_IT_SOURCE(&huart6, UART_IT_IDLE)) {
// 		__HAL_UART_CLEAR_IDLEFLAG(&huart6);
// 		ModemUartDma_OnUartIdleOrRto();
// 	}
// #ifdef UART_FLAG_RTOF
// 	if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_RTOF) &&
// 			__HAL_UART_GET_IT_SOURCE(&huart6, UART_IT_RTO)) {
// 		__HAL_UART_CLEAR_FLAG(&huart6, UART_CLEAR_RTOF);
// 		ModemUartDma_OnUartIdleOrRto();
// 	}
// #endif
// 	/* USER CODE END USART6_IRQn 0 */

// 	HAL_UART_IRQHandler(&huart6);

// 	/* USER CODE BEGIN USART6_IRQn 1 */
// 	/* 可选：把 IDLE/RTO 放在 HAL 调用之后也行，注意不要重复清标志 */
// 	/* USER CODE END USART6_IRQn 1 */
// }