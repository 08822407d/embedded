#include "../headers.h"

#include "options.h"

WaveDiplayOptions GlobOpts;
uint8_t opt				= None;


int voltage_division[] = { //screen has 4 divisions, 40 pixels each (170 pixels of height)
	1250,
	ADC_VOLTREAD_CAP / 4, //fullscreen 3.3V peak-peak
	550,
	375,
	250,
	180,
	100,
	// 50
};

/*each sample represents 1us (1Msps),
	 thus, the time division is the number
	 of samples per screen division
*/
float time_division[] = { //screen has 8 divisions, 40 pixel each (280 pixel of width)
	// 10,
	// 25,
	// 50,
	100,
	250,
	500,
	1000,
	2500,
	5000
};


void menu_handler() {
	if (BtnStat.btnok || BtnStat.btnbk || BtnStat.btnpl || BtnStat.btnmn) {
		GlobOpts.menu_action = true;
	}
	if (GlobOpts.menu) {
		if (GlobOpts.set_value) {
			switch (opt) {
				case Vdiv:
					if (BtnStat.btnpl > 0) {
						GlobOpts.volts_index++;
						if (GlobOpts.volts_index >= sizeof(voltage_division) / sizeof(*voltage_division)) {
							GlobOpts.volts_index = 0;
						}
						BtnStat.btnpl--;
					} else if (BtnStat.btnmn > 0) {
						GlobOpts.volts_index--;
						if (GlobOpts.volts_index < 0) {
							GlobOpts.volts_index = sizeof(voltage_division) / sizeof(*voltage_division) - 1;
						}
						BtnStat.btnmn--;
					}

					CurveArea.v_div = voltage_division[GlobOpts.volts_index];
					break;

				case Sdiv:
					if (BtnStat.btnmn > 0) {
						GlobOpts.tscale_index++;
						if (GlobOpts.tscale_index >= sizeof(time_division) / sizeof(*time_division)) {
							GlobOpts.tscale_index = 0;
						}
						BtnStat.btnmn--;
					} else if (BtnStat.btnpl > 0) {
						GlobOpts.tscale_index--;
						if (GlobOpts.tscale_index < 0) {
							GlobOpts.tscale_index = sizeof(time_division) / sizeof(*time_division) - 1;
						}
						BtnStat.btnpl--;
					}

					CurveArea.t_div = time_division[GlobOpts.tscale_index];
					break;

				case Offset:
					if (BtnStat.btnmn > 0) {
						CurveArea.offset += 0.1 * (CurveArea.v_div * 4) / 3300;
						BtnStat.btnmn--;
					} else if (BtnStat.btnpl > 0) {
						CurveArea.offset -= 0.1 * (CurveArea.v_div * 4) / 3300;
						BtnStat.btnpl = 0;
					}

					if (CurveArea.offset > 3.3)
						CurveArea.offset = 3.3;
					if (CurveArea.offset < -3.3)
						CurveArea.offset = -3.3;

					break;

				case TOffset:
					if (BtnStat.btnpl > 0) {
						CurveArea.toffset += 0.1 * CurveArea.t_div;
						BtnStat.btnpl--;
					} else if (BtnStat.btnmn > 0) {
						CurveArea.toffset -= 0.1 * CurveArea.t_div;
						BtnStat.btnmn--;
					}

					break;

				default:
					break;

			}
			if (BtnStat.btnbk) {
				GlobOpts.set_value = 0;
				BtnStat.btnbk = false;
			}
		} else {
			if (BtnStat.btnpl > 0) {
				opt++;
				if (opt > Single)
					opt = 1;

				// Serial.print("option : ");
				Serial.println(opt);
				BtnStat.btnpl--;
			}
			if (BtnStat.btnmn > 0) {
				opt--;
				if (opt < 1)
					opt = 9;

				// Serial.print("option : ");
				Serial.println(opt);
				BtnStat.btnmn--;
			}
			if (BtnStat.btnbk) {
				hide_menu();
				BtnStat.btnbk = false;
			}
			if (BtnStat.btnok) {
				switch (opt) {
					case Autoscale:
						GlobOpts.auto_scale = !GlobOpts.auto_scale;
						break;

					case Vdiv:
					case Sdiv:
					case Offset:
					case TOffset:
						GlobOpts.set_value = true;
						break;

					case Stop:
						stop = !stop;
						GlobOpts.set_value = false;
						break;

					case Single:
						single_trigger = true;
						GlobOpts.set_value = false;
						break;

					case Reset:
						CurveArea.offset = 0;
						CurveArea.v_div = 550;
						CurveArea.t_div = 10;
						GlobOpts.tscale_index = 0;
						GlobOpts.volts_index = 0;
						break;

					case Mode:
						GlobOpts.digi_wave_opt++;
						if (GlobOpts.digi_wave_opt > 2)
							GlobOpts.digi_wave_opt = 0;
						break;

					case Filter:
						GlobOpts.current_filter++;
						if (GlobOpts.current_filter > 3)
							GlobOpts.current_filter = 0;
						break;

					default:
						break;

				}

				BtnStat.btnok = false;
			}
		}
	} else {
		if (BtnStat.btnok) {
			opt = 1;
			show_menu();
			BtnStat.btnok = false;
		}
		if (BtnStat.btnbk) {
			if (GlobOpts.info == true)
				hide_all();
			else
				GlobOpts.info = true;

			BtnStat.btnbk = false;
		}
		if (BtnStat.btnpl > 0) {
			GlobOpts.volts_index++;
			if (GlobOpts.volts_index >= sizeof(voltage_division) / sizeof(*voltage_division))
				GlobOpts.volts_index = 0;

			BtnStat.btnpl--;
			CurveArea.v_div = voltage_division[GlobOpts.volts_index];
		}
		if (BtnStat.btnmn > 0) {
			GlobOpts.tscale_index++;
			if (GlobOpts.tscale_index >= sizeof(time_division) / sizeof(*time_division))
				GlobOpts.tscale_index = 0;

			BtnStat.btnmn--;
			CurveArea.t_div = time_division[GlobOpts.tscale_index];
		}
	}
}

void hide_menu() {
	GlobOpts.menu = false;
}

void hide_all() {
	GlobOpts.menu = false;
	GlobOpts.info = false;
}

void show_menu() {
	GlobOpts.menu = true;
}