#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   Language
 *====================*/
#define LV_CONF_LANGUAGE_EN 1  /* Use English */

/*====================
   Graphical Settings
 *====================*/

/* Color depth:
 * 1: 1 bit-per-pixel
 * 8: 8 bit-per-pixel
 * 16: 16 bit-per-pixel
 * 32: 32 bit-per-pixel
 */
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0

/* Screen resolution */
#define LV_HOR_RES_MAX          240
#define LV_VER_RES_MAX          320

/* Display buffer size (in pixels)
 * Buffer size should be a multiple of LV_HOR_RES_MAX
 */
#define LV_DISP_BUF_SIZE        (LV_HOR_RES_MAX * 10)

/*====================
   Memory Settings
 *====================*/

/* Size of the memory used by the library
 * (replace the value with the real RAM available)
 */
#define LV_MEM_SIZE             (40U * 1024U)  /* 40 KB */

/*====================
   Features
 *====================*/

/* Enable built-in themes */
#define LV_USE_THEME_DEFAULT    1

/* Enable animation */
#define LV_USE_ANIMATION        1

/* Enable group (for keyboard, encoder, mouse, touchpad devices) */
#define LV_USE_GROUP            1

/* Enable flex layout */
#define LV_USE_FLEX             1

/* Add other features as needed */

/*====================
   Fonts
 *====================*/

// /* The default font */
// #define LV_FONT_DEFAULT         &lv_font_montserrat_16

/*====================
   Debug Settings
 *====================*/

/* Enable/disable debug output
 * 0: No output
 * 1: LVGL debug messages
 */
#define LV_USE_LOG              0

#define TFT_ROTATION            LV_DISPLAY_ROTATION_270 // 设置显示屏方向

#endif /* LV_CONF_H */