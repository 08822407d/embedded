#include <Arduino.h>

#include "configs.h"

#include "drv/adc.h"
#include "drv/buttons.h"
#include "drv/joystick.hpp"
#include "gui/screen.h"
#include "algo/data_analysis.h"
#include "algo/filters.h"
#include "options/options.h"


/*ESP32S3*/
#define PIN_POWER_ON	15



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