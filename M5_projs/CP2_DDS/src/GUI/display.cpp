#include "display.hpp"
#include "ScreenPage.hpp"
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


extern void initScreenPages(void);


#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
	LV_UNUSED(level);
	Serial.println(buf);
	Serial.flush();
}
#endif

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
	/*Copy `px map` to the `area`*/

	/*For example ("my_..." functions needs to be implemented by you)
	uint32_t w = lv_area_get_width(area);
	uint32_t h = lv_area_get_height(area);

	my_set_window(area->x1, area->y1, w, h);
	my_draw_bitmaps(px_map, w * h);
	 */

	/*Call it to tell LVGL you are ready*/
	lv_display_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
	/*For example  ("my_..." functions needs to be implemented by you)
	int32_t x, y;
	bool touched = my_get_touch( &x, &y );

	if(!touched) {
		data->state = LV_INDEV_STATE_RELEASED;
	} else {
		data->state = LV_INDEV_STATE_PRESSED;

		data->point.x = x;
		data->point.y = y;
	}
	 */
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
	return millis();
}



void loopLvglDisplay(void) {
	lv_timer_handler(); /* let the GUI do its work */
}

void initLvglDisplay(void) {
	Serial.println( "LVGL_Arduino" );

	lv_init();

	/*Set a tick source so that LVGL will know how much time elapsed. */
	lv_tick_set_cb(my_tick);

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
	lv_indev_set_read_cb(indev, my_touchpad_read);

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

	// lv_obj_t *label = lv_label_create( lv_screen_active() );
	// lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
	// lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

	Serial.println( "Setup done" );

	extern void initRoller(void);
	initRoller();
}



lv_obj_t * DigitRoller;

void Joystick4WayRollerUp()
{
	// uint32_t index = lv_roller_get_selected(DigitRoller);
	// index++;
	// lv_roller_set_selected(DigitRoller, index, LV_ANIM_ON);

	uint32_t t = LV_KEY_UP;
	lv_obj_send_event(DigitRoller, LV_EVENT_KEY, &t);
}

void Joystick4WayRollerDown()
{
	// uint32_t index = lv_roller_get_selected(DigitRoller);
	// index--;
	// lv_roller_set_selected(DigitRoller, index, LV_ANIM_ON);

	uint32_t t = LV_KEY_DOWN;
	lv_obj_send_event(DigitRoller, LV_EVENT_KEY, &t);
}

static void event_handler(lv_event_t * e)
{
	// lv_event_code_t code = lv_event_get_code(e);
	// lv_obj_t * obj = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
	// if(code == LV_EVENT_KEY) {
	// 	char buf[32];
	// 	lv_roller_get_selected_str(obj, buf, sizeof(buf));
	// 	LV_LOG_USER("Selected value: %s", buf);
	// 	Serial.printf("Selected value: %s\n", buf);
	// }
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

	initScreenPages();
}


void initScreenPages(void) {
	RootPage = std::make_shared<ScreenPage>("Root");

	SetDDSPage = std::make_shared<ScreenPage>("Set DDS Parameters");
	OtherSettingsPage = std::make_shared<ScreenPage>("Other Settings");

	SetWaveFormPage = std::make_shared<ScreenPage>("Set WaveForm");
	SetFrequencyPage = std::make_shared<ScreenPage>("Set Frequency");
	SetPhasePage = std::make_shared<ScreenPage>("Set Phase");



	RootPage->addChild(SetDDSPage);
	RootPage->addChild(OtherSettingsPage);

	SetDDSPage->addChild(SetWaveFormPage);
	SetDDSPage->addChild(SetFrequencyPage);
	SetDDSPage->addChild(SetPhasePage);

	extern void initMenuScreens(std::shared_ptr<ScreenPage> RootPage);
	initMenuScreens(RootPage);
}