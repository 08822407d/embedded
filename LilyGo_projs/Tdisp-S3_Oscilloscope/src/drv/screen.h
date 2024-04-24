#include <SPI.h>
#include <TFT_eSPI.h>

#include "algo/data_analysis.h"


//#define DEBUG_SERIAL
//#define DEBUG_BUFF
#define ADC_CHANNEL		ADC1_CHANNEL_5  // GPIO33

/* screen.cpp */
extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern int16_t ScreenWidth;
extern int16_t ScreenHeight;

extern float v_div;
extern float t_div;
extern float offset;
extern float toffset;
extern bool auto_scale;
extern bool full_pix;
extern bool single_trigger;
extern bool data_trigger;
extern void setup_screen(void);
extern float to_scale(float reading);
extern void update_screen(uint32_t *AdcDataBuf, float sample_rate);
extern void draw_sprite(SignalInfo *Wave, float sample_rate, bool digital_data, bool new_data);
extern void draw_grid(int startX, int startY, uint width, uint heigh);
extern void draw_channel1(uint32_t trigger0, uint32_t trigger1, uint32_t *AdcDataBuf, float sample_rate);

extern void maxFPStest(void);