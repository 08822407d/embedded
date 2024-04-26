#include <SPI.h>
#include <TFT_eSPI.h>

#ifdef AMOLED
#  include "rm67162.h"
#endif


#include "algo/data_analysis.h"

class DisplayParameters {
public:
	int16_t		ScreenWidth		= TFT_WIDTH;
	int16_t		ScreenHeight	= TFT_HEIGHT;
	uint		grid_size;
	uint32_t	v_div;
	uint32_t	t_div;
	float		offset;
	float		toffset;
};



/* screen.cpp */
extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern DisplayParameters Canvas;


extern bool single_trigger;
extern bool data_trigger;
extern void setup_screen(void);
extern float to_scale(float reading);
extern void update_screen(SignalInfo *Wave);
extern void draw_sprite(SignalInfo *Wave, bool new_data);
extern void draw_grid(int startX, int startY, uint width, uint heigh);
extern void genDrawBuffer(SignalInfo *Wave);
extern void draw_channel1(SignalInfo *Wave);

extern void maxFPStest(void);