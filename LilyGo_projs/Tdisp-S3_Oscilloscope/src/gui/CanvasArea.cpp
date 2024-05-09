#include "CanvasArea.hpp"
#include "gui_cfg.h"

#include "algo/filters.h"
#include "options/options.h"
/*================================================================================*
 *										private members							  *
 *================================================================================*/
void CanvasArea::castPosition(int32_t &x, int32_t &y) {
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= Width) x = Width - 1;
	if (y >= Height) y = Height - 1;
}
void CanvasArea::castPosition(int32_t &x, int32_t &y, int32_t &w, int32_t &h) {
	if ((x + w) > Width) w = Width - x;
	if ((y + h) > Height) h = Height - y;
	castPosition(x, y);
}
void CanvasArea::castStringPosition(int32_t &x, int32_t &y, const String &string) {
	int strPixLen = FONT_WIDTH * string.length();
	if (x < 0 && strPixLen <= Width)
		x = Width - strPixLen + x;
	else
		x = abs(x);

	if (y < 0 && strPixLen <= Height)
		y = Height - FONT_HEIGHT + y;
	else
		y = abs(y);

	castPosition(x, y);
}
bool CanvasArea::posValid(int32_t x, int32_t y) {
	return ((x >= 0 && x < Width) && (y >= 0 &&  y < Height));
}

/*================================================================================*
 *										private members							  *
 *================================================================================*/
CanvasArea::CanvasArea(TFT_eSprite *spr) {
	_spr = spr;
}


void CanvasArea::setArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	CanvasPos.Start.X = x;
	CanvasPos.Start.Y = y;
	Width = w;
	Height = h;
	CanvasPos.End.X = x + w - 1;
	CanvasPos.End.Y = y + h - 1;
	_spr->createSprite(Width, Height);
	_spr->setTextColor(TxtFG_color, TxtBG_color, true);

	GND_Ypos = Height - ((Height / 2) % grid_size) - 1;
	CurveDrawBuff = new int32_t[Width];
}
void CanvasArea::setArea(Point2D start, Point2D end) {
	setArea(start.X, start.Y, end.X - start.X, end.Y - start.Y);
}
void CanvasArea::setArea(Extent2D area) {
	setArea(area.Start, area.End);
}


void CanvasArea::setColors(uint32_t BG, uint32_t TxtFG, uint32_t TxtBG) {
	BG_color = BG;
	TxtFG_color = TxtFG;
	TxtBG_color = TxtBG;
	_spr->setTextColor(TxtFG_color, TxtBG_color, true);
}


Point2D CanvasArea::getStartOnCanvas() {
	return CanvasPos.Start;
}
Point2D CanvasArea::getEndOnCanvas() {
	return CanvasPos.End;
}

void CanvasArea::drawBorder(int32_t radius, uint32_t color) {
	int32_t startX = CanvasPos.Start.X;
	int32_t startY = CanvasPos.Start.Y;
	_spr->drawSmoothRoundRect(startX - radius, startY - radius,radius * 2, radius,
			Width - 1 + 2 * radius, Height - 1 + 2 * radius, color, BG_DARK_GRAY);
}


extern void pushScreenBuffer(TFT_eSprite *s, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void CanvasArea::flushDrawArea(void) {
	pushScreenBuffer(_spr, CanvasPos.Start.X, CanvasPos.Start.Y, Width, Height);
}


void CanvasArea::fillArea(uint32_t color) {
	_spr->fillRect(CanvasPos.Start.X, CanvasPos.Start.Y, Width, Height, color);
}
void CanvasArea::fillRect(uint32_t color) {
	fillArea(color);
}


void CanvasArea::drawString(const String &string, int32_t x, int32_t y) {
	castStringPosition(x, y, string);
	_spr->drawString(string, x, y);
}
void CanvasArea::drawPixel(int32_t x, int32_t y, uint32_t color) {
	castPosition(x, y);
	_spr->drawPixel(x, y, color);
}
void CanvasArea::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
	castPosition(x0, y0);
	castPosition(x1, y1);
	_spr->drawLine(x0, y0, x1, y1, color);
}
void CanvasArea::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	castPosition(x, y, w, h);
	_spr->drawRect(x, y, w, h, color);
}


void CanvasArea::clearArea(void) {
	fillArea(BG_Color);
}



int32_t CanvasArea::to_scale(float reading) {
	int32_t VoltPixels = (to_voltage(reading) + offset) * grid_size * 1000 / v_div;
	return GND_Ypos - VoltPixels;
}

float CanvasArea::dataPerPixel(SignalInfo *Wave) {
	// t_div counted in micro_seconds
	float uS_per_pixel = (float)t_div / grid_size;
	float sample_per_uS = Wave->SampleRate / 1000000.0;
	return uS_per_pixel * sample_per_uS;
}