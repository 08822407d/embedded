#include "app_display.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "key_io.h"
#include "lcd_io.h"

#include <string.h>
#include <stdbool.h>


#define USE_LCD_DMA                 0

/* RGB565：R(5) G(6) B(5) */
#define LCD565_BLACK   0x0000U
#define LCD565_WHITE   0xFFFFU
#define LCD565_RED     0xF800U
#define LCD565_GREEN   0x07E0U
#define LCD565_BLUE    0x001FU

static uint32_t LCD_Width = 0;
static uint32_t LCD_Height = 0;
static uint32_t LCD_Orientation = 0;

static uint8_t CacheBuffer[(320*2*BUFFER_CACHE_LINES)];



/**
	* @brief  Draws a full rectangle in currently active layer.
	* @param  Instance   LCD Instance
	* @param  Xpos X position
	* @param  Ypos Y position
	* @param  Width Rectangle width
	* @param  Height Rectangle height
	*/
static int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
	uint32_t size;
	uint32_t CacheLinesCnt, CacheLinesSz;
	uint32_t offset = 0, line_cnt = Ypos;

	size = (2*Width*Height);
	CacheLinesCnt = (Height > BUFFER_CACHE_LINES ? BUFFER_CACHE_LINES : Height);
	CacheLinesSz = (2*Width*CacheLinesCnt);
	memset(CacheBuffer, Color, CacheLinesSz);
#if (__CORTEX_M == 0x07) && defined(SCB_CCR_DC_Msk)
	if((SCB->CCR) & (uint32_t)SCB_CCR_DC_Msk)
	{
		SCB_CleanDCache_by_Addr((uint32_t*)CacheBuffer, CacheLinesSz);
	}
#endif

	while(1)
	{
		BSP_LCD_WaitForTransferToBeDone(0);
		if(BSP_LCD_FillRGBRect(Instance, USE_LCD_DMA, CacheBuffer, Xpos, line_cnt, Width, CacheLinesCnt) != BSP_ERROR_NONE)
		{
			Error_Handler();
		}
		line_cnt += CacheLinesCnt;
		offset += CacheLinesSz;
		/* Check remaining data size */
		if(offset == size)
		{
			/* last block transfer was done, so exit */
			break;
		}
		else if((offset + CacheLinesSz) > size)
		{
			/* Transfer last block and exit */
			CacheLinesCnt = ((size - offset)/ (2*Width));
			BSP_LCD_WaitForTransferToBeDone(0);
			if(BSP_LCD_FillRGBRect(Instance, USE_LCD_DMA, CacheBuffer, Xpos, line_cnt, Width, CacheLinesCnt) != BSP_ERROR_NONE)
			{
				Error_Handler();
			}
			break;
		}
	}
	BSP_LCD_WaitForTransferToBeDone(0);
	return BSP_ERROR_NONE;
}

void BSP_LCD_FillScreen(uint32_t Instance, uint32_t Color)
{
	BSP_LCD_FillRect(Instance, 0, 0, LCD_Width, LCD_Height, Color);
}

static void BSP_LCD_Clear(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height)
{
	BSP_LCD_FillRect(Instance, Xpos, Ypos, Width, Height, 0);
}


void MX_DISPLAY_PostInit(void)
{
	/* USER CODE BEGIN MX_DISPLAY_Init 1 */
	if((BSP_LCD_GetXSize(0, &LCD_Width) != BSP_ERROR_NONE) \
	|| (BSP_LCD_GetYSize(0, &LCD_Height) != BSP_ERROR_NONE) \
	|| (BSP_LCD_GetOrientation(0, &LCD_Orientation) != BSP_ERROR_NONE) )
	{
		Error_Handler();
	}
	BSP_LCD_Clear(0, 0, 0, LCD_Width, LCD_Height);
	BSP_LCD_FillScreen(0, 0);
	if(BSP_LCD_DisplayOn(0) != BSP_ERROR_NONE)
	{
		Error_Handler();
	}

	BSP_LCD_FillScreen(0, LCD565_WHITE);
}

static const uint32_t colors[] = {LCD565_RED, LCD565_GREEN, LCD565_BLUE, LCD565_BLACK, LCD565_WHITE};
int color_idx = 0;
void ScreenFPS_test(void)
{
	BSP_LCD_FillScreen(0, colors[color_idx]);
	color_idx = (color_idx + 1) % (sizeof(colors)/sizeof(colors[0]));
}

/* USER CODE BEGIN Header_StartGUITask */
/**
  * @brief  Function implementing the GUITask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartGUITask */
void StartGUITask(void *argument)
{
  /* USER CODE BEGIN StartGUITask */

	MX_DISPLAY_PostInit();

  /* Infinite loop */
  for(;;)
  {
		ScreenFPS_test();
    osDelay(1);
  }
  /* USER CODE END StartGUITask */
}