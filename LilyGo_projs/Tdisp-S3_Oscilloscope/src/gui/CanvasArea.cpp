#include "CanvasArea.hpp"
#include "gui_cfg.h"

#include "algo/filters.h"
#include "options/options.h"

#define DRAW_TIME_NUM	60
unsigned long DrawScreenTimes[DRAW_TIME_NUM] = { 0 };
float ScreenFPS = 0;


/*================================================================================*
 *										private members							  *
 *================================================================================*/
int32_t CanvasArea::to_scale(float reading) {
	int32_t VoltPixels = (to_voltage(reading) + offset)
							* grid_size * 1000 / v_div;
	return GND_Ypos - VoltPixels;
}

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


void CanvasArea::genDrawBuffer(SignalInfo *Wave) 
{
	uint curve_color = TFT_SKYBLUE;
	uint32_t trigger0 = Wave->TrigIdx_0;
	uint32_t *AdcDataBuf = Wave->SampleBuff;
	//screen wave drawing
	low_pass filter(0.99);
	mean_filter mfilter(5);
	mfilter.init(AdcDataBuf[trigger0]);
	filter._value = AdcDataBuf[trigger0];
	float data_per_pixel = ((float)CurveArea.t_div / CurveArea.grid_size) / (Wave->SampleRate / 1000.0);

	uint32_t index_offset = (uint32_t)(CurveArea.toffset / data_per_pixel);
	trigger0 += index_offset;  
	uint32_t old_index = trigger0;
	int32_t n_data = 0, o_data = to_scale(AdcDataBuf[trigger0]);
	CurveDrawBuff[0] = o_data;
	for (uint32_t i = 1; i < CurveArea.Width; i++) {
		uint32_t index = trigger0 + (uint32_t)((i + 1) * data_per_pixel);
		if (index < Wave->SampleNum) {
			if (GlobOpts.current_filter == 2)
				n_data = to_scale(mfilter.filter((float)AdcDataBuf[index]));
			else if (GlobOpts.current_filter == 3)
				n_data = to_scale(filter.filter((float)AdcDataBuf[index]));
			else
				n_data = to_scale(AdcDataBuf[index]);

			CurveDrawBuff[i] = n_data;
		} else {
			break;
		}
		old_index = index;
	}
}
void CanvasArea::drawCurve(SignalInfo *Wave) {
	uint curve_color = TFT_SKYBLUE;
	int32_t
		currY,
		prevY = CurveDrawBuff[0];
	for (uint32_t i = 1; i < Width; i++) {
		currY = CurveDrawBuff[i];
		if (posValid(i - 1, prevY) && posValid(i, currY))
			drawLine(i - 1, prevY, i, currY, curve_color);
		prevY = currY;
	}
	drawString(String(ScreenFPS, 1) + "FPS", FONT_WIDTH, FONT_HEIGHT);

	// draw screen performance
	unsigned long timestamp = micros();
	memmove(&DrawScreenTimes[0], &DrawScreenTimes[1],
			(DRAW_TIME_NUM - 1) * sizeof(typeof(DrawScreenTimes[0])));
	DrawScreenTimes[DRAW_TIME_NUM - 1] = timestamp;
	if (DrawScreenTimes[0] >= 0)
		ScreenFPS =  (1000000.0 * (DRAW_TIME_NUM - 1)) /
						(timestamp - DrawScreenTimes[0]);

	drawBorder(TFT_DARKGREY);
}
void CanvasArea::drawBorder(uint32_t color) {
	int32_t startX = CanvasPos.Start.X;
	int32_t startY = CanvasPos.Start.Y;
	int32_t radius = 20;
	_spr->drawSmoothRoundRect(startX - radius, startY - radius, radius * 2, radius,
			Width - 1 + 2 * radius, Height - 1 + 2 * radius, color, BG_DARK_GRAY);
}


extern void pushScreenBuffer(TFT_eSprite *s, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void CanvasArea::flushDrawArea(void) {
	pushScreenBuffer(_spr, CanvasPos.Start.X, CanvasPos.Start.Y, Width, Height);
}


void CanvasArea::fillArea(uint32_t color) {
	_spr->fillRect(CanvasPos.Start.X, CanvasPos.Start.Y,
			Width, Height, color);
}
void CanvasArea::fillRect(uint32_t color) {
	fillArea(color);
}


void CanvasArea::drawString(const String &string, int32_t x, int32_t y) {
	castStringPosition(x, y, string);
	_spr->drawString(string, CanvasPos.Start.X + x, CanvasPos.Start.Y + y);
}
void CanvasArea::drawPixel(int32_t x, int32_t y, uint32_t color) {
	castPosition(x, y);
	_spr->drawPixel(CanvasPos.Start.X + x, CanvasPos.Start.Y + y, color);
}
void CanvasArea::drawLine(int32_t x0, int32_t y0,
		int32_t x1, int32_t y1, uint32_t color) {
	castPosition(x0, y0);
	castPosition(x1, y1);
	_spr->drawLine(CanvasPos.Start.X + x0, CanvasPos.Start.Y + y0,
			CanvasPos.Start.X + x1, CanvasPos.Start.Y + y1, color);
}
void CanvasArea::drawRect(int32_t x, int32_t y,
		int32_t w, int32_t h, uint32_t color) {
	castPosition(x, y, w, h);
	_spr->drawRect(CanvasPos.Start.X + x, CanvasPos.Start.Y + y, w, h, color);
}


void CanvasArea::clearArea(void) {
	fillArea(BG_Color);
}
