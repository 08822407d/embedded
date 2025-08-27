// #include "app_display.h"
// #include "main.h"

// #include "key_io.h"
// #include "lcd_io.h"

// #include "string.h"
// #include "stdbool.h"


// #define BUFFER_CACHE_COUNT          3

// /* RGB565: R[15:11], G[10:5], B[4:0] */
// #define RGB565_BLACK        0x0000u
// #define RGB565_WHITE        0xFFFFu
// #define RGB565_RED          0xF800u
// #define RGB565_GREEN        0x07E0u
// #define RGB565_BLUE         0x001Fu
// #define RGB565_YELLOW       0xFFE0u
// #define RGB565_CYAN         0x07FFu
// #define RGB565_MAGENTA      0xF81Fu

// /* 常见扩展色 */
// #define RGB565_ORANGE       0xFD20u  /* (255,165,  0) */
// #define RGB565_GOLD         0xFEA0u  /* (255,215,  0) */
// #define RGB565_SILVER       0xC618u  /* (192,192,192) */
// #define RGB565_GRAY         0x8410u  /* (128,128,128) */
// #define RGB565_DARKGRAY     0x4208u  /* ( 64, 64, 64) */
// #define RGB565_LIGHTGRAY    0xD69Au  /* (211,211,211) */

// #define RGB565_MAROON       0x8000u  /* (128,  0,  0) */
// #define RGB565_OLIVE        0x8400u  /* (128,128,  0) */
// #define RGB565_NAVY         0x0010u  /* (  0,  0,128) */
// #define RGB565_TEAL         0x0410u  /* (  0,128,128) */
// #define RGB565_PURPLE       0x8010u  /* (128,  0,128) */
// #define RGB565_BROWN        0xA145u  /* (165, 42, 42) */

// #define RGB565_PINK         0xFE19u  /* (255,192,203) */
// #define RGB565_VIOLET       0xEC1Du  /* (238,130,238) */
// #define RGB565_ORCHID       0xDE1Au  /* (218,112,214) */
// #define RGB565_SKYBLUE      0x867Du  /* (135,206,235) */
// #define RGB565_DEEPSKYBLUE  0x05FFu  /* (  0,191,255) */
// #define RGB565_LIGHTGREEN   0x9772u  /* (144,238,144) */
// #define RGB565_LIMEGREEN    0x3666u  /* ( 50,205, 50) */



// static uint8_t CacheBuffer[BUFFER_CACHE_COUNT][(320*2*BUFFER_CACHE_LINES)];
// // static uint16_t posy0 = 0;
// // static uint16_t posx = 0;
// // static uint16_t posy = 0;
// static uint32_t LCD_Width = 0;
// static uint32_t LCD_Height = 0;
// static uint32_t LCD_Orientation = 0;

// // static int drawing_block_idx = 0;
// // static int display_block_idx = 0;
// static __IO uint16_t tearing_effect_counter = 0;



// /**
//   * @brief  Draws a full rectangle in currently active layer.
//   * @param  Instance   LCD Instance
//   * @param  Xpos X position
//   * @param  Ypos Y position
//   * @param  Width Rectangle width
//   * @param  Height Rectangle height
//   */
// static int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
// {
//   uint32_t size;
//   uint32_t CacheLinesCnt, CacheLinesSz;
//   uint32_t offset = 0, line_cnt = Ypos;

//   size = (2*Width*Height);
//   CacheLinesCnt = (Height > BUFFER_CACHE_LINES ? BUFFER_CACHE_LINES : Height);
//   CacheLinesSz = (2*Width*CacheLinesCnt);
//   memset(CacheBuffer[0], Color, CacheLinesSz);

//   while(line_cnt < (Ypos + Height))
//   {
//     uint16_t current_display_line = (tearing_effect_counter > 0 ? 0xFFFF : ((uint16_t)hLCDTIM.Instance->CNT));
//     if((line_cnt + CacheLinesCnt) < current_display_line)
//     {
//       if(BSP_LCD_FillRGBRect(Instance, 0, CacheBuffer[0], Xpos, line_cnt, Width, CacheLinesCnt) == BSP_ERROR_NONE)
//       {
//         line_cnt += CacheLinesCnt;
//         offset += CacheLinesSz;
//       }
//     }
//     /* Check remaining data size */
//     if(offset == size)
//     {
//       /* last block transfer was done, so exit */
//       break;
//     }
//     else if((offset + CacheLinesSz) > size)
//     {
//       /* Transfer last block and exit */
//       CacheLinesCnt = ((size - offset)/ (2*Width));
//       current_display_line = (tearing_effect_counter > 0 ? 0xFFFF : ((uint16_t)hLCDTIM.Instance->CNT));
//       if((line_cnt + CacheLinesCnt) < current_display_line)
//       {
//         if(BSP_LCD_FillRGBRect(Instance, 0, CacheBuffer[0], Xpos, line_cnt, Width, CacheLinesCnt) == BSP_ERROR_NONE)
//         {
//           line_cnt += CacheLinesCnt;
//         }
//       }
//     }
//   }
//   return BSP_ERROR_NONE;
// }

// static void BSP_LCD_Clear(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height)
// {
//   BSP_LCD_FillRect(Instance, Xpos, Ypos, Width, Height, 0);
// }

// static void BSP_LCD_FlushColor(uint32_t Instance, uint32_t Color)
// {
//   BSP_LCD_FillRect(Instance, 0, 0, LCD_Width, LCD_Height, Color);
// }


// /**
//  * Initialize DISPLAY application
//  */
// void MX_DISPLAY_PostInit(void)
// {
//   /* USER CODE BEGIN MX_DISPLAY_Init 0 */

//   /* USER CODE END MX_DISPLAY_Init 0 */

//   /* USER CODE BEGIN MX_DISPLAY_Init 1 */
//   if((BSP_LCD_GetXSize(0, &LCD_Width) != BSP_ERROR_NONE) \
//   || (BSP_LCD_GetYSize(0, &LCD_Height) != BSP_ERROR_NONE) \
//   || (BSP_LCD_GetOrientation(0, &LCD_Orientation) != BSP_ERROR_NONE) )
//   {
//     Error_Handler();
//   }
//   BSP_LCD_Clear(0, 0, 0, LCD_Width, LCD_Height);
//   BSP_LCD_FlushColor(0, RGB565_RED); // Black
//   if(BSP_LCD_DisplayOn(0) != BSP_ERROR_NONE)
//   {
//     Error_Handler();
//   }
//   /* USER CODE END MX_DISPLAY_Init 1 */
// }