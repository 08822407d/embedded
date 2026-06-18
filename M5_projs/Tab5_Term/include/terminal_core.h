#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ui_theme.h"

namespace terminal {

// terminal_core owns terminal parsing, screen state, rendering, and terminal
// replies. Callers provide bytes and an optional response writer; they should
// not manipulate cells or write replies directly to a transport.
using ResponseWriter = void (*)(const uint8_t *data, size_t length);

enum class TerminalCellWidth : uint8_t {
    Continuation = 0,
    Single = 1,
    Wide = 2,
};

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

struct TerminalGeometry {
    int32_t x;
    int32_t y;
    int32_t pixel_width;
    int32_t pixel_height;
    int32_t rendered_width;
    int32_t rendered_height;
    int32_t cell_width;
    int32_t cell_height;
    uint16_t columns;
    uint16_t rows;
};

// Snapshots are diagnostic and transport-readiness APIs. They expose the
// terminal state without allowing external modules to mutate the screen model.
struct TerminalStateSnapshot {
    uint16_t columns;
    uint16_t rows;
    uint16_t cursor_column;
    uint16_t cursor_row;
    uint16_t saved_column;
    uint16_t saved_row;
    uint16_t scroll_top;
    uint16_t scroll_bottom;
    uint16_t current_foreground;
    uint16_t current_background;
    uint8_t current_attributes;
    bool wrap_enabled;
    bool wrap_pending;
    bool origin_mode;
    bool application_cursor_mode;
    bool application_keypad_mode;
    bool bracketed_paste_mode;
    bool mouse_tracking_mode;
    bool cursor_visible;
    bool alternate_screen;
    uint32_t buffer_hash;
};

struct TerminalCellSnapshot {
    char text[9];
    uint16_t foreground;
    uint16_t background;
    uint8_t attributes;
    TerminalCellWidth width;
    bool dec_special_graphics;
};

// Public API used by the firmware, regression tools, and future mature
// transports. All printable host output should enter through writeByte or
// writeBytes so parser state, Unicode width, wrapping, and diagnostics remain
// coherent.
void begin(const TerminalConfig& config);
void reset();
void redraw();
void writeByte(uint8_t byte);
void writeBytes(const uint8_t *data, size_t length);
uint16_t columns();
uint16_t rows();
bool getGeometry(TerminalGeometry *geometry);
bool applicationCursorMode();
bool applicationKeypadMode();
bool bracketedPasteMode();
bool getStateSnapshot(TerminalStateSnapshot *snapshot);
bool getCellSnapshot(uint16_t row, uint16_t column, TerminalCellSnapshot *snapshot);
uint32_t rowHash(uint16_t row);
size_t copyRowText(
    uint16_t row,
    char *buffer,
    size_t capacity,
    bool trim_trailing_spaces = true);

} // namespace terminal
