#pragma once

#include "CanvasArea.hpp"


class DrawParams {
public:
	int16_t		ScreenWidth		= TFT_WIDTH;
	int16_t		ScreenHeight	= TFT_HEIGHT;

	DrawParams(uint8_t Rotation) {
		if ((Rotation % 4) == 0 || (Rotation % 4) == 2) {
			ScreenWidth = TFT_WIDTH;
			ScreenHeight = TFT_HEIGHT;
		} else {
			ScreenWidth = TFT_HEIGHT;
			ScreenHeight = TFT_WIDTH;
		}	
	}
};


/* screen.cpp */
extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern DrawParams Canvas;
extern CanvasArea CurveArea;


extern bool single_trigger;
extern bool data_trigger;
extern void setup_screen(void);
extern void update_screen(SignalInfo *Wave);
extern void draw_sprite(SignalInfo *Wave, bool new_data);
extern void draw_grid(int startX, int startY, uint width, uint heigh);
extern void drawGridOnArea(CanvasArea *area);

extern void maxFPStest(void);


extern void DebugScreenMessage(String additional_message = "");



inline void pushScreenBuffer(uint16_t x = 0, uint16_t y = 0,
		uint16_t w = Canvas.ScreenWidth,
		uint16_t h = Canvas.ScreenHeight) {
	#ifdef AMOLED
		lcd_PushColors(x, y, Canvas.ScreenWidth, Canvas.ScreenHeight, (uint16_t *)spr.getPointer());
	#else
		spr.pushSprite(x, y);
	#endif
}