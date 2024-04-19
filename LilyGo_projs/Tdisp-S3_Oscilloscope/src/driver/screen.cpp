#include "../headers.h"


TFT_eSPI	tft		= TFT_eSPI();         // Declare object "tft"
TFT_eSprite	spr		= TFT_eSprite(&tft);  // Declare Sprite object "spr" with pointer to "tft" object
int16_t		ScreenWidth		= 0;
int16_t		ScreenHeight	= 0;

float v_div			= 825;
float s_div			= 10;
float offset		= 0;
float toffset		= 0;
bool auto_scale		= false;
bool full_pix		= true;
bool single_trigger	= false;
bool data_trigger	= false;

uint32_t AdcSample_Buffer[BUFF_SIZE];
int data[280] = {0};

void setup_screen() {
	// (POWER ON)IO15 must be set to HIGH before starting, otherwise the screen will not display when using battery
	pinMode(PIN_POWER_ON, OUTPUT);
	digitalWrite(PIN_POWER_ON, HIGH);

	// Initialise the TFT registers
	tft.begin();
	tft.setRotation(3);
	ScreenWidth = tft.width();
	ScreenHeight = tft.height();

	// Optionally set colour depth to 8 or 16 bits, default is 16 if not spedified
	spr.setColorDepth(8);
	// Create a sprite of defined size
	spr.createSprite(ScreenWidth, ScreenHeight);
	spr.setSwapBytes(true);
	// Clear the TFT screen to blue
	tft.fillScreen(TFT_BLACK);
}

float to_scale(float reading) {
	float temp = ScreenWidth -
							 (
								 (
									 (
										 (float)((reading - 20480.0) / 4095.0)
										 + (offset / 3.3)
									 )
									 * 3300 /
									 (v_div * 6)
								 )
							 )
							 * (ScreenWidth - 1)
							 - 1;
	return temp;
}

float to_voltage(float reading) {
	return  (reading - 20480.0) / 4095.0 * 3.3;
}

uint32_t from_voltage(float voltage) {
	return uint32_t(voltage / 3.3 * 4095 + 20480.0);
}


void update_screen(uint32_t *AdcDataBuf, float sample_rate) {
	float mean = 0;
	float max_v, min_v;

	peak_mean(AdcDataBuf, BUFF_SIZE, &max_v, &min_v, &mean);

	float freq = 0;
	float period = 0;
	uint32_t trigger0 = 0;
	uint32_t trigger1 = 0;
	bool digital_data =
		trigger_freq(AdcDataBuf, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);

	draw_sprite(freq, period, mean, max_v, min_v, trigger0, sample_rate, digital_data, true);
}

void draw_sprite(float freq, float period, float mean, float max_v, float min_v,
		uint32_t trigger, float sample_rate, bool digital_data, bool new_data ) {

	max_v = to_voltage(max_v);
	min_v = to_voltage(min_v);

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
	if (current_filter == 0)
		str_filter = "None";
	else if (current_filter == 1)
		str_filter = "Pixel";
	else if (current_filter == 2)
		str_filter = "Mean-5";
	else if (current_filter == 3)
		str_filter = "Lpass9";

	String str_stop = "";
	if (!stop)
		str_stop = "RUNNING";
	else
		str_stop = "STOPPED";

	String wave_option = "";
	if (digital_wave_option == 0)
		if (digital_data )
			wave_option = "AUTO:Dig./data";
		else
			wave_option = "AUTO:Analog";
	else if (digital_wave_option == 1)
		wave_option = "MODE:Analog";
	else
		wave_option = "MODE:Dig./data";


	if (new_data) {
		// Fill the whole sprite with black (Sprite is in memory so not visible yet)
		spr.fillSprite(TFT_BLACK);

		draw_grid(0, 0, ScreenWidth, ScreenHeight);

		if (auto_scale) {
			auto_scale = false;
			v_div = 1000.0 * max_v / 6.0;
			s_div = period / 3.5;
			if (s_div > 7000 || s_div <= 0)
				s_div = 7000;
			if (v_div <= 0)
				v_div = 550;
		}

		//only draw digital data if a trigger was in the data
		if (!(digital_wave_option == 2 && trigger == 0))
			draw_channel1(trigger, 0, AdcSample_Buffer, sample_rate);
	}

	int shift = 210;
	if (menu) {
		spr.fillRect(shift, 0, 102, 135, TFT_BLACK);
		spr.drawRect(shift, 0, 102, 135, TFT_WHITE);
		spr.fillRect(shift + 1, 3 + 10 * (opt - 1), 100, 11, TFT_RED);

		spr.drawString("AUTOSCALE",  shift + 5, 5);
		spr.drawString(String(int(v_div)) + "mV/div",  shift + 5, 15);
		spr.drawString(String(int(s_div)) + "uS/div",  shift + 5, 25);
		spr.drawString("Offset: " + String(offset) + "V",  shift + 5, 35);
		spr.drawString("T-Off: " + String((uint32_t)toffset) + "uS",  shift + 5, 45);
		spr.drawString("Filter: " + str_filter, shift + 5, 55);
		spr.drawString(str_stop, shift + 5, 65);
		spr.drawString(wave_option, shift + 5, 75);
		spr.drawString("Single " + String(single_trigger ? "ON" : "OFF"), shift + 5, 85);

		spr.drawLine(shift, 103, shift + 100, 103, TFT_WHITE);

		spr.drawString("Vmax: " + String(max_v) + "V",  shift + 5, 105);
		spr.drawString("Vmin: " + String(min_v) + "V",  shift + 5, 115);
		spr.drawString(s_mean,  shift + 5, 125);

		shift -= 70;

		//spr.fillRect(shift, 0, 70, 30, TFT_BLACK);
		spr.drawRect(shift, 0, 70, 30, TFT_WHITE);
		spr.drawString("P-P: " + String(max_v - min_v) + "V",  shift + 5, 5);
		spr.drawString(frequency,  shift + 5, 15);
		String offset_line = String((2.0 * v_div) / 1000.0 - offset) + "V";
		spr.drawString(offset_line,  shift + 40, 59);

		if (set_value) {
			spr.fillRect(229, 0, 11, 11, TFT_BLUE);
			spr.drawRect(229, 0, 11, 11, TFT_WHITE);
			spr.drawLine(231, 5, 238 , 5, TFT_WHITE);
			spr.drawLine(234, 2, 234, 8, TFT_WHITE);


			spr.fillRect(229, 124, 11, 11, TFT_BLUE);
			spr.drawRect(229, 124, 11, 11, TFT_WHITE);
			spr.drawLine(231, 129, 238, 129, TFT_WHITE);
		}
	}
	else if (info) {
		//spr.drawRect(shift + 10, 0, 280 - shift - 20, 30, TFT_WHITE);
		spr.drawString("P-P: " + String(max_v - min_v) + "V",  shift + 15, 5);
		spr.drawString(frequency,  shift + 15, 15);
		spr.drawString(String(int(v_div)) + "mV/div",  shift - 100, 5);
		spr.drawString(String(int(s_div)) + "uS/div",  shift - 100, 15);
		String offset_line = String((2.0 * v_div) / 1000.0 - offset) + "V";
		spr.drawString(offset_line,  shift + 70, 112);
	}


	//push the drawed sprite to the screen
	spr.pushSprite(0, 0);

	yield(); // Stop watchdog reset
}

void draw_grid(int startX, int startY, uint width, uint heigh) {
	int x_off = width / 2;
	int y_off = heigh / 2;
	uint point_off = 4;
	uint grid_size = 40;
	uint cross_size = 2;
	uint dash_color = TFT_SILVER;
	uint axis_color = TFT_YELLOW;
	uint cross_color = TFT_YELLOW;
	// make sure the dash-lines exactly overlap at each cross point
	assert((grid_size % point_off) == 0);
	int centerX = x_off + startX;
	int centerY = y_off + startY;

	// draw horizontal dash-lines symmetrically from center to 4 Diagonals
	for (int row = 0; row <= heigh / 2; row += grid_size)
		for (int pix_idx = 0; pix_idx <= width / 2; pix_idx += point_off) {
			int x_left = centerX - pix_idx;
			int x_right = centerX + pix_idx;
			int y_top = centerY - row;
			int y_bottom = centerY + row;
			spr.drawPixel(x_right, y_top, dash_color);
			spr.drawPixel(x_right, y_bottom, dash_color);
			spr.drawPixel(x_left, y_bottom, dash_color);
			spr.drawPixel(x_left, y_top, dash_color);
			if ((pix_idx % grid_size) == 0) {
				spr.drawLine(x_left - cross_size, y_top, x_left + cross_size, y_top, cross_color);
				spr.drawLine(x_left - cross_size, y_bottom, x_left + cross_size, y_bottom, cross_color);
				spr.drawLine(x_right - cross_size, y_bottom, x_right + cross_size, y_bottom, cross_color);
				spr.drawLine(x_right - cross_size, y_top, x_right + cross_size, y_top, cross_color);
			}
		}
	// draw vertical dash-lines symmetrically from center to 4 Diagonals
	for (int col = 0; col <= width / 2; col += grid_size)
		for (int pix_idx = 0; pix_idx <= heigh / 2; pix_idx += point_off) {
			int x_left = centerX - col;
			int x_right = centerX + col;
			int y_top = centerY - pix_idx;
			int y_bottom = centerY + pix_idx;
			spr.drawPixel(x_left, y_top, dash_color);
			spr.drawPixel(x_left, y_bottom, dash_color);
			spr.drawPixel(x_right, y_bottom, dash_color);
			spr.drawPixel(x_right, y_top, dash_color);
			if ((pix_idx % grid_size) == 0) {
				spr.drawLine(x_left, y_top - cross_size, x_left, y_top + cross_size, cross_color);
				spr.drawLine(x_left, y_bottom - cross_size, x_left, y_bottom + cross_size, cross_color);
				spr.drawLine(x_right, y_bottom - cross_size, x_right, y_bottom + cross_size, cross_color);
				spr.drawLine(x_right, y_top - cross_size, x_right, y_top + cross_size, cross_color);
			}
		}
	
	// draw two Axis
	spr.drawLine(0, centerY, width, centerY, axis_color); // X-axis
	// spr.drawLine(centerX, 0, centerX, heigh, axis_color); // Y-axis
}

void draw_channel1(uint32_t trigger0, uint32_t trigger1, uint32_t *AdcDataBuf, float sample_rate) {
	//screen wave drawing
	data[0] = to_scale(AdcDataBuf[trigger0]);
	low_pass filter(0.99);
	mean_filter mfilter(5);
	mfilter.init(AdcDataBuf[trigger0]);
	filter._value = AdcDataBuf[trigger0];
	float data_per_pixel = (s_div / 40.0) / (sample_rate / 1000);

	Serial.printf("s_div %d, sample_rate %d, data_per_pixel %d\n", s_div, sample_rate, data_per_pixel);
	// Serial.printf("buf_datas: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
	// 		AdcDataBuf[0], AdcDataBuf[10], AdcDataBuf[20], AdcDataBuf[30], AdcDataBuf[40],
	// 		AdcDataBuf[50], AdcDataBuf[60], AdcDataBuf[70], AdcDataBuf[80], AdcDataBuf[90]);

	//  uint32_t cursor = (trigger1-trigger0)/data_per_pixel;
	//  spr.drawLine(cursor, 0, cursor, 135, TFT_RED);

	uint32_t index_offset = (uint32_t)(toffset / data_per_pixel);
	trigger0 += index_offset;  
	uint32_t old_index = trigger0;
	float n_data = 0, o_data = to_scale(AdcDataBuf[trigger0]);
	for (uint32_t i = 1; i < ScreenWidth; i++) {
		uint32_t index = trigger0 + (uint32_t)((i + 1) * data_per_pixel);
		if (index < BUFF_SIZE) {
			if (full_pix && s_div > 40 && current_filter == 0) {
				uint32_t max_val = AdcDataBuf[old_index];
				uint32_t min_val = AdcDataBuf[old_index];
				for (int j = old_index; j < index; j++) {
					//draw lines for all this data points on pixel i
					if (AdcDataBuf[j] > max_val)
						max_val = AdcDataBuf[j];
					else if (AdcDataBuf[j] < min_val)
						min_val = AdcDataBuf[j];

				}
				spr.drawLine(i, to_scale(min_val), i, to_scale(max_val), TFT_BLUE);
			} else {
				if (current_filter == 2)
					n_data = to_scale(mfilter.filter((float)AdcDataBuf[index]));
				else if (current_filter == 3)
					n_data = to_scale(filter.filter((float)AdcDataBuf[index]));
				else
					n_data = to_scale(AdcDataBuf[index]);

				spr.drawLine(i - 1, o_data, i, n_data, TFT_BLUE);
				o_data = n_data;
			}

		} else {
			break;
		}
		old_index = index;
	}
}
