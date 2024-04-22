#include "headers.h"


uint8_t opt				= None;
int8_t volts_index		= 0;
int8_t tscale_index		= 0;
uint8_t	current_filter	= 1;

bool menu				= false;
bool info				= true;
bool set_value			= false;
bool menu_action		= false;

uint8_t digital_wave_option	= 0; // 0-auto | 1-analog | 2-digital data (SERIAL/SPI/I2C/etc)

int voltage_division[7] = { //screen has 4 divisions, 40 pixels each (170 pixels of height)
	850, //fullscreen 3.3V peak-peak
	550,
	375,
	250,
	180,
	100,
	50
};

/*each sample represents 1us (1Msps),
	 thus, the time division is the number
	 of samples per screen division
*/
float time_division[7] = { //screen has 8 divisions, 40 pixel each (280 pixel of width)
	10,
	25,
	50,
	100,
	250,
	500,
	1000,
};
// 	2500,
// 	5000
// };


void menu_handler() {
	if (btnok || btnbk || btnpl || btnmn) {
		menu_action = true;
	}
	if (menu) {
		if (set_value) {
			switch (opt) {
				case Vdiv:
					if (btnpl > 0) {
						volts_index++;
						if (volts_index >= sizeof(voltage_division) / sizeof(*voltage_division)) {
							volts_index = 0;
						}
						btnpl--;
					} else if (btnmn > 0) {
						volts_index--;
						if (volts_index < 0) {
							volts_index = sizeof(voltage_division) / sizeof(*voltage_division) - 1;
						}
						btnmn--;
					}

					v_div = voltage_division[volts_index];
					break;

				case Sdiv:
					if (btnmn > 0) {
						tscale_index++;
						if (tscale_index >= sizeof(time_division) / sizeof(*time_division)) {
							tscale_index = 0;
						}
						btnmn--;
					} else if (btnpl > 0) {
						tscale_index--;
						if (tscale_index < 0) {
							tscale_index = sizeof(time_division) / sizeof(*time_division) - 1;
						}
						btnpl--;
					}

					t_div = time_division[tscale_index];
					break;

				case Offset:
					if (btnmn > 0) {
						offset += 0.1 * (v_div * 4) / 3300;
						btnmn--;
					} else if (btnpl > 0) {
						offset -= 0.1 * (v_div * 4) / 3300;
						btnpl = 0;
					}

					if (offset > 3.3)
						offset = 3.3;
					if (offset < -3.3)
						offset = -3.3;

					break;

				case TOffset:
					if (btnpl > 0) {
						toffset += 0.1 * t_div;
						btnpl--;
					} else if (btnmn > 0) {
						toffset -= 0.1 * t_div;
						btnmn--;
					}

					break;

				default:
					break;

			}
			if (btnbk) {
				set_value = 0;
				btnbk = false;
			}
		} else {
			if (btnpl > 0) {
				opt++;
				if (opt > Single) {
					opt = 1;
				}
				// Serial.print("option : ");
				Serial.println(opt);
				btnpl--;
			}
			if (btnmn > 0) {
				opt--;
				if (opt < 1)
				{
					opt = 9;
				}
				// Serial.print("option : ");
				Serial.println(opt);
				btnmn--;
			}
			if (btnbk) {
				hide_menu();
				btnbk = false;
			}
			if (btnok) {
				switch (opt) {
					case Autoscale:
						auto_scale = !auto_scale;
						break;

					case Vdiv:
						set_value = true;
						break;

					case Sdiv:
						set_value = true;
						break;

					case Offset:
						set_value = true;
						break;

					case Stop:
						stop = !stop;
						//Serial.print("Stop : ");
						//Serial.println(stop);
						set_value = false;
						break;

					case TOffset:
						set_value = true;
						//set_value = false;
						break;

					case Single:
						single_trigger = true;
						set_value = false;
						break;

					case Reset:
						offset = 0;
						v_div = 550;
						t_div = 10;
						tscale_index = 0;
						volts_index = 0;
						break;

					case Probe:
						break;

					case Mode:
						digital_wave_option++;
						if (digital_wave_option > 2)
							digital_wave_option = 0;
						break;

					case Filter:
						current_filter++;
						if (current_filter > 3)
							current_filter = 0;
						break;

					default:
						break;

				}

				btnok = false;
			}
		}
	} else {
		if (btnok) {
			opt = 1;
			show_menu();
			btnok = false;
		}
		if (btnbk) {
			if (info == true) {
				hide_all();
			} else {
				info = true;
			}
			btnbk = false;
		}
		if (btnpl > 0) {
			volts_index++;
			if (volts_index >= sizeof(voltage_division) / sizeof(*voltage_division)) {
				volts_index = 0;
			}
			btnpl--;
			v_div = voltage_division[volts_index];
		}
		if (btnmn > 0) {
			tscale_index++;
			if (tscale_index >= sizeof(time_division) / sizeof(*time_division)) {
				tscale_index = 0;
			}
			btnmn--;
			t_div = time_division[tscale_index];
		}
	}
}

void hide_menu() {
	menu = false;
}

void hide_all() {
	menu = false;
	info = false;
}

void show_menu() {
	menu = true;
}