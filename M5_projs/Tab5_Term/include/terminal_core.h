#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ui_theme.h"

namespace terminal {

using ResponseWriter = void (*)(const uint8_t *data, size_t length);

struct TerminalConfig {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint16_t max_columns;
    uint16_t max_rows;
    uint8_t text_size;
    const ui::TerminalTheme *theme;
    ResponseWriter response_writer;
};

void begin(const TerminalConfig& config);
void reset();
void redraw();
void writeByte(uint8_t byte);
void writeBytes(const uint8_t *data, size_t length);
uint16_t columns();
uint16_t rows();
bool applicationCursorMode();
bool applicationKeypadMode();
bool bracketedPasteMode();

} // namespace terminal
