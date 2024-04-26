#pragma once


class WaveDiplayOptions {
public:
	bool		menu			= false;
	bool		info			= true;
	bool		set_value		= false;
	bool		menu_action		= false;

	bool		auto_scale		= false;

	int8_t		volts_index		= 1;
	int8_t		tscale_index	= 5;
	uint8_t		current_filter	= 1;
	uint8_t		digi_wave_opt	= 0; // 0-auto | 1-analog | 2-digital data (SERIAL/SPI/I2C/etc)
};

enum WaveOpt {
	Auto,
	Analog,
	Digital,
};



/* options_handler.cpp */
extern WaveDiplayOptions GlobOpts;
extern int voltage_division[];
extern float time_division[];
extern void InitUserButton(void);
extern void menu_handler(void);
extern void hide_menu(void);
extern void hide_all(void);
extern void show_menu(void);