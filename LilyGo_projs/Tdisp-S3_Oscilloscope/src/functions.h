#include <Arduino.h>
#include <driver/adc.h>
#include <soc/syscon_reg.h>
#include <SPI.h>
#include <OneButton.h>
#include <arduino-timer.h>
#include "TFT_eSPI.h"
#include "esp_adc_cal.h"
#include "filters.h"


/* LCD CONFIG */
// Too low or too high pixel clock may cause screen mosaic
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ	(16 * 1000 * 1000)
// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES			320
#define EXAMPLE_LCD_V_RES			170
#define LVGL_LCD_BUF_SIZE			(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES)

/*ESP32S3*/
#define PIN_LCD_BL					38

#define PIN_LCD_D0					39
#define PIN_LCD_D1					40
#define PIN_LCD_D2					41
#define PIN_LCD_D3					42
#define PIN_LCD_D4					45
#define PIN_LCD_D5					46
#define PIN_LCD_D6					47
#define PIN_LCD_D7					48

#define PIN_POWER_ON				15

#define PIN_LCD_RES					5
#define PIN_LCD_CS					6
#define PIN_LCD_DC					7
#define PIN_LCD_WR					8
#define PIN_LCD_RD					9

#define PIN_BUTTON_1				0
#define PIN_BUTTON_2				14
#define PIN_BAT_VOLT				4

#define PIN_IIC_SCL					17
#define PIN_IIC_SDA					18

#define PIN_TOUCH_INT				16
#define PIN_TOUCH_RES				21

/* External expansion */					
#define PIN_SD_CMD					13
#define PIN_SD_CLK					11
#define PIN_SD_D0					12




//#define DEBUG_SERIAL
//#define DEBUG_BUFF
#define DELAY			1000

#define ADC_CHANNEL		ADC1_CHANNEL_5  // GPIO33
#define NUM_SAMPLES		1000            // number of samples
#define I2S_NUM			(0)
#define BUFF_SIZE		50000
#define B_MULT			BUFF_SIZE/NUM_SAMPLES
#define BtnLongPressMs	250
// #define Enable_longPress_Continuous



/* adc.cpp */
extern esp_adc_cal_characteristics_t	adc_chars;
extern void characterize_adc(void);


/* data_analysis.cpp */
extern void peak_mean(uint16_t *i2s_buffer, uint32_t len, float * max_value, float * min_value, float *pt_mean);
extern bool digital_analog(uint16_t *i2s_buffer, uint32_t max_v, uint32_t min_v);
extern void trigger_freq_analog(uint16_t *i2s_buffer, float sample_rate, float mean, uint32_t max_v,
		uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0, uint32_t *pt_trigger1);
void trigger_freq_digital(uint16_t *i2s_buffer, float sample_rate, float mean, uint32_t max_v,
		uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0);


/* debug_routines.cpp */
extern void debug_buffer(void);


/* i2s.cpp */
extern void configure_i2s(int rate);
extern void ADC_Sampling(uint16_t *i2s_buff);
extern void ADC_Sampling(uint16_t *i2s_buff);


/* options_handler.cpp */
extern OneButton BtnBack;
extern OneButton BtnEnter;
extern Timer<1> BtnEnter_AckLongPressTimer;
extern Timer<1> BtnBack_AckLongPressTimer;
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
extern void button(void);
extern void hide_menu(void);
extern void hide_all(void);
extern void show_menu(void);
extern String strings_vdiv(void);
extern String strings_sdiv(void);
extern String strings_offset(void);
extern String strings_toffset(void);
extern String strings_freq(void);
extern String strings_peak(void);
extern String strings_vmax(void);
extern String strings_vmin(void);
extern String strings_filter(void);


/* screen.cpp */
extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern float v_div;
extern float s_div;
extern float offset;
extern float toffset;
extern bool auto_scale;
extern bool full_pix;
extern bool single_trigger;
extern bool data_trigger;
extern uint16_t i2s_buff[BUFF_SIZE];
extern int data[280];
extern void setup_screen(void);
extern float to_scale(float reading);
extern float to_voltage(float reading);
extern uint32_t from_voltage(float voltage);
extern void update_screen(uint16_t *i2s_buff, float sample_rate);
void draw_sprite(float freq, float period, float mean, float max_v, float min_v,
		uint32_t trigger, float sample_rate, bool digital_data, bool new_data );
extern void draw_grid(void);
extern void draw_channel1(uint32_t trigger0, uint32_t trigger1, uint16_t *i2s_buff, float sample_rate);


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
	Cursor2
};
extern uint8_t opt;
extern bool stop;
extern bool stop_change;
extern bool new_data;
extern void core0_task(void * pvParameters);
extern void core1_task(void * pvParameters);