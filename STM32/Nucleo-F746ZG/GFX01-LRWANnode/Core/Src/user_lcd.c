#include "app_display.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lcd_io.h"
#include "lvgl.h"

#include <src/misc/lv_rb.h>
#include <stdarg.h>
#include <stdio.h>
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

extern void init_lvterm(void);
extern void lvterm_write_async(const char *s);
extern void lvterm_printf_async(const char *fmt, ...);


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
	init_lvterm();

	osThreadFlagsSet(UserTaskHandle, 1U<<0);

	/* Infinite loop */
	for(;;)
	{
		uint32_t wait_ms = lv_timer_handler();
		osDelay(wait_ms);

		// lv_timer_handler();
		// osDelay(5);
	}
	/* USER CODE END StartGUITask */
}


// static const uint32_t colors_rgb888[] = {0x000000, 0xFFFFFF, 0xFF0000, 0x00FF00, 0x0000FF};
// int color_idx = 0;
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

	// lv_obj_t *scr = lv_screen_active();          // 取当前活动屏（根对象）
	// lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	// lv_obj_set_style_bg_color(scr, lv_color_hex(0), 0);
	// /* 可选：立即刷新一帧（否则等到下一次 lv_timer_handler() 节拍） */
	// lv_refr_now(NULL);

	int i = 0;
	/* Infinite loop */
	for(;;)
	{
		lvterm_printf_async("msg %02d value=%d", i, 123+i);
		i++;
		osDelay(100);
	}
	/* USER CODE END StartUserTask */
}








/* ==== 可调默认值 ==== */
#ifndef LVTERM_KEEP_LINES_DEFAULT
#define LVTERM_KEEP_LINES_DEFAULT     50
#endif
#ifndef LVTERM_QUEUE_LEN_DEFAULT
#define LVTERM_QUEUE_LEN_DEFAULT      32
#endif
#ifndef LVTERM_MAX_MSG_CHARS_DEFAULT
#define LVTERM_MAX_MSG_CHARS_DEFAULT  64
#endif

typedef struct {
	uint16_t keep_lines;                 // 保留的“内容行”数，默认 50
	lv_color_t fg;                       // 前景色（文字色），默认 0x00FF00 绿
	lv_color_t bg;                       // 背景色，默认 0x000000 黑
	const lv_font_t *font;               // 字体；若为 NULL 则用 LV_FONT_DEFAULT
	uint16_t queue_len;                  // 打印消息队列长度（条），默认 32
	uint16_t max_msg_chars;              // 每条消息最大字符（含可选自动补 '\n'），默认 64
	bool     auto_append_newline;        // 若消息尾部无 '\n' 是否自动补，默认 true
} lvterm_cfg_t;


/* ==== 内部状态 ==== */
static lv_obj_t *s_ta = NULL;
static lvterm_cfg_t s_cfg;
static osMessageQueueId_t s_msgq = NULL;

/* 队列元素：指向动态分配的以 '\0' 结尾的字符串 */
typedef char* lvterm_msg_t;

/* GUI 线程里的 LVGL 定时器：周期拉取队列并写入 textarea */
static void lvterm_drain_timer_cb(lv_timer_t *t);

/* ==== 工具函数 ==== */

static void scroll_to_bottom(void)
{
	lv_textarea_set_cursor_pos(s_ta, LV_TEXTAREA_CURSOR_LAST);                     // 光标到末尾
	// lv_obj_scroll_to_y(s_ta, LV_COORD_MAX, LV_ANIM_OFF);                           // 视口滚到底
	int32_t lh = lv_font_get_line_height(lv_obj_get_style_text_font(s_ta, LV_PART_MAIN));
	lv_obj_scroll_by(s_ta, 0, lh, LV_ANIM_ON);   // 下滚一个行高（平滑）
}

/* 保留最近 keep_lines 条“内容行”（按 '\n' 统计） */
static void trim_keep_last_lines(uint16_t keep_lines)
{
	const char *txt = lv_textarea_get_text(s_ta);
	if(!txt) return;

	/* 统计现有行数 */
	size_t total_lines = 0;
	for(const char *p = txt; *p; ++p) if(*p == '\n') ++total_lines;

	if(total_lines <= keep_lines) return;

	size_t drop = total_lines - keep_lines;

	/* 找到应保留文本的起始位置：跳过 drop 个 '\n' 之后 */
	const char *p = txt;
	while(drop && (p = strchr(p, '\n'))) { ++p; --drop; }

	if(p && *p) {
		lv_textarea_set_text(s_ta, p);                                             // 直接重设为子串
		scroll_to_bottom();                                                        // 裁剪后保持在底部
	}
}

/* 在 GUI 线程里消费一条队列消息（若存在），返回是否消费到 */
static bool lvterm_try_consume_one(void)
{
	lvterm_msg_t m = NULL;
	osStatus_t st = osMessageQueueGet(s_msgq, &m, NULL, 0);                        // 0 超时=非阻塞
	if(st != osOK || !m) return false;

	/* 1) 追加文本 */
	lv_textarea_add_text(s_ta, m);

	/* 2) 如需自动补换行（只在创建时设置一次），这里一般已经是带 \n 的整行 */
	if(s_cfg.auto_append_newline) {
		size_t n = strlen(m);
		if(n == 0 || m[n-1] != '\n') {
			lv_textarea_add_text(s_ta, "\n");
		}
	}

	/* 3) 滚到底 + 懒裁剪（仅在超限时执行一次 set_text） */
	scroll_to_bottom();
	trim_keep_last_lines(s_cfg.keep_lines);

	/* 4) 释放消息 */
	lv_free(m);
	return true;
}

static void lvterm_drain_timer_cb(lv_timer_t *t)
{
	LV_UNUSED(t);
	/* 每次尽量清空队列，避免积压 */
	while(lvterm_try_consume_one()) { /* loop */ }
}

/* ==== 对外 API ==== */

lv_obj_t *lvterm_create(lv_obj_t *parent, const lvterm_cfg_t *cfg)
{
	/* 1) 保存配置（填默认值） */
	memset(&s_cfg, 0, sizeof(s_cfg));
	s_cfg.keep_lines         = cfg && cfg->keep_lines        ? cfg->keep_lines        : LVTERM_KEEP_LINES_DEFAULT;
	s_cfg.fg                 = cfg ? cfg->fg : lv_color_hex(0x00FF00);               // 绿
	s_cfg.bg                 = cfg ? cfg->bg : lv_color_hex(0x000000);               // 黑
	s_cfg.font               = cfg ? cfg->font : NULL;
	s_cfg.queue_len          = cfg && cfg->queue_len         ? cfg->queue_len         : LVTERM_QUEUE_LEN_DEFAULT;
	s_cfg.max_msg_chars      = cfg && cfg->max_msg_chars     ? cfg->max_msg_chars     : LVTERM_MAX_MSG_CHARS_DEFAULT;
	s_cfg.auto_append_newline= cfg ? cfg->auto_append_newline: true;

	/* 2) 创建 textarea */
	s_ta = lv_textarea_create(parent ? parent : lv_screen_active());
	lv_obj_set_size(s_ta, LV_PCT(100), LV_PCT(100));
	lv_textarea_set_one_line(s_ta, false);                                           // 多行，允许换行
	lv_textarea_set_text(s_ta, "");

	/* 3) 样式：绿字黑底 + 字体（若给定） */
	lv_obj_set_style_bg_color   (s_ta, s_cfg.bg, 0);
	lv_obj_set_style_text_color (s_ta, s_cfg.fg, 0);
	if(s_cfg.font) lv_obj_set_style_text_font(s_ta, s_cfg.font, 0);                 // 未给则沿用全局默认字体
	lv_obj_set_scrollbar_mode(s_ta, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_clear_flag(s_ta, LV_OBJ_FLAG_CLICKABLE);                                 // 只显示输出，不可编辑
	// 如需隐藏光标，可再禁用光标可见性：
	// lv_obj_set_style_opa(s_ta, LV_OPA_0, LV_PART_CURSOR);

	/* 4) 创建 FreeRTOS 消息队列：元素是“char*” */
	if(!s_msgq) {
		const osMessageQueueAttr_t attr = { .name = "lvtermq" };
		s_msgq = osMessageQueueNew(s_cfg.queue_len, sizeof(lvterm_msg_t), &attr);
	}

	/* 5) 创建一个 LVGL 定时器（在 GUI 线程里运行）用来“抽干队列” */
	lv_timer_create(lvterm_drain_timer_cb, 10, NULL);                                // 每 10ms 拉取；你也可设 0 让其每 tick 都跑

	/* 6) 初始滚到底 */
	scroll_to_bottom();

	return s_ta;
}

/* 任意任务可调用：复制字符串 -> 投递到队列；由 GUI 线程消费 */
void lvterm_write_async(const char *s)
{
	if(!s_msgq) return;
	/* 限长拷贝，始终确保以 '\0' 结尾 */
	size_t n = strlen(s);
	if(n > s_cfg.max_msg_chars) n = s_cfg.max_msg_chars;

	char *copy = (char *)lv_malloc(n + 1);
	if(!copy) return;
	memcpy(copy, s, n);
	copy[n] = '\0';

	/* 若队列满：丢弃最旧的一条（避免无限占用内存），再放入新条 */
	if(osMessageQueuePut(s_msgq, &copy, 0, 0) != osOK) {
		lvterm_msg_t old = NULL;
		if(osMessageQueueGet(s_msgq, &old, NULL, 0) == osOK && old) lv_free(old);
		(void)osMessageQueuePut(s_msgq, &copy, 0, 0);
	}
}

void lvterm_printf_async(const char *fmt, ...)
{
	if(!s_msgq) return;

	/* 先计算长度，再一次分配 */
	va_list ap;
	va_start(ap, fmt);
	va_list ap2; va_copy(ap2, ap);
	int need = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);

	if(need < 0) { va_end(ap); return; }
	size_t n = (size_t)need;
	if(n > s_cfg.max_msg_chars) n = s_cfg.max_msg_chars;

	char *buf = (char *)lv_malloc(n + 1);
	if(!buf) { va_end(ap); return; }
	vsnprintf(buf, n + 1, fmt, ap);
	va_end(ap);

	/* 入队（同 put 策略） */
	if(osMessageQueuePut(s_msgq, &buf, 0, 0) != osOK) {
		lvterm_msg_t old = NULL;
		if(osMessageQueueGet(s_msgq, &old, NULL, 0) == osOK && old) lv_free(old);
		(void)osMessageQueuePut(s_msgq, &buf, 0, 0);
	}
}

void init_lvterm(void)
{
	/* 创建终端控件（在 GUI 线程里） */
	extern lv_obj_t *lvterm_create(lv_obj_t *, const lvterm_cfg_t *);
	lvterm_cfg_t cfg = {
		.keep_lines = 50,
		.fg         = lv_color_hex(0x00FF00),
		.bg         = lv_color_hex(0x000000),
		.font       = &lv_font_montserrat_14,  // 例：14px 字体（确保在 lv_conf.h 里已启用）
		.queue_len  = 32,
		.max_msg_chars = 64,
		.auto_append_newline = true,
	};
	lv_obj_t *term = lvterm_create(lv_screen_active(), &cfg);
	LV_UNUSED(term);
}