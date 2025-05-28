#include <M5Unified.h>

#include "display.hpp"
#include "ScreenPage.hpp"
#include "InteractManager.hpp"
#include "menu_screen.hpp"
#include "digit_editor_screen.hpp"
#include "roller_screen.hpp"
#include "glob.hpp"

#include "conf.h"

#include "debug_utils.hpp"


// screen offset (40,53)
#define TFT_HOR_RES		135
#define TFT_VER_RES		240

#undef DRAW_BUF_SIZE
/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];


// 主页面
RollerMenuScreenPage *RootPage;

// 设置DDS参数菜单页
RollerMenuScreenPage *SetDDSPage;
// 各设置项目页
RollerScreenPage *SetWaveFormPage;
DigitEditorScreenPage *SetFrequencyPage;
DigitEditorScreenPage *SetPhasePage;

// 其他设置菜单页
RollerMenuScreenPage *OtherSettingsPage;



void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
	uint32_t w = area->x2 - area->x1 + 1;
	uint32_t h = area->y2 - area->y1 + 1;
	// // 若颜色格式为 RGB565，需要交换高低字节以匹配 ST7789 格式
	// lv_draw_sw_rgb565_swap(color_p, w * h);  // 可选
	// 调用 M5Unified 提供的API将像素数据写入屏幕区域
	M5.Display.pushImage(area->x1, area->y1, w, h, (uint16_t*)color_p);
	lv_display_flush_ready(disp);  // 通知 LVGL 刷新完成
}


#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
	LV_UNUSED(level);
	Serial.println(buf);
	Serial.flush();
}
#endif


void initLvglDisplay(void) {
	MODULE_LOG_HEAD( LVGL_Arduino );

	lv_init();

	/*Set a tick source so that LVGL will know how much time elapsed. */
	lv_tick_set_cb((lv_tick_get_cb_t)millis);

	/* register print function for debugging */
#if LV_USE_LOG != 0
	lv_log_register_print_cb( my_print );
#endif

	lv_display_t * disp;

	/*Else create a display yourself*/
	disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
	lv_display_set_flush_cb(disp, my_disp_flush);
	lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

	/*Initialize the (dummy) input device driver*/
	lv_indev_t * indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
	lv_indev_set_read_cb(indev, NULL);

	/* Create a simple label
	 * ---------------------
	 lv_obj_t *label = lv_label_create( lv_screen_active() );
	 lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
	 lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

	 * Try an example. See all the examples
	 *  - Online: https://docs.lvgl.io/master/examples.html
	 *  - Source codes: https://github.com/lvgl/lvgl/tree/master/examples
	 * ----------------------------------------------------------------

	 lv_example_btn_1();

	 * Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMO_WIDGETS
	 * -------------------------------------------------------------------------------------------

	 lv_demo_widgets();
	 */

	// lv_display_set_offset(disp, 40, 53);

	MODULE_LOG_TAIL( " ... Setup done\n" );
}


void initScreenPages(void) {
	MODULE_LOG_HEAD( ScreenPages );

	// 根菜单
	RootPage = new RollerMenuScreenPage("Root");
	DDSInteractManager = std::make_shared<InteractManager>(RootPage);

	// DDS设置菜单
	SetDDSPage = new RollerMenuScreenPage("Wave", RootPage);
	RootPage->addItem(SetDDSPage);
	// DDS具体参数设置菜单
	SetWaveFormPage = new RollerScreenPage("WaveForm", SetDDSPage);
	SetWaveFormPage->addItem("Sine\nSquare\nTriangle\nSawtooth");
	SetWaveFormPage->setDataReciever(&(globalDDSparams.WaveForm));
	SetDDSPage->addItem(SetWaveFormPage);
	SetFrequencyPage = new DigitEditorScreenPage("Frequency", SetDDSPage, 0, 10000000);
	SetFrequencyPage->setDataReciever((int32_t *)&(globalDDSparams.Frequency));
	SetDDSPage->addItem(SetFrequencyPage);
	SetPhasePage = new DigitEditorScreenPage("Phase", SetDDSPage, 0, 360);
	SetPhasePage->setDataReciever((int32_t *)&(globalDDSparams.Phase));
	SetDDSPage->addItem(SetPhasePage);

	// 其他设置菜单
	OtherSettingsPage = new RollerMenuScreenPage("Other", RootPage);
	RootPage->addItem(OtherSettingsPage);

	RootPage->enterPage(LV_SCR_LOAD_ANIM_NONE);
	MODULE_LOG_TAIL( " ... Setup done\n" );
}