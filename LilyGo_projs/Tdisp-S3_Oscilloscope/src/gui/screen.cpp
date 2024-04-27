#include "../headers.h"


#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"
// NotoSansMono-SemiCondensedBold 20pt
#define AA_FONT_MONO  "NotoSansMonoSCB20"


// Declare object "tft"
TFT_eSPI tft			= TFT_eSPI();
// Declare Sprite object "spr" with pointer to "tft" object
TFT_eSprite spr			= TFT_eSprite(&tft);
DrawParams Canvas		= DrawParams(SCREEN_ROTAION);
CanvasArea CurveArea	= CanvasArea(&spr);

bool single_trigger	= false;



#define DRAW_TIME_NUM	60
unsigned long DrawScreenTimes[DRAW_TIME_NUM] = { 0 };
float ScreenFPS = 0;

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
		tft.setAttribute(PSRAM_ENABLE, false);
		tft.setRotation(SCREEN_ROTAION);
		ScreenWidth = tft.width();
		ScreenHeight = tft.height();
	#endif

	// Optionally set colour depth to 8 or 16 bits, default is 16 if not spedified
	// spr.setColorDepth(8);
	spr.createSprite(Canvas.ScreenWidth, Canvas.ScreenHeight);
	spr.setSwapBytes(1);

	CurveArea.setArea(6, 0, 6 * GRID_SIZE, Canvas.ScreenHeight);
	voltage_division[1] = ADC_VOLTREAD_CAP / (CurveArea.Height / GRID_SIZE);
}

void setup_screen() {
	screen_init();

	// // Optionally set colour depth to 8 or 16 bits, default is 16 if not spedified
	// spr.setColorDepth(16);
	// spr.loadFont(AA_FONT_LARGE); // Must load the font first into the sprite class
	// Create a sprite of defined size
	spr.setTextColor(TFT_GREEN, BG_DARK_GRAY);
	spr.setTextSize(2);
	spr.fillSprite(TFT_DARKGREY);
	pushScreenBuffer();


	// Init Global variables related to draw screen
	CurveArea.v_div = voltage_division[GlobOpts.volts_index];
	CurveArea.t_div = time_division[GlobOpts.tscale_index];
}



void update_screen(SignalInfo *Wave) {
	draw_sprite(Wave, true);
	
	unsigned long draw_start = micros();

	//push the drawed sprite to the screen
	pushScreenBuffer();

	// draw screen performance
	unsigned long draw_end = micros();
	unsigned long draw_timespan = draw_end - draw_start;
	Serial.println("Push Sprite Time: " + String(draw_timespan / 1000.0) + "ms\n");

	memmove(&DrawScreenTimes[0], &DrawScreenTimes[1],
			(DRAW_TIME_NUM - 1) * sizeof(typeof(DrawScreenTimes[0])));
	DrawScreenTimes[DRAW_TIME_NUM - 1] = draw_end;
	if (DrawScreenTimes[0] >= 0)
		ScreenFPS =  (1000000.0 * (DRAW_TIME_NUM - 1)) /
						(draw_end - DrawScreenTimes[0]);
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
			CurveArea.drawCurve(Wave);
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
		spr.drawString("P-P: " + String(max_v - min_v) + "V",  Xshift, Yshift);
		spr.drawString(frequency, Xshift, Yshift + 10);

		spr.drawString(String(int(CurveArea.v_div)) + "mV/div", Xshift, Yshift + 25);
		spr.drawString(String(int(CurveArea.t_div)) + "uS/div", Xshift, Yshift + 35);

		String offset_line = String((2.0 * CurveArea.v_div) / 1000.0 - CurveArea.offset) + "V";
		spr.drawString(offset_line, Xshift + 40, Canvas.ScreenHeight / 2 + 5);

		spr.drawString(String(ScreenFPS) + "FPS", 20, 5);
		spr.drawString(String(min_v) + " - " + String(max_v), Xshift, Canvas.ScreenHeight - 15);
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
}




void CanvasArea::genDrawBuffer(SignalInfo *Wave) 
{
	uint curve_color = TFT_SKYBLUE;
	uint32_t trigger0 = Wave->TrigIdx_0;
	uint32_t *AdcDataBuf = Wave->SampleBuff;
	//screen wave drawing
	low_pass filter(0.99);
	mean_filter mfilter(5);
	mfilter.init(AdcDataBuf[trigger0]);
	filter._value = AdcDataBuf[trigger0];
	float data_per_pixel = ((float)CurveArea.t_div / CurveArea.grid_size) / (Wave->SampleRate / 1000.0);


	uint32_t index_offset = (uint32_t)(CurveArea.toffset / data_per_pixel);
	trigger0 += index_offset;  
	uint32_t old_index = trigger0;
	int32_t n_data = 0, o_data = to_scale(AdcDataBuf[trigger0]);
	CurveDrawBuff[0] = o_data;
	for (uint32_t i = 1; i < CurveArea.Width; i++) {
		uint32_t index = trigger0 + (uint32_t)((i + 1) * data_per_pixel);
		if (index < Wave->SampleNum) {
			if (GlobOpts.current_filter == 2)
				n_data = to_scale(mfilter.filter((float)AdcDataBuf[index]));
			else if (GlobOpts.current_filter == 3)
				n_data = to_scale(filter.filter((float)AdcDataBuf[index]));
			else
				n_data = to_scale(AdcDataBuf[index]);

			CurveDrawBuff[i] = n_data;
		} else {
			break;
		}
		old_index = index;
	}
}

void CanvasArea::drawCurve(SignalInfo *Wave) {
	uint curve_color = TFT_SKYBLUE;
	int32_t
		currY,
		prevY = CurveDrawBuff[0];
	for (uint32_t i = 1; i < Width; i++) {
		currY = CurveDrawBuff[i];
		if (posValid(i - 1, prevY) && posValid(i, currY))
			drawLine(i - 1, prevY, i, currY, curve_color);
		prevY = currY;
	}
}