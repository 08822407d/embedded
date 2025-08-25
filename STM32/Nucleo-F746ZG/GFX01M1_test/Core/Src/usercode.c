#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "lcd_io.h"



/* ---------------- 固定为 RGB565 的颜色常量（宏） ---------------- */
/* RGB565：R(5) G(6) B(5) */
#define LCD565_BLACK   0x0000U
#define LCD565_WHITE   0xFFFFU
#define LCD565_RED     0xF800U
#define LCD565_GREEN   0x07E0U
#define LCD565_BLUE    0x001FU

/* ---------------- 块传输的全局缓存 ----------------
 * 说明：为简化测试，假设屏幕最大宽度不超过 320 像素（GFX01M1 为 QVGA）。:contentReference[oaicite:2]{index=2}
 * 你可以按需把 LCD_MAX_WIDTH/LCD_CHUNK_LINES 调大/调小。
 */
#ifndef LCD_MAX_WIDTH
#define LCD_MAX_WIDTH      320U
#endif
#ifndef LCD_CHUNK_LINES
#define LCD_CHUNK_LINES     16U
#endif
/* RGB565 → 每像素 2 字节 */
uint8_t g_lcd_chunk[LCD_MAX_WIDTH * 2U * LCD_CHUNK_LINES];


/* 内部工具：把 g_lcd_chunk 预填为某个 RGB565 颜色的若干行（全局缓存版） */
static void LCD_PrepChunk_RGB565(uint16_t color, uint32_t width, uint32_t lines)
{
    /* 小端主机：低字节在前；SPI 发送层会按字节推流。
       若底层期望字节序与此相反，测试时红蓝会异常，可对调两个字节验证。 */
    const uint8_t lo = (uint8_t)(color & 0xFFU);
    const uint8_t hi = (uint8_t)(color >> 8);
    const uint32_t stride = width * 2U;

    for (uint32_t y = 0; y < lines; ++y) {
        uint8_t *row = &g_lcd_chunk[y * stride];
        for (uint32_t x = 0; x < width; ++x) {
            row[2U*x + 0] = lo;
            row[2U*x + 1] = hi;
        }
    }
}

/* 核心：整屏刷成某个 RGB565 颜色（按行块发送，避免大内存占用） */
int LCD_FillScreen_RGB565(uint32_t Instance, uint8_t UseDMA, uint16_t color565)
{
    uint32_t w = 0, h = 0;
    (void)BSP_LCD_GetXSize(Instance, &w);
    (void)BSP_LCD_GetYSize(Instance, &h);
    if (w == 0 || h == 0) { /* 兜底到 QVGA 竖屏尺寸 */
        w = 240U; h = 320U;
    }

    /* 若实际宽度超过我们为缓存预留的上限，直接返回错误（或改为分列分块的更细切片） */
    if (w > LCD_MAX_WIDTH) {
        return -1;
    }

    for (uint32_t y = 0; y < h; y += LCD_CHUNK_LINES) {
        const uint32_t lines = (y + LCD_CHUNK_LINES <= h) ? LCD_CHUNK_LINES : (h - y);
        LCD_PrepChunk_RGB565(color565, w, lines);

        int32_t st = BSP_LCD_FillRGBRect(Instance, UseDMA,
                                         g_lcd_chunk, /* pData */
                                         0 /*X*/, y /*Y*/,
                                         w, lines);
        if (st != 0) return st;
    }
    return 0;
}


/* ----------- 便捷封装：RGBW 四色 + 清屏（全黑） ----------- */
static inline int LCD_ClearBlack(uint32_t Instance, uint8_t UseDMA)
{ return LCD_FillScreen_RGB565(Instance, UseDMA, LCD565_BLACK); }

static inline int LCD_FillRed(uint32_t Instance, uint8_t UseDMA)
{ return LCD_FillScreen_RGB565(Instance, UseDMA, LCD565_RED); }

static inline int LCD_FillGreen(uint32_t Instance, uint8_t UseDMA)
{ return LCD_FillScreen_RGB565(Instance, UseDMA, LCD565_GREEN); }

static inline int LCD_FillBlue(uint32_t Instance, uint8_t UseDMA)
{ return LCD_FillScreen_RGB565(Instance, UseDMA, LCD565_BLUE); }

static inline int LCD_FillWhite(uint32_t Instance, uint8_t UseDMA)
{ return LCD_FillScreen_RGB565(Instance, UseDMA, LCD565_WHITE); }




void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  BSP_LCD_Init(0, 2);

  /* Infinite loop */
  for(;;)
  {
    LCD_FillRed(0, 1);
    LCD_FillGreen(0, 1);
    LCD_FillBlue(0, 1);
    LCD_FillWhite(0, 1);

    LCD_ClearBlack(0, 1);
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}