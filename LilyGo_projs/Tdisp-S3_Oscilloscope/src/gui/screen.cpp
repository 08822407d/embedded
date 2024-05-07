#include "../headers.h"

#include "gui_cfg.h"


#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"
// NotoSansMono-SemiCondensedBold 20pt
#define AA_FONT_MONO  "NotoSansMonoSCB20"


// Declare object "tft"
TFT_eSPI tft			= TFT_eSPI();
// Declare Sprite object "spr" with pointer to "tft" object
TFT_eSprite spr			= TFT_eSprite(&tft);
DrawParams Canvas		= DrawParams(SCREEN_ROTAION);

TFT_eSprite CurveSpr	= TFT_eSprite(&tft);
TFT_eSprite InfoSpr		= TFT_eSprite(&tft);
CanvasArea CurveArea	= CanvasArea(&CurveSpr);
CanvasArea InfoArea		= CanvasArea(&InfoSpr);
// CanvasArea CurveArea	= CanvasArea(&tft);
// CanvasArea InfoArea		= CanvasArea(&tft);


bool single_trigger	= false;



void screen_init()
{
	#ifdef AMOLED
		/*
		 * Compatible with touch version
		 * Touch version, IO38 is the screen power enable
		 * Non-touch version, IO38 is an onboard LED light
		 * * */
		// pinMode(PIN_LED, OUTPUT);
		// digitalWrite(PIN_LED, HIGH);
		rm67162_init();
		lcd_setRotation(SCREEN_ROTAION);
		Canvas.ScreenWidth = lcd_width();
		Canvas.ScreenHeight = lcd_height();
	#else
		// (POWER ON)IO15 must be set to HIGH before starting, otherwise the screen will not display when using battery
		pinMode(PIN_POWER_ON, OUTPUT);
		digitalWrite(PIN_POWER_ON, HIGH);
		// Initialise the TFT registers
		tft.begin();
		// tft.setAttribute(PSRAM_ENABLE, false);
		tft.setRotation(SCREEN_ROTAION);
		Canvas.ScreenWidth = tft.width();
		Canvas.ScreenHeight = tft.height();
		tft.setTextColor(TFT_GREEN, BG_DARK_GRAY);
		tft.setTextSize(2);
	#endif

	// Optionally set colour depth to 8 or 16 bits, default is 16 if not spedified
	// spr.setColorDepth(8);
	spr.createSprite(Canvas.ScreenWidth, Canvas.ScreenHeight);
	spr.setSwapBytes(1);


	CurveArea.setArea(0, 0, 6 * GRID_SIZE, Canvas.ScreenHeight);
	InfoArea.setArea(Point2D(CurveArea.getEndOnCanvas().X + 1, 0),
			Point2D(Canvas.ScreenWidth - 1, Canvas.ScreenHeight));
	voltage_division[1] = ADC_VOLTREAD_CAP / (CurveArea.Height / GRID_SIZE);
}

void setup_screen() {
	screen_init();

	// // Optionally set colour depth to 8 or 16 bits, default is 16 if not spedified
	// spr.setColorDepth(16);
	// spr.loadFont(AA_FONT_LARGE); // Must load the font first into the sprite class
	// Create a sprite of defined size
	spr.setTextColor(TFT_GREEN, BG_DARK_GRAY);
	spr.setTextSize(SPR_TXT_SIZE);
	spr.fillSprite(BG_DARK_GRAY);
	pushScreenBuffer();


	// Init Global variables related to draw screen
	CurveArea.v_div = voltage_division[GlobOpts.volts_index];
	CurveArea.t_div = time_division[GlobOpts.tscale_index];
}



void update_screen(SignalInfo *Wave) {
	unsigned long draw_start = micros();

	draw_sprite(Wave, true);

	//push the drawed sprite to the screen
	// pushScreenBuffer();

	// draw screen performance
	unsigned long draw_end = micros();
	unsigned long draw_timespan = draw_end - draw_start;
	Serial.println("Push Sprite Time: " + String(draw_timespan / 1000.0) + "ms\n");
}

void draw_sprite(SignalInfo *Wave, bool new_data) {

	float max_v = to_voltage(Wave->MaxVal);
	float min_v = to_voltage(Wave->MinVal);
	float mean = Wave->MeanVolt;
	float freq = Wave->Freq;
	float period = Wave->Period;
	uint32_t trigger = Wave->TrigIdx_0;

	String frequency = "";
	if (freq < 1000)
		frequency = String(freq) + "hz";
	else if (freq < 100000)
		frequency = String(freq / 1000) + "khz";
	else
		frequency = "----";

	String s_mean = "";
	if (mean > 1.0)
		s_mean = "Avg: " + String(mean) + "V";
	else
		s_mean = "Avg: " + String(mean * 1000.0) + "mV";

	String str_filter = "";
	if (GlobOpts.current_filter == 0)
		str_filter = "None";
	else if (GlobOpts.current_filter == 1)
		str_filter = "Pixel";
	else if (GlobOpts.current_filter == 2)
		str_filter = "Mean-5";
	else if (GlobOpts.current_filter == 3)
		str_filter = "Lpass9";

	String str_stop = "";
	if (!stop)
		str_stop = "RUNNING";
	else
		str_stop = "STOPPED";

	String wave_option = "";
	if (GlobOpts.digi_wave_opt == 0)
		if (Wave->IsDigital)
			wave_option = "AUTO:Dig./data";
		else
			wave_option = "AUTO:Analog";
	else if (GlobOpts.digi_wave_opt == 1)
		wave_option = "MODE:Analog";
	else
		wave_option = "MODE:Dig./data";


	if (new_data) {
		// Fill the whole sprite with black (Sprite is in memory so not visible yet)
		drawGridOnArea(&CurveArea);

		if (GlobOpts.auto_scale) {
			GlobOpts.auto_scale = false;
			CurveArea.v_div = 1000.0 * max_v / 6.0;
			CurveArea.t_div = period / 8;
			if (CurveArea.t_div > 7000 || CurveArea.t_div <= 0)
				CurveArea.t_div = 7000;
			if (CurveArea.v_div <= 0)
				CurveArea.v_div = 550;
		}

		//only draw digital data if a trigger was in the data
		if (!(GlobOpts.digi_wave_opt == 2 && trigger == 0))
		{
			CurveArea.drawCurve(Wave);
			CurveArea.flushDrawArea();
		}
	}

	int Xshift = 250;
	int Yshift = 10;
	if (GlobOpts.menu) {
		spr.fillRect(Xshift, 0, 102, 135, BG_DARK_GRAY);
		spr.drawRect(Xshift, 0, 102, 135, TFT_WHITE);
		spr.fillRect(Xshift + 1, 3 + 10 * (opt - 1), 100, 11, TFT_RED);

		spr.drawString("AUTOSCALE", Xshift + 5, 5);
		spr.drawString(String(int(CurveArea.v_div)) + "mV/div", Xshift + 5, 15);
		spr.drawString(String(int(CurveArea.t_div)) + "uS/div", Xshift + 5, 25);
		spr.drawString("Offset: " + String(CurveArea.offset) + "V", Xshift + 5, 35);
		spr.drawString("T-Off: " + String((uint32_t)CurveArea.toffset) + "uS", Xshift + 5, 45);
		spr.drawString("Filter: " + str_filter, Xshift + 5, 55);
		spr.drawString(str_stop, Xshift + 5, 65);
		spr.drawString(wave_option, Xshift + 5, 75);
		spr.drawString("Single " + String(single_trigger ? "ON" : "OFF"), Xshift + 5, 85);

		spr.drawLine(Xshift, 103, Xshift + 100, 103, TFT_WHITE);

		spr.drawString("Vmax: " + String(max_v) + "V", Xshift + 5, 105);
		spr.drawString("Vmin: " + String(min_v) + "V", Xshift + 5, 115);
		spr.drawString(s_mean, Xshift + 5, 125);

		Xshift -= 70;

		spr.drawRect(Xshift, 0, 70, 30, TFT_WHITE);
		spr.drawString("P-P: " + String(max_v - min_v) + "V",  Xshift + 5, 5);
		spr.drawString(frequency,  Xshift + 5, 15);
		String offset_line = String((2.0 * CurveArea.v_div) / 1000.0 - CurveArea.offset) + "V";
		spr.drawString(offset_line,  Xshift + 30, 59);

		if (GlobOpts.set_value) {
			spr.fillRect(229, 0, 11, 11, TFT_BLUE);
			spr.drawRect(229, 0, 11, 11, TFT_WHITE);
			spr.drawLine(231, 5, 238 , 5, TFT_WHITE);
			spr.drawLine(234, 2, 234, 8, TFT_WHITE);


			spr.fillRect(229, 124, 11, 11, TFT_BLUE);
			spr.drawRect(229, 124, 11, 11, TFT_WHITE);
			spr.drawLine(231, 129, 238, 129, TFT_WHITE);
		}
	} else if (GlobOpts.info) {
		InfoArea.drawString("P-P: " + String(max_v - min_v) + "V", FONT_WIDTH, Yshift);
		InfoArea.drawString(frequency, FONT_WIDTH, Yshift + 20);

		InfoArea.drawString(String(int(CurveArea.v_div)) + "mV/div", FONT_WIDTH, Yshift + 45);
		InfoArea.drawString(String(int(CurveArea.t_div)) + "uS/div", FONT_WIDTH, Yshift + 65);

		InfoArea.drawString(String(min_v) + "/" + String(max_v), FONT_WIDTH, Canvas.ScreenHeight - 15);

		InfoArea.flushDrawArea();
	}
}



void drawGridOnArea(CanvasArea *area) {
	area->clearArea();

	uint dash_color = TFT_WHITE;
	uint axis_color = TFT_YELLOW;
	uint cross_color = TFT_YELLOW;

	// Find the center of the @area
	int HalfWidth = area->Width / 2;
	int HalfHeight = area->Height / 2;
	int centerX = HalfWidth;
	int centerY = HalfHeight;
	// Interval of points in dash-lines
	uint point_off = CurveArea.grid_size / 10;
	uint cross_size = 1;
	// make sure the dash-lines exactly overlap at each cross point
	assert((CurveArea.grid_size % point_off) == 0);

	// draw horizontal dash-lines symmetrically from center to 4 Diagonals
	for (int row = 0; row <= HalfHeight; row += CurveArea.grid_size)
		for (int pix_idx = 0; pix_idx <= HalfWidth; pix_idx += point_off) {
			int x_left = centerX - pix_idx;
			int x_right = centerX + pix_idx;
			int y_top = centerY - row;
			int y_bottom = centerY + row;
			area->drawPixel(x_right, y_top, dash_color);
			area->drawPixel(x_right, y_bottom, dash_color);
			area->drawPixel(x_left, y_bottom, dash_color);
			area->drawPixel(x_left, y_top, dash_color);
			if ((pix_idx % CurveArea.grid_size) == 0) {
				area->drawLine(x_left - cross_size, y_top,
						x_left + cross_size, y_top, cross_color);
				area->drawLine(x_left - cross_size, y_bottom,
						x_left + cross_size, y_bottom, cross_color);
				area->drawLine(x_right - cross_size, y_bottom,
						x_right + cross_size, y_bottom, cross_color);
				area->drawLine(x_right - cross_size, y_top,
						x_right + cross_size, y_top, cross_color);
			}
		}
	// draw vertical dash-lines symmetrically from center to 4 Diagonals
	for (int col = 0; col <= HalfWidth; col += CurveArea.grid_size)
		for (int pix_idx = 0; pix_idx <= HalfHeight; pix_idx += point_off) {
			int x_left = centerX - col;
			int x_right = centerX + col;
			int y_top = centerY - pix_idx;
			int y_bottom = centerY + pix_idx;
			area->drawPixel(x_left, y_top, dash_color);
			area->drawPixel(x_left, y_bottom, dash_color);
			area->drawPixel(x_right, y_bottom, dash_color);
			area->drawPixel(x_right, y_top, dash_color);
			if ((pix_idx % CurveArea.grid_size) == 0) {
				area->drawLine(x_left, y_top - cross_size,
						x_left, y_top + cross_size, cross_color);
				area->drawLine(x_left, y_bottom - cross_size,
						x_left, y_bottom + cross_size, cross_color);
				area->drawLine(x_right, y_bottom - cross_size,
						x_right, y_bottom + cross_size, cross_color);
				area->drawLine(x_right, y_top - cross_size,
						x_right, y_top + cross_size, cross_color);
			}
		}
	
	// draw two Axis
	area->drawLine(0, centerY, area->Width, centerY, axis_color); // X-axis
	// area.drawLine(centerX, 0, centerX, area.Height, axis_color); // Y-axis

	String offset_line = String((2.0 * CurveArea.v_div) / 1000.0 - CurveArea.offset) + "V";
	area->drawString(offset_line, -FONT_WIDTH, centerY - (int32_t)(FONT_HEIGHT * 1.5));
}


void pushScreenBuffer(TFT_eSprite *s,
		uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	#ifdef AMOLED
		lcd_PushColors(x, y, Canvas.ScreenWidth, Canvas.ScreenHeight, (uint16_t *)s->getPointer());
	#else
		s->pushSprite(x, y);
	#endif
}





int dbg_counter = 0;
char progress_chars[] = { '-', '\\', '|', '/' };
void DebugScreenMessage(String additional_message) {
	tft.setCursor(10, 10);
	tft.printf("Debug Here... %c\n    %s",
			progress_chars[dbg_counter++ % 4], additional_message);
}