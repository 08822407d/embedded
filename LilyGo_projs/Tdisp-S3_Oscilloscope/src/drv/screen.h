#pragma once

#include <SPI.h>
#include <TFT_eSPI.h>

#ifdef AMOLED
#  include "rm67162.h"
#endif

#include "algo/data_analysis.h"

extern void pushScreenBuffer(uint16_t x,
		uint16_t y, uint16_t w, uint16_t h);

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

class CanvasArea {
public:
	bool			Dirty			= false;
	uint16_t		Width;
	uint16_t		Height;
	uint			grid_size		= GRID_SIZE;
	uint32_t		v_div;
	uint32_t		t_div;
	float			offset;
	float			toffset;

	CanvasArea(TFT_eSprite *s) {
		_spr = s;
	}
	
	void setArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
		X_onScreen = x;
		Y_onScreen = y;
		Width = w;
		Height = h;
		GND_Ypos = Height - ((Height / 2) % grid_size);
		CurveDrawBuff = new uint16_t[Width];
	}
	
	void drawCurve(SignalInfo *Wave);
	void genDrawBuffer(SignalInfo *Wave);

	inline void flushDrawArea(void) {
		pushScreenBuffer(X_onScreen, Y_onScreen, Width, Height);
	}

	inline void fillArea(uint32_t color) {
		_spr->fillRect(X_onScreen, Y_onScreen,
				Width, Height, color);
	}
	#define fillArea fillRect
	
	inline void drawString(const String &string, uint16_t x, uint16_t y) {
		_spr->drawString(string, X_onScreen + x, Y_onScreen + y);
	}

	inline void drawPixel(uint16_t x, uint16_t y, uint32_t color) {
		_spr->drawPixel(X_onScreen + x, Y_onScreen + y, color);
	}
	inline void drawLine(uint16_t x0, uint16_t y0,
			uint16_t x1, uint16_t y1, uint32_t color) {
		_spr->drawLine(X_onScreen + x0, Y_onScreen + y0,
				X_onScreen + x1, Y_onScreen + y1, color);
	}
	inline void drawRect(uint16_t x, uint16_t y,
			uint16_t w, uint16_t h, uint32_t color) {
		_spr->drawRect(X_onScreen + x, Y_onScreen + y, w, h, color);
	}


private:
	TFT_eSprite		*_spr;
	uint16_t		X_onScreen;
	uint16_t		Y_onScreen;
	uint			GND_Ypos;		// Y-position of the votage 0 on screen
	uint16_t		*CurveDrawBuff;	// Only stores Screen-Y coords of the Wave curve

	float to_scale(float reading) {
		return GND_Ypos - (to_voltage(reading) + offset)
				* grid_size * 1000 / v_div;
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

inline void pushScreenBuffer(uint16_t x = 0, uint16_t y = 0,
		uint16_t w = Canvas.ScreenWidth,
		uint16_t h = Canvas.ScreenHeight) {
	#ifdef AMOLED
		lcd_PushColors(x, y, Canvas.ScreenWidth, Canvas.ScreenHeight, (uint16_t *)spr.getPointer());
	#else
		spr.pushSprite(x, y);
	#endif
}