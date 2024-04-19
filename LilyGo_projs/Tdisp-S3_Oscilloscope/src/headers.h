#include <Arduino.h>
#include <driver/adc.h>
#include <soc/syscon_reg.h>
#include "esp_adc_cal.h"


#include "board_defs.h"

#include "driver/buttons.h"
#include "driver/screen.h"
#include "algo/data_analysis.h"
#include "algo/filters.h"


/*ESP32S3*/
#define PIN_POWER_ON	15

#define NUM_SAMPLES		10				// number of samples
#define BUFF_SIZE		1000
#define B_MULT			(BUFF_SIZE / NUM_SAMPLES)


/* adc.cpp */
extern esp_adc_cal_characteristics_t	adc_chars;
extern void characterize_adc(void);
extern void ADC_Sampling(uint32_t *AdcDataBuf);


/* debug_routines.cpp */
extern void debug_buffer(void);


/* options_handler.cpp */
enum WaveOpt {
	Auto,
	Analog,
	Digital,
};
extern uint8_t current_filter;
extern bool menu;
extern bool info;
extern bool set_value;
extern bool menu_action;
extern uint8_t digital_wave_option;
extern int voltage_division[6];
extern float time_division[9];
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