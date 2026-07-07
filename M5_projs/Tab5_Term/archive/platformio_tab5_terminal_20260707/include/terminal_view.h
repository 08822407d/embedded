#pragma once

#include <stddef.h>
#include <stdint.h>

#include "terminal_core.h"
#include "ui_theme.h"

namespace ui {

struct TerminalViewRect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

struct TerminalViewConfig {
    TerminalViewRect bounds;
    uint16_t columns;
    uint16_t rows;
    uint8_t text_size;
    const TerminalTheme *theme;
    terminal::ResponseWriter response_writer;
};

struct TerminalViewGeometry {
    TerminalViewRect bounds;
    TerminalViewRect content;
    int32_t cell_width;
    int32_t cell_height;
    uint16_t columns;
    uint16_t rows;
};

void beginTerminalView(const TerminalViewConfig& config);
void redrawTerminalView();
bool getTerminalViewGeometry(TerminalViewGeometry *geometry);

} // namespace ui
