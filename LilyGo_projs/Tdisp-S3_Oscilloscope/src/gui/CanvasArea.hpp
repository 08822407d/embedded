#pragma once

#include <stdint.h>

#include <SPI.h>
#include <TFT_eSPI.h>

#ifdef AMOLED
#  include "../drv/rm67162.h"
#endif

#include "../configs.h"
#include "../utils/Geometry.hpp"
#include "../algo/data_analysis.h"


extern void pushScreenBuffer(uint16_t x,
		uint16_t y, uint16_t w, uint16_t h);

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
		CanvasPos.Start.X = x;
		CanvasPos.Start.Y = y;
		Width = w;
		Height = h;
		CanvasPos.End.X = x + w - 1;
		CanvasPos.End.Y = y + h - 1;

		GND_Ypos = Height - ((Height / 2) % grid_size) - 1;
		CurveDrawBuff = new int32_t[Width];
	}
	void setArea(Point2D start, Point2D end) {
		setArea(start.X, start.Y, end.X - start.X, end.Y - start.Y);
	}
	void setArea(Extent2D area) {
		setArea(area.Start, area.End);
	}

	Point2D getStartOnCanvas() {
		return CanvasPos.Start;
	}
	Point2D getEndOnCanvas() {
		return CanvasPos.End;
	}
	
	void drawCurve(SignalInfo *Wave);
	void genDrawBuffer(SignalInfo *Wave);

	void flushDrawArea(void) {
		pushScreenBuffer(CanvasPos.Start.X, CanvasPos.Start.Y, Width, Height);
	}

	void fillArea(uint32_t color) {
		_spr->fillRect(CanvasPos.Start.X, CanvasPos.Start.Y,
				Width, Height, color);
	}
	void fillRect(uint32_t color) {
		fillArea(color);
	}
	
	
	void drawString(const String &string, int32_t x, int32_t y) {
		castPosition(x, y);
		_spr->drawString(string, CanvasPos.Start.X + x, CanvasPos.Start.Y + y);
	}

	void drawPixel(int32_t x, int32_t y, uint32_t color) {
		castPosition(x, y);
		_spr->drawPixel(CanvasPos.Start.X + x, CanvasPos.Start.Y + y, color);
	}
	void drawLine(int32_t x0, int32_t y0,
			int32_t x1, int32_t y1, uint32_t color) {
		castPosition(x0, y0);
		castPosition(x1, y1);
		_spr->drawLine(CanvasPos.Start.X + x0, CanvasPos.Start.Y + y0,
				CanvasPos.Start.X + x1, CanvasPos.Start.Y + y1, color);
	}
	void drawRect(int32_t x, int32_t y,
			int32_t w, int32_t h, uint32_t color) {
		castPosition(x, y, w, h);
		_spr->drawRect(CanvasPos.Start.X + x, CanvasPos.Start.Y + y, w, h, color);
	}

	void clearArea(void) {
		fillArea(BG_Color);
	}


private:
	TFT_eSprite		*_spr;
	Extent2D		CanvasPos;
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

