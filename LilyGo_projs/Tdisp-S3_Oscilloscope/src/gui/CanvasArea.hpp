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
	uint32_t		BG_Color		= BACK_GROUND_COLOR;

	int32_t			*CurveDrawBuff;	// Only stores Screen-Y coords of the Wave curve


	CanvasArea(TFT_eSprite *spr);
	
	void setArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	void setArea(Point2D start, Point2D end);
	void setArea(Extent2D area);

	void setColors(uint32_t BG, uint32_t TxtFG, uint32_t TxtBG);

	Point2D getStartOnCanvas();
	Point2D getEndOnCanvas();
	
	void flushDrawArea(void);

	void drawBorder(int32_t radius, uint32_t color);

	void fillArea(uint32_t color);
	void fillRect(uint32_t color);
	
	void drawString(const String &string, int32_t x, int32_t y);
	void drawPixel(int32_t x, int32_t y, uint32_t color);
	void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
	void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

	void clearArea(void);


	bool posValid(int32_t x, int32_t y);
	int32_t to_scale(float reading);

private:
	TFT_eSprite		*_spr;
	Extent2D		CanvasPos;
	uint			GND_Ypos;		// Y-position of the votage 0 on screen
	uint32_t		BG_color		= BG_DARK_GRAY;
	uint32_t		TxtFG_color		= TFT_GREEN;
	uint32_t		TxtBG_color		= BG_DARK_GRAY;


	void castPosition(int32_t &x, int32_t &y);
	void castPosition(int32_t &x, int32_t &y, int32_t &w, int32_t &h);
	void castStringPosition(int32_t &x, int32_t &y, const String &string);
};


extern CanvasArea CurveArea;
extern CanvasArea InfoArea;