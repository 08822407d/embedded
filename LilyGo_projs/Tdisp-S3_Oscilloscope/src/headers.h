#include <Arduino.h>

#include "board_defs.h"
#include "configs.h"

#include "drv/adc.h"
#include "drv/buttons.h"
#include "drv/screen.h"
#include "algo/data_analysis.h"
#include "algo/filters.h"


/*ESP32S3*/
#define PIN_POWER_ON	15


/* debug_routines.cpp */
extern void debug_buffer(void);


/* options_handler.cpp */
enum WaveOpt {
	Auto,
	Analog,
	Digital,
};
extern int8_t volts_index;
extern int8_t tscale_index;
extern uint8_t current_filter;
extern bool menu;
extern bool info;
extern bool set_value;
extern bool menu_action;
extern uint8_t digital_wave_option;
extern int voltage_division[];
extern float time_division[];
extern void InitUserButton(void);
extern void menu_handler(void);
extern void hide_menu(void);
extern void hide_all(void);
extern void show_menu(void);


/* Tdisp-S3_Oscilloscope.cpp */
//options handler
enum Option {
	None,
	Autoscale,
	Vdiv,
	Sdiv,
	Offset,
	TOffset,
	Filter,
	Stop,
	Mode,
	Single,
	Clear,
	Reset,
	Probe,
	UpdateF,
	Cursor1,
	Cursor2,
};
extern uint8_t opt;
extern bool stop;
extern bool stop_change;
extern bool new_data;
extern void core0_task(void * pvParameters);
extern void core1_task(void * pvParameters);