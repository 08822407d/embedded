#include "app_display.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lcd_io.h"
#include "lvgl.h"

#include <string.h>
#include <stdbool.h>


#define USE_LCD_DMA    1

/* RGB565：R(5) G(6) B(5) */
#define LCD565_BLACK   0x0000U
#define LCD565_WHITE   0xFFFFU
#define LCD565_RED     0xF800U
#define LCD565_GREEN   0x07E0U
#define LCD565_BLUE    0x001FU

extern osTimerId_t LvglTickTimerHandle;

static uint32_t LCD_Width = 0;
static uint32_t LCD_Height = 0;
static uint32_t LCD_Orientation = 0;

static uint8_t CacheBuffer[(320*2*BUFFER_CACHE_LINES)];



/* 双缓冲多行块，32B 对齐便于 D-Cache 清理 */
static lv_color_t __attribute__((aligned(32))) buf1[240 * 40];
static lv_color_t __attribute__((aligned(32))) buf2[240 * 40];

/* ★ flush 回调：把 LVGL 画好的这块区域经 SPI-DMA 送给 LCD */
static void my_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    const uint32_t nbytes = w * h * 2U;               /* RGB565: 2字节/像素 */

    /* F7: DMA 前清 D-Cache，避免发出旧数据 */
    SCB_CleanDCache_by_Addr((uint32_t*)px_map, (int32_t)nbytes);

    /* 用 i-cube-display 的块写 API；UseDMA=1 走 SPI-DMA */
    /* Instance 一般为 0，与你生成的 BSP 工程保持一致 */
    (void)BSP_LCD_FillRGBRect(/*Instance=*/0, /*UseDMA=*/1,
                              /*pData=*/px_map,
                              /*Xpos=*/(uint32_t)area->x1,
                              /*Ypos=*/(uint32_t)area->y1,
                              /*Width=*/w, /*Height=*/h);

    /* 大多数版本该 API 会“等 DMA 完成”再返回 → 直接标记就绪 */
    lv_display_flush_ready(disp);
}

/* ★ 注册显示：创建 display、设置 flush、设置 draw buffers（v9 写法） */
void lv_port_disp_init(void)
{
    /* 1) 创建 display（分辨率按你的面板方向） */
    lv_display_t * disp = lv_display_create(/*hor_res=*/240, /*ver_res=*/320);

    /* 如需显式设颜色格式可加这一句；否则用 lv_conf.h 的 LV_COLOR_DEPTH=16 默认 */
    /* lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565); */

    /* 2) 绑定 flush 回调 */
    lv_display_set_flush_cb(disp, my_flush_cb);

    /* 3) 提供 draw buffers（双缓冲，按“部分刷新”模式） */
    lv_display_set_buffers(disp,
                           buf1, buf2,                      /* 两个缓冲 */
                           sizeof(buf1),                    /* 单个缓冲的字节数 */
                           LV_DISPLAY_RENDER_MODE_PARTIAL); /* 分块渲染模式 */
}



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
	BSP_LCD_DeInit(0);
	if(BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE) != BSP_ERROR_NONE)
	{
		Error_Handler();
	}

	/* USER CODE BEGIN MX_DISPLAY_Init 1 */
	if((BSP_LCD_GetXSize(0, &LCD_Width) != BSP_ERROR_NONE)
		|| (BSP_LCD_GetYSize(0, &LCD_Height) != BSP_ERROR_NONE)
		|| (BSP_LCD_GetOrientation(0, &LCD_Orientation) != BSP_ERROR_NONE))
	{
		Error_Handler();
	}
	BSP_LCD_Clear(0, 0, 0, LCD_Width, LCD_Height);
	BSP_LCD_FillScreen(0, 0);
	if(BSP_LCD_DisplayOn(0) != BSP_ERROR_NONE)
	{
		Error_Handler();
	}

	BSP_LCD_FillScreen(0, LCD565_BLACK);
}

static const uint32_t colors[] = {LCD565_RED, LCD565_GREEN, LCD565_BLUE, LCD565_BLACK, LCD565_WHITE};
int color_idx = 0;
void ScreenFPS_test(void)
{
	BSP_LCD_FillScreen(0, colors[color_idx]);
	color_idx = (color_idx + 1) % (sizeof(colors)/sizeof(colors[0]));
}


/* LvglTick_Callback function */
void LvglTick_Callback(void *argument)
{
	/* USER CODE BEGIN LvglTick_Callback */
	lv_tick_inc(1);   // 每次回调推进 LVGL 1ms
	/* USER CODE END LvglTick_Callback */
}

/* USER CODE BEGIN Header_StartLCDTask */
/**
	* @brief  Function implementing the LCDTask thread.
	* @param  argument: Not used
	* @retval None
	*/
/* USER CODE END Header_StartLCDTask */
void StartGuiTask(void *argument)
{
	/* USER CODE BEGIN StartGUITask */

	MX_DISPLAY_PostInit();

	lv_init();                  /* 1) 初始化 LVGL 内核 */
	lv_port_disp_init();        /* 2) 注册显示驱动（见下） */

	/* 1 tick 周期启动：Tick rate = 1000Hz 时即 1ms */
	osTimerStart(LvglTickTimerHandle, 1);

	/* Infinite loop */
	for(;;)
	{
		// ScreenFPS_test();
		lv_timer_handler();
		osDelay(5);
	}
	/* USER CODE END StartGUITask */
}