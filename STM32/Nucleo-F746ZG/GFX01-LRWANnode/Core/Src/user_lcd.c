#include "app_display.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lcd_io.h"
#include "lvgl.h"

#include <src/misc/lv_rb.h>
#include <string.h>
#include <stdbool.h>


#define USE_LCD_DMA		1
#define GLOB_ROTATION	LCD_ORIENTATION_LANDSCAPE
#if (GLOB_ROTATION == LCD_ORIENTATION_LANDSCAPE) || (GLOB_ROTATION == LCD_ORIENTATION_LANDSCAPE_ROT180)
#	define HOR_RES      320
#	define VER_RES      240
#else
#	define HOR_RES      320
#	define VER_RES      240
#endif

extern osThreadId_t GuiTaskHandle;
extern osThreadId_t UserTaskHandle;
extern osTimerId_t LvglTickTimerHandle;

static uint32_t LCD_Width = 0;
static uint32_t LCD_Height = 0;
static uint32_t LCD_Orientation = 0;

/* 双缓冲多行块，32B 对齐便于 D-Cache 清理 */
static lv_color_t __attribute__((aligned(32))) buf1[HOR_RES * 60];
static lv_color_t __attribute__((aligned(32))) buf2[HOR_RES * 60];


/* ★ flush 回调：把 LVGL 画好的这块区域经 SPI-DMA 送给 LCD */
static void my_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
	const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
	const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
	// const uint32_t nbytes = w * h * 2U;               /* RGB565: 2字节/像素 */

	// /* F7: DMA 前清 D-Cache，避免发出旧数据 */
	// SCB_CleanDCache_by_Addr((uint32_t*)px_map, (int32_t)nbytes);

	/* 用 i-cube-display 的块写 API；UseDMA=1 走 SPI-DMA */
	/* Instance 一般为 0，与你生成的 BSP 工程保持一致 */
	(void)BSP_LCD_FillRGBRect(
		/*Instance=*/	0,
		/*UseDMA=*/		1,
		/*pData=*/		px_map,
		/*Xpos=*/		(uint32_t)area->x1,
		/*Ypos=*/		(uint32_t)area->y1,
		/*Width=*/		w,
		/*Height=*/		h
	);

	/* 大多数版本该 API 会“等 DMA 完成”再返回 → 直接标记就绪 */
	lv_display_flush_ready(disp);
}

/* ★ 注册显示：创建 display、设置 flush、设置 draw buffers（v9 写法） */
void lv_port_disp_init(void)
{
	/* 1) 创建 display（分辨率按你的面板方向） */
	lv_display_t * disp = lv_display_create(HOR_RES, VER_RES);

	/* 如需显式设颜色格式可加这一句；否则用 lv_conf.h 的 LV_COLOR_DEPTH=16 默认 */
	/* lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565); */

	/* 2) 绑定 flush 回调 */
	lv_display_set_flush_cb(disp, my_flush_cb);

	/* 3) 提供 draw buffers（双缓冲，按“部分刷新”模式） */
	lv_display_set_buffers(disp,
		buf1, buf2,									/* 两个缓冲 */
		sizeof(buf1),						/* 单个缓冲的字节数 */
		LV_DISPLAY_RENDER_MODE_PARTIAL	/* 分块渲染模式 */
	);
	lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
}

void gui_set_orientation(bool landscape)
{
    // /* 1) 确保在 GUI 线程/或持有 GUI 互斥，避免与正在刷新的 DMA 并发 */
    // // wait_for_prev_flush_done();  // 若你维护了“正在刷”的标志

    // /* 2) 切 LCD 方向（任选：重新 Init，或调用 SetOrientation） */
    // // BSP_LCD_DeInit(0);                      // 若有
    // BSP_LCD_Init(0, landscape ? ORIENT_LANDSCAPE : ORIENT_PORTRAIT);

    // /* 3) 更新 LVGL 分辨率，并保持不旋转 */
    // lv_display_t *disp = lv_display_get_default();
    // lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
    // lv_display_set_resolution(disp, landscape ? 320 : 240,
    //                                    landscape ? 240 : 320);
}



void MX_DISPLAY_PostInit(void)
{
	BSP_LCD_DeInit(0);
	if(BSP_LCD_Init(0, GLOB_ROTATION) != BSP_ERROR_NONE)
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
	if(BSP_LCD_DisplayOn(0) != BSP_ERROR_NONE)
	{
		Error_Handler();
	}
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

	osThreadFlagsSet(UserTaskHandle, 1U<<0);

	/* Infinite loop */
	for(;;)
	{
		lv_timer_handler();
		osDelay(5);
	}
	/* USER CODE END StartGUITask */
}


static const uint32_t colors_rgb888[] = {0x000000, 0xFFFFFF, 0xFF0000, 0x00FF00, 0x0000FF};
int color_idx = 0;
/* USER CODE BEGIN Header_StartUserTask */
/**
* @brief Function implementing the UserTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUserTask */
void StartUserTask(void *argument)
{
  /* USER CODE BEGIN StartUserTask */

  // 等待基础设施线程初始化全部完成
  osThreadFlagsWait(1U<<0, osFlagsWaitAny|osFlagsNoClear, osWaitForever);

  /* Infinite loop */
  for(;;)
  {
	lv_obj_t *scr = lv_screen_active();          // 取当前活动屏（根对象）
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(scr, lv_color_hex(colors_rgb888[color_idx]), 0);
	color_idx = (color_idx + 1) % (sizeof(colors_rgb888)/sizeof(colors_rgb888[0]));

	/* 可选：立即刷新一帧（否则等到下一次 lv_timer_handler() 节拍） */
	lv_refr_now(NULL);
	
	// osDelay(1);
  }
  /* USER CODE END StartUserTask */
}