#include "display.hpp"
#include "ScreenPage.hpp"
#include "InteractManager.hpp"
#include "menu_screen.hpp"
#include "conf.h"


// screen offset (40,53)
#define TFT_HOR_RES		135
#define TFT_VER_RES		240

#undef DRAW_BUF_SIZE
/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];


// 主页面
std::shared_ptr<ScreenPage> RootPage;

// 设置DDS参数菜单页
std::shared_ptr<ScreenPage> SetDDSPage;
// 各设置项目页
std::shared_ptr<ScreenPage> SetWaveFormPage;
std::shared_ptr<ScreenPage> SetFrequencyPage;
std::shared_ptr<ScreenPage> SetPhasePage;

// 其他设置菜单页
std::shared_ptr<ScreenPage> OtherSettingsPage;




#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
	LV_UNUSED(level);
	Serial.println(buf);
	Serial.flush();
}
#endif


void initLvglDisplay(void) {
	Serial.println( "LVGL_Arduino" );

	lv_init();

	/*Set a tick source so that LVGL will know how much time elapsed. */
	lv_tick_set_cb((lv_tick_get_cb_t)millis);

	/* register print function for debugging */
#if LV_USE_LOG != 0
	lv_log_register_print_cb( my_print );
#endif

	lv_display_t * disp;
#if LV_USE_TFT_ESPI
	/*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
	disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));
	lv_display_set_rotation(disp, LVGL_ROTATION);
#else
	/*Else create a display yourself*/
	disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
	lv_display_set_flush_cb(disp, my_disp_flush);
	lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

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

	Serial.println( "Setup done" );
}



void initRoller()
{
	// /*A style to make the selected option larger*/
	// static lv_style_t style_sel;
	// lv_style_init(&style_sel);
	// lv_style_set_text_font(&style_sel, &lv_font_montserrat_22);
	// lv_style_set_bg_color(&style_sel, lv_color_hex3(0xf88));
	// lv_style_set_border_width(&style_sel, 2);
	// lv_style_set_border_color(&style_sel, lv_color_hex3(0xf00));

	// DigitRoller = lv_roller_create(lv_screen_active());
	// lv_roller_set_options(DigitRoller,
	// 					  "0\n1\n2\n3\n4\n5\n6\n7\n8\n9",
	// 					  LV_ROLLER_MODE_INFINITE);
	// lv_roller_set_visible_row_count(DigitRoller, 3);
	// lv_obj_add_style(DigitRoller, &style_sel, LV_PART_SELECTED);
	// lv_obj_center(DigitRoller);
	// lv_obj_add_event_cb(DigitRoller, event_handler, LV_EVENT_ALL, NULL);
}


void initScreenPages(void) {
	// 页面树的根和管理对象
	RootPage =
		std::make_shared<ScreenPage>("Root");
	DDSInteractManager =
		std::make_shared<InteractManager>(RootPage);


	// DDS参数设置总页
	SetDDSPage =
		std::make_shared<ScreenPage>("DDS Params");
	SetDDSPage->lvgl_widget =
		createMenuScreen(SetDDSPage->getName());
	RootPage->addChild(SetDDSPage);


	// 各个DDS具体参数设置页
	SetWaveFormPage =
		std::make_shared<ScreenPage>("WaveForm");
	SetWaveFormPage->lvgl_widget =
		creatTextMenuItems(SetDDSPage->lvgl_widget, SetWaveFormPage->getName());
	SetDDSPage->addChild(SetWaveFormPage);

	SetFrequencyPage =
		std::make_shared<ScreenPage>("Frequency");
	SetFrequencyPage->lvgl_widget =
		creatTextMenuItems(SetDDSPage->lvgl_widget, SetFrequencyPage->getName());
	SetDDSPage->addChild(SetFrequencyPage);
	
	SetPhasePage =
		std::make_shared<ScreenPage>("Phase");
	SetPhasePage->lvgl_widget =
		creatTextMenuItems(SetDDSPage->lvgl_widget, SetPhasePage->getName());
	SetDDSPage->addChild(SetPhasePage);


	// 其他设置总页
	OtherSettingsPage =
		std::make_shared<ScreenPage>("Other");
	OtherSettingsPage->lvgl_widget =
		createMenuScreen(OtherSettingsPage->getName());
	RootPage->addChild(OtherSettingsPage);

	// extern void initMenuScreens(std::shared_ptr<ScreenPage> RootPage);
	// initMenuScreens(RootPage);

	DDSInteractManager->setCurrent(SetDDSPage);
	lv_scr_load(SetDDSPage->lvgl_widget);
}