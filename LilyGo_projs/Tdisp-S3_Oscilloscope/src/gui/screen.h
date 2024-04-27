#pragma once

#include <SPI.h>
#include <TFT_eSPI.h>

#ifdef AMOLED
#  include "../drv/rm67162.h"
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
	bool			Dirty		= false;
	uint16_t		Width;
	uint16_t		Height;
	uint			grid_size	= GRID_SIZE;
	uint32_t		v_div;
	uint32_t		t_div;
	float			offset;
	float			toffset;
	uint32_t		BG_Color	= BACK_GROUND_COLOR;

	CanvasArea(TFT_eSprite *s) {
		_spr = s;
	}
	
	void setArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
		X_onScreen = x;
		Y_onScreen = y;
		Width = w;
		Height = h;
		GND_Ypos = Height - ((Height / 2) % grid_size) - 1;
		CurveDrawBuff = new int32_t[Width];
	}
	
	void drawCurve(SignalInfo *Wave);
	void genDrawBuffer(SignalInfo *Wave);

	void flushDrawArea(void) {
		pushScreenBuffer(X_onScreen, Y_onScreen, Width, Height);
	}

	void fillArea(uint32_t color) {
		_spr->fillRect(X_onScreen, Y_onScreen,
				Width, Height, color);
	}
	void fillRect(uint32_t color) {
		fillArea(color);
	}
	
	
	void drawString(const String &string, int32_t x, int32_t y) {
		castPosition(x, y);
		_spr->drawString(string, X_onScreen + x, Y_onScreen + y);
	}

	void drawPixel(int32_t x, int32_t y, uint32_t color) {
		castPosition(x, y);
		_spr->drawPixel(X_onScreen + x, Y_onScreen + y, color);
	}
	void drawLine(int32_t x0, int32_t y0,
			int32_t x1, int32_t y1, uint32_t color) {
		castPosition(x0, y0);
		castPosition(x1, y1);
		_spr->drawLine(X_onScreen + x0, Y_onScreen + y0,
				X_onScreen + x1, Y_onScreen + y1, color);
	}
	void drawRect(int32_t x, int32_t y,
			int32_t w, int32_t h, uint32_t color) {
		castPosition(x, y, w, h);
		_spr->drawRect(X_onScreen + x, Y_onScreen + y, w, h, color);
	}

	void clearArea(void) {
		fillArea(BG_Color);
	}


private:
	TFT_eSprite		*_spr;
	uint16_t		X_onScreen;
	uint16_t		Y_onScreen;
	uint			GND_Ypos;		// Y-position of the votage 0 on screen
	int32_t			*CurveDrawBuff;	// Only stores Screen-Y coords of the Wave curve

	int32_t to_scale(float reading) {
		int32_t VoltPixels = (to_voltage(reading) + offset)
								* grid_size * 1000 / v_div;
		return GND_Ypos - VoltPixels;
	}

	void castPosition(int32_t &x, int32_t &y) {
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x >= Width) x = Width - 1;
		if (y >= Height) y = Height - 1;
	}
	void castPosition(int32_t &x, int32_t &y, int32_t &w, int32_t &h) {
		if ((x + w) > Width) w = Width - x;
		if ((y + h) > Height) h = Height - y;
		castPosition(x, y);
	}
	bool posValid(int32_t x, int32_t y) {
		return ((x >= 0 && x < Width) && (y >= 0 &&  y < Height));
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