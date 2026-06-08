#pragma once

#include <stdint.h>

#include "ui_theme.h"

namespace ui {

void beginStatusBar(const TerminalTheme& theme);
void drawStatusBar(const char *title);
void refreshBatteryStatus(bool force);
void applyTerminalTextStyle();
uint16_t terminalBackground();

} // namespace ui
