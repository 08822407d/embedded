#include <SPI.h>
#include <TFT_eSPI.h>


//#define DEBUG_SERIAL
//#define DEBUG_BUFF
#define ADC_CHANNEL		ADC1_CHANNEL_5  // GPIO33

/* screen.cpp */
extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern float v_div;
extern float t_div;
extern float offset;
extern float toffset;
extern bool auto_scale;
extern bool full_pix;
extern bool single_trigger;
extern bool data_trigger;
extern uint16_t AdcSample_Buffer[];
extern void setup_screen(void);
extern float to_scale(float reading);
extern float to_voltage(float reading);
extern void update_screen(uint16_t *AdcDataBuf, float sample_rate);
void draw_sprite(float freq, float period, float mean, float max_v, float min_v,
		uint32_t trigger, float sample_rate, bool digital_data, bool new_data );
extern void draw_grid(int startX, int startY, uint width, uint heigh);
extern void draw_channel1(uint32_t trigger0, uint32_t trigger1, uint16_t *AdcDataBuf, float sample_rate);