#pragma once

#include <stdint.h>

#include "ui_theme.h"

namespace ui {

void beginStatusBar(const TerminalTheme& theme);
void drawStatusBar(const char *title);
void refreshTemperatureStatus(bool force);
void refreshBatteryStatus(bool force);
void updateStatusBarTouch();
void applyTerminalTextStyle();
uint16_t terminalBackground();

} // namespace ui
