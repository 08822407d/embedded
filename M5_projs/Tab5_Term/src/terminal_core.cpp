#include "terminal_core.h"

#include <M5Unified.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"

namespace terminal {
namespace {
constexpr uint16_t kMaxColumns = 96;
constexpr uint16_t kMaxRows = 96;
constexpr uint8_t kTabWidth = 8;
constexpr uint8_t kMaxCsiParams = 16;
constexpr int32_t kMissingParam = -1;

constexpr uint8_t kAttrBold = 1 << 0;
constexpr uint8_t kAttrDim = 1 << 1;
constexpr uint8_t kAttrUnderline = 1 << 2;
constexpr uint8_t kAttrInverse = 1 << 3;
constexpr uint8_t kAttrHidden = 1 << 4;

enum class ParserState : uint8_t {
    Ground,
    Escape,
    EscapeIgnoreOne,
    CharsetDesignate,
    Csi,
    Utf8,
    OscString,
    DcsString,
    StringEscape,
};

enum class StringParserReturn : uint8_t {
    Osc,
    Dcs,
};

enum class CharacterSet : uint8_t {
    Ascii,
    DecSpecialGraphics,
};

enum class CharacterSetSlot : uint8_t {
    G0,
    G1,
};

struct Cell {
    char ch;
    char text[5];
    CharacterSet charset;
    uint16_t fg;
    uint16_t bg;
    uint8_t attrs;
};

struct RuntimeSnapshot {
    uint16_t cursor_col = 0;
    uint16_t cursor_row = 0;
    uint16_t saved_col = 0;
    uint16_t saved_row = 0;
    uint16_t scroll_top = 0;
    uint16_t scroll_bottom = 0;
    uint16_t current_fg = 0;
    uint16_t current_bg = 0;
    uint8_t current_attrs = 0;
    uint16_t saved_fg = 0;
    uint16_t saved_bg = 0;
    uint8_t saved_attrs = 0;
    CharacterSet g0_charset = CharacterSet::Ascii;
    CharacterSet g1_charset = CharacterSet::Ascii;
    CharacterSet saved_g0_charset = CharacterSet::Ascii;
    CharacterSet saved_g1_charset = CharacterSet::Ascii;
    bool use_g1 = false;
    bool saved_use_g1 = false;
    bool wrap_enabled = true;
    bool wrap_pending = false;
    bool origin_mode = false;
    bool cursor_visible = true;
    bool tab_stops[kMaxColumns] = {};
};

struct TerminalState {
    TerminalConfig config = {};
    uint16_t cols = 0;
    uint16_t rows = 0;
    int32_t cell_width = 0;
    int32_t cell_height = 0;
    uint16_t cursor_col = 0;
    uint16_t cursor_row = 0;
    uint16_t saved_col = 0;
    uint16_t saved_row = 0;
    uint16_t scroll_top = 0;
    uint16_t scroll_bottom = 0;
    uint16_t default_fg = 0;
    uint16_t default_bg = 0;
    uint16_t current_fg = 0;
    uint16_t current_bg = 0;
    uint16_t saved_fg = 0;
    uint16_t saved_bg = 0;
    uint8_t current_attrs = 0;
    uint8_t saved_attrs = 0;
    CharacterSet g0_charset = CharacterSet::Ascii;
    CharacterSet g1_charset = CharacterSet::Ascii;
    CharacterSet saved_g0_charset = CharacterSet::Ascii;
    CharacterSet saved_g1_charset = CharacterSet::Ascii;
    bool use_g1 = false;
    bool saved_use_g1 = false;
    CharacterSetSlot designate_target_slot = CharacterSetSlot::G0;
    bool wrap_enabled = true;
    bool wrap_pending = false;
    bool origin_mode = false;
    bool application_cursor_mode = false;
    bool application_keypad_mode = false;
    bool bracketed_paste_mode = false;
    bool mouse_tracking_mode = false;
    bool cursor_visible = true;
    bool cursor_drawn = false;
    bool tab_stops[kMaxColumns] = {};
    bool using_alternate_screen = false;
    RuntimeSnapshot primary_runtime_snapshot = {};
    ParserState parser_state = ParserState::Ground;
    StringParserReturn string_return = StringParserReturn::Osc;
    char utf8_buffer[5] = {};
    uint8_t utf8_length = 0;
    uint8_t utf8_expected = 0;
    int32_t csi_params[kMaxCsiParams] = {};
    uint8_t csi_param_count = 0;
    uint8_t csi_param_index = 0;
    char csi_private_marker = 0;
    bool initialized = false;
};

TerminalState state;
Cell primary_cells[kMaxRows][kMaxColumns];
Cell alternate_cells[kMaxRows][kMaxColumns];
Cell (*cells)[kMaxColumns] = primary_cells;

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return static_cast<uint16_t>(
        ((red & 0xF8) << 8)
        | ((green & 0xFC) << 3)
        | (blue >> 3));
}

uint16_t scaleRgb565(uint16_t color, uint8_t numerator, uint8_t denominator)
{
    const uint8_t red = static_cast<uint8_t>(((color >> 11) & 0x1f) * 255 / 31);
    const uint8_t green = static_cast<uint8_t>(((color >> 5) & 0x3f) * 255 / 63);
    const uint8_t blue = static_cast<uint8_t>((color & 0x1f) * 255 / 31);
    return rgb565(
        static_cast<uint8_t>(red * numerator / denominator),
        static_cast<uint8_t>(green * numerator / denominator),
        static_cast<uint8_t>(blue * numerator / denominator));
}

constexpr uint16_t kAnsiColorTable[16] = {
    rgb565(0, 0, 0),
    rgb565(170, 0, 0),
    rgb565(0, 170, 0),
    rgb565(170, 85, 0),
    rgb565(0, 0, 170),
    rgb565(170, 0, 170),
    rgb565(0, 170, 170),
    rgb565(170, 170, 170),
    rgb565(85, 85, 85),
    rgb565(255, 85, 85),
    rgb565(85, 255, 85),
    rgb565(255, 255, 85),
    rgb565(85, 85, 255),
    rgb565(255, 85, 255),
    rgb565(85, 255, 255),
    rgb565(255, 255, 255),
};

uint16_t clampU16(int32_t value, uint16_t max_value)
{
    if (value <= 0) {
        return 0;
    }
    if (value >= max_value) {
        return max_value == 0 ? 0 : static_cast<uint16_t>(max_value - 1);
    }
    return static_cast<uint16_t>(value);
}

int32_t maxInt(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

int32_t minInt(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

Cell blankCell()
{
    return Cell{' ', {'\0'}, CharacterSet::Ascii, state.current_fg, state.current_bg, 0};
}

bool proportionalRenderingEnabled()
{
#if TERMINAL_RENDER_MODE == TERMINAL_RENDER_MODE_PROPORTIONAL && !TERMINAL_FORCE_FIXED_CELL_RENDERING
    return true;
#else
    return false;
#endif
}

void applyDisplayFont()
{
#if TERMINAL_FONT_FACE == TERMINAL_FONT_FACE_DEJAVU18
    M5.Display.setFont(&fonts::DejaVu18);
#else
    M5.Display.setFont(&fonts::AsciiFont8x16);
#endif
    M5.Display.setTextScroll(false);
    M5.Display.setTextDatum(m5gfx::textdatum_t::top_left);
    M5.Display.setTextSize(state.config.text_size);
}

void applyDisplayTextStyle(uint16_t fg, uint16_t bg)
{
    applyDisplayFont();
    M5.Display.setTextColor(fg, bg);
}

bool isUtf8CellText(const char *text)
{
    return text != nullptr && (static_cast<uint8_t>(text[0]) & 0x80) != 0;
}

bool isUtf8Continuation(uint8_t byte)
{
    return (byte & 0xc0) == 0x80;
}

uint32_t decodeUtf8CellText(const char *text)
{
    if (text == nullptr) {
        return 0;
    }

    const uint8_t b0 = static_cast<uint8_t>(text[0]);
    const uint8_t b1 = static_cast<uint8_t>(text[1]);
    const uint8_t b2 = static_cast<uint8_t>(text[2]);
    const uint8_t b3 = static_cast<uint8_t>(text[3]);
    if (b0 < 0x80) {
        return b0;
    }
    if (b0 >= 0xc2 && b0 <= 0xdf && isUtf8Continuation(b1) && text[2] == '\0') {
        return static_cast<uint32_t>(((b0 & 0x1f) << 6) | (b1 & 0x3f));
    }
    if (b0 >= 0xe0 && b0 <= 0xef && isUtf8Continuation(b1) && isUtf8Continuation(b2) && text[3] == '\0') {
        return static_cast<uint32_t>(((b0 & 0x0f) << 12) | ((b1 & 0x3f) << 6) | (b2 & 0x3f));
    }
    if (b0 >= 0xf0 && b0 <= 0xf4 && isUtf8Continuation(b1) && isUtf8Continuation(b2) && isUtf8Continuation(b3)) {
        return static_cast<uint32_t>(
            ((b0 & 0x07) << 18) | ((b1 & 0x3f) << 12) | ((b2 & 0x3f) << 6) | (b3 & 0x3f));
    }
    return 0xfffd;
}

void drawSixByEightGlyph(
    const uint8_t glyph[8],
    int32_t x,
    int32_t y,
    int32_t box_width,
    uint16_t fg,
    bool bold)
{
    const int32_t scale = maxInt(1, minInt(maxInt(1, box_width) / 6, state.cell_height / 8));
    const int32_t glyph_width = 6 * scale;
    const int32_t glyph_height = 8 * scale;
    const int32_t glyph_x = x + maxInt(0, (box_width - glyph_width) / 2);
    const int32_t glyph_y = y + maxInt(0, (state.cell_height - glyph_height) / 2);
    const int32_t draw_width = scale + (bold ? 1 : 0);

    for (int32_t row = 0; row < 8; ++row) {
        for (int32_t col = 0; col < 6; ++col) {
            if ((glyph[row] & (1 << (5 - col))) == 0) {
                continue;
            }
            M5.Display.fillRect(
                glyph_x + col * scale,
                glyph_y + row * scale,
                draw_width,
                scale,
                fg);
        }
    }
}

bool drawUnicodeFallbackCell(uint32_t codepoint, int32_t x, int32_t y, int32_t box_width, uint16_t fg, bool bold)
{
    // These single-cell glyphs keep UTF-8 validation deterministic until a
    // complete Unicode-capable terminal font is selected.
    static constexpr uint8_t kLatinSmallLetterEAcute[8] = {
        0b001100,
        0b000000,
        0b011100,
        0b100010,
        0b111110,
        0b100000,
        0b011110,
        0b000000,
    };
    static constexpr uint8_t kGreekCapitalOmega[8] = {
        0b011110,
        0b100001,
        0b100001,
        0b100001,
        0b010010,
        0b010010,
        0b110011,
        0b000000,
    };
    static constexpr uint8_t kCjkMiddle[8] = {
        0b001000,
        0b111110,
        0b101010,
        0b101010,
        0b111110,
        0b001000,
        0b001000,
        0b000000,
    };

    switch (codepoint) {
    case 0x00e9:
        drawSixByEightGlyph(kLatinSmallLetterEAcute, x, y, box_width, fg, bold);
        return true;
    case 0x03a9:
        drawSixByEightGlyph(kGreekCapitalOmega, x, y, box_width, fg, bold);
        return true;
    case 0x4e2d:
        drawSixByEightGlyph(kCjkMiddle, x, y, box_width, fg, bold);
        return true;
    default:
        return false;
    }
}

void drawAsciiCellText(
    const char *text,
    int32_t x,
    int32_t y,
    int32_t box_width,
    uint16_t fg,
    uint16_t bg,
    bool bold,
    bool center_in_box)
{
    applyDisplayTextStyle(fg, bg);
    const int32_t text_width = M5.Display.textWidth(text) + (bold ? 1 : 0);
    const int32_t text_height = M5.Display.fontHeight();
    const int32_t text_x = center_in_box ? x + maxInt(0, (box_width - text_width) / 2) : x;
    const int32_t text_y = y + maxInt(0, (state.cell_height - text_height) / 2);
    M5.Display.drawString(text, text_x, text_y);
    if (bold && box_width > 2) {
        M5.Display.drawString(text, text_x + 1, text_y);
    }
}

void drawCellText(const char *text, int32_t x, int32_t y, int32_t box_width, uint16_t fg, uint16_t bg, bool bold)
{
    if (text == nullptr || text[0] == '\0') {
        return;
    }

    const bool center_in_box = !proportionalRenderingEnabled();
    if (isUtf8CellText(text)) {
        const uint32_t codepoint = decodeUtf8CellText(text);
        if (drawUnicodeFallbackCell(codepoint, x, y, box_width, fg, bold)) {
            return;
        }
        text = "?";
        bold = false;
    } else {
        drawAsciiCellText(text, x, y, box_width, fg, bg, bold, center_in_box);
        return;
    }

    drawAsciiCellText(text, x, y, box_width, fg, bg, bold, center_in_box);
}

int32_t terminalRenderWidth()
{
    return proportionalRenderingEnabled() ? state.config.width : state.cols * state.cell_width;
}

void setTerminalScrollRect()
{
    M5.Display.setScrollRect(
        state.config.x,
        state.config.y,
        terminalRenderWidth(),
        state.rows * state.cell_height,
        state.default_bg);
}

void setScrollRectForRows(uint16_t top, uint16_t bottom)
{
    if (state.rows == 0 || top >= state.rows || bottom >= state.rows || top > bottom) {
        setTerminalScrollRect();
        return;
    }

    M5.Display.setScrollRect(
        state.config.x,
        state.config.y + top * state.cell_height,
        terminalRenderWidth(),
        (bottom - top + 1) * state.cell_height,
        state.default_bg);
}

void clearWrapPending()
{
    state.wrap_pending = false;
}

void sendResponse(const char *text)
{
    if (state.config.response_writer == nullptr || text == nullptr) {
        return;
    }

    state.config.response_writer(reinterpret_cast<const uint8_t *>(text), strlen(text));
}

const char *cellText(const Cell& cell, char fallback_text[2])
{
    const char *text = cell.text[0] != '\0' ? cell.text : "";
    fallback_text[0] = cell.ch;
    fallback_text[1] = '\0';
    if (text[0] == '\0') {
        text = fallback_text;
    }
    return text;
}

int32_t textAdvance(const char *text, bool bold)
{
    if (text == nullptr || text[0] == '\0') {
        text = " ";
    }
    applyDisplayFont();
    const int32_t measured = M5.Display.textWidth(text) + (bold ? 1 : 0);
    return maxInt(TERMINAL_PROPORTIONAL_MIN_ADVANCE, measured + TERMINAL_PROPORTIONAL_GLYPH_GAP);
}

int32_t cellAdvance(const Cell& cell)
{
    if (!proportionalRenderingEnabled()) {
        return state.cell_width;
    }
    if (cell.charset == CharacterSet::DecSpecialGraphics && cell.ch != ' ') {
        return state.cell_width;
    }
    if (isUtf8CellText(cell.text)) {
        return state.cell_width;
    }

    char fallback_text[2] = {};
    const char *text = cellText(cell, fallback_text);
    return textAdvance(text, (cell.attrs & kAttrBold) != 0);
}

int32_t rowXForColumn(uint16_t row, uint16_t col)
{
    int32_t x = state.config.x;
    if (row >= state.rows) {
        return x;
    }
    const uint16_t stop_col = col > state.cols ? state.cols : col;
    for (uint16_t current_col = 0; current_col < stop_col; ++current_col) {
        x += cellAdvance(cells[row][current_col]);
    }
    return x;
}

void renderCellAt(uint16_t row, uint16_t col, int32_t x, int32_t box_width, bool cursor)
{
    if (!state.initialized || row >= state.rows || col >= state.cols) {
        return;
    }

    box_width = maxInt(1, box_width);
    const Cell& cell = cells[row][col];
    uint16_t fg = cell.fg;
    uint16_t bg = cell.bg;
    if ((cell.attrs & kAttrDim) != 0) {
        fg = scaleRgb565(fg, 3, 5);
    }
    if ((cell.attrs & kAttrInverse) != 0) {
        const uint16_t swapped = fg;
        fg = bg;
        bg = swapped;
    }
    const uint16_t cursor_fg = bg;
    const uint16_t cursor_bg = fg;
    const bool hidden = (cell.attrs & kAttrHidden) != 0;
    if (hidden) {
        fg = bg;
    }
    const bool bold = (cell.attrs & kAttrBold) != 0;
    if (cursor && state.cursor_visible) {
        fg = cursor_fg;
        bg = cursor_bg;
    }

    const int32_t y = state.config.y + row * state.cell_height;
    const int32_t render_right = state.config.x + terminalRenderWidth();
    if (x >= render_right) {
        return;
    }
    const int32_t clipped_width = minInt(box_width, render_right - x);
    box_width = clipped_width;
    M5.Display.fillRect(x, y, clipped_width, state.cell_height, bg);
    if (!hidden && cell.charset == CharacterSet::DecSpecialGraphics && cell.ch != ' ') {
        const int32_t left = x + 1;
        const int32_t right = x + box_width - 2;
        const int32_t top = y + 1;
        const int32_t bottom = y + state.cell_height - 2;
        const int32_t mid_x = x + box_width / 2;
        const int32_t mid_y = y + state.cell_height / 2;
        const int32_t thickness = minInt(2, maxInt(1, minInt(box_width, state.cell_height) / 8));
        const auto draw_horizontal_segment = [&](int32_t x0, int32_t x1) {
            for (int32_t offset = 0; offset < thickness; ++offset) {
                M5.Display.drawFastHLine(x0, mid_y + offset, x1 - x0 + 1, fg);
            }
        };
        const auto draw_vertical_segment = [&](int32_t y0, int32_t y1) {
            for (int32_t offset = 0; offset < thickness; ++offset) {
                M5.Display.drawFastVLine(mid_x + offset, y0, y1 - y0 + 1, fg);
            }
        };
        const auto draw_horizontal = [&]() {
            draw_horizontal_segment(left, right);
        };
        const auto draw_vertical = [&]() {
            draw_vertical_segment(top, bottom);
        };
        switch (cell.ch) {
        case 'j': // lower-right corner
            draw_horizontal_segment(left, mid_x);
            draw_vertical_segment(top, mid_y);
            break;
        case 'k': // upper-right corner
            draw_horizontal_segment(left, mid_x);
            draw_vertical_segment(mid_y, bottom);
            break;
        case 'l': // upper-left corner
            draw_horizontal_segment(mid_x, right);
            draw_vertical_segment(mid_y, bottom);
            break;
        case 'm': // lower-left corner
            draw_horizontal_segment(mid_x, right);
            draw_vertical_segment(top, mid_y);
            break;
        case 'n': // crossing
            draw_horizontal();
            draw_vertical();
            break;
        case 'q': // horizontal line
            draw_horizontal();
            break;
        case 't': // left tee
            draw_horizontal_segment(mid_x, right);
            draw_vertical();
            break;
        case 'u': // right tee
            draw_horizontal_segment(left, mid_x);
            draw_vertical();
            break;
        case 'v': // bottom tee
            draw_horizontal();
            draw_vertical_segment(top, mid_y);
            break;
        case 'w': // top tee
            draw_horizontal();
            draw_vertical_segment(mid_y, bottom);
            break;
        case 'x': // vertical line
            draw_vertical();
            break;
        case '`': // diamond
        case '~': // bullet
            M5.Display.fillCircle(mid_x, mid_y, maxInt(1, minInt(box_width, state.cell_height) / 5), fg);
            break;
        case 'a': // checkerboard
            for (int32_t py = top; py <= bottom; py += 2) {
                for (int32_t px = left + ((py - top) & 2); px <= right; px += 4) {
                    M5.Display.drawPixel(px, py, fg);
                }
            }
            break;
        case 'o':
            M5.Display.drawFastHLine(left, y + state.cell_height * 1 / 4, right - left + 1, fg);
            break;
        case 'p':
            M5.Display.drawFastHLine(left, y + state.cell_height * 2 / 5, right - left + 1, fg);
            break;
        case 'r':
            M5.Display.drawFastHLine(left, y + state.cell_height * 3 / 5, right - left + 1, fg);
            break;
        case 's':
            M5.Display.drawFastHLine(left, y + state.cell_height * 3 / 4, right - left + 1, fg);
            break;
        default:
            char fallback_text[2] = {};
            drawCellText(cellText(cell, fallback_text), x, y, box_width, fg, bg, bold);
            break;
        }
    } else if (!hidden && cell.ch != ' ') {
        char fallback_text[2] = {};
        drawCellText(cellText(cell, fallback_text), x, y, box_width, fg, bg, bold);
    }

    if (!hidden && (cell.attrs & kAttrUnderline) != 0) {
        const int32_t underline_y = y + maxInt(0, state.cell_height - 2);
        M5.Display.drawFastHLine(x + 1, underline_y, maxInt(1, box_width - 2), fg);
    }
}

void renderRowWithCursor(uint16_t row, bool include_cursor);

void renderCell(uint16_t row, uint16_t col, bool cursor)
{
    if (proportionalRenderingEnabled()) {
        renderRowWithCursor(row, cursor && row == state.cursor_row && col == state.cursor_col);
        return;
    }

    renderCellAt(row, col, state.config.x + col * state.cell_width, state.cell_width, cursor);
}

void eraseCursor()
{
    if (!state.cursor_drawn) {
        return;
    }

    if (proportionalRenderingEnabled()) {
        renderRowWithCursor(state.cursor_row, false);
    } else {
        renderCell(state.cursor_row, state.cursor_col, false);
    }
    state.cursor_drawn = false;
}

void drawCursor()
{
    if (!state.cursor_visible || state.rows == 0 || state.cols == 0) {
        return;
    }

    if (proportionalRenderingEnabled()) {
        renderRowWithCursor(state.cursor_row, true);
    } else {
        renderCell(state.cursor_row, state.cursor_col, true);
    }
    state.cursor_drawn = true;
}

void clearCell(uint16_t row, uint16_t col, bool render)
{
    if (row >= state.rows || col >= state.cols) {
        return;
    }

    cells[row][col] = blankCell();
    if (render) {
        if (proportionalRenderingEnabled()) {
            renderRowWithCursor(row, false);
        } else {
            renderCell(row, col, false);
        }
    }
}

void clearAllCells(bool render)
{
    const Cell blank = blankCell();
    for (uint16_t row = 0; row < state.rows; ++row) {
        for (uint16_t col = 0; col < state.cols; ++col) {
            cells[row][col] = blank;
        }
    }

    if (render) {
        M5.Display.fillRect(
            state.config.x,
            state.config.y,
            terminalRenderWidth(),
            state.rows * state.cell_height,
            blank.bg);
    }
}

void renderRowWithCursor(uint16_t row, bool include_cursor)
{
    if (row >= state.rows) {
        return;
    }

    const int32_t y = state.config.y + row * state.cell_height;
    if (proportionalRenderingEnabled()) {
        const int32_t render_width = terminalRenderWidth();
        const int32_t render_right = state.config.x + render_width;
        M5.Display.fillRect(state.config.x, y, render_width, state.cell_height, state.default_bg);
        int32_t x = state.config.x;
        for (uint16_t col = 0; col < state.cols && x < render_right; ++col) {
            const int32_t advance = cellAdvance(cells[row][col]);
            renderCellAt(row, col, x, advance, include_cursor && row == state.cursor_row && col == state.cursor_col);
            x += advance;
        }
        return;
    }

    for (uint16_t col = 0; col < state.cols; ++col) {
        renderCellAt(
            row,
            col,
            state.config.x + col * state.cell_width,
            state.cell_width,
            include_cursor && row == state.cursor_row && col == state.cursor_col);
    }
}

void renderRow(uint16_t row)
{
    renderRowWithCursor(row, false);
}

void renderRows(uint16_t top, uint16_t bottom)
{
    if (state.rows == 0 || top >= state.rows || bottom >= state.rows || top > bottom) {
        return;
    }

    for (uint16_t row = top; row <= bottom; ++row) {
        renderRow(row);
        if (row == UINT16_MAX) {
            break;
        }
    }
}

void scrollUpRegion(uint16_t top, uint16_t bottom)
{
    if (state.rows == 0 || state.cols == 0 || top >= state.rows || bottom >= state.rows || top > bottom) {
        return;
    }

    for (uint16_t row = top + 1; row <= bottom; ++row) {
        for (uint16_t col = 0; col < state.cols; ++col) {
            cells[row - 1][col] = cells[row][col];
        }
        if (row == UINT16_MAX) {
            break;
        }
    }
    for (uint16_t col = 0; col < state.cols; ++col) {
        cells[bottom][col] = blankCell();
    }

    setScrollRectForRows(top, bottom);
    M5.Display.scroll(0, -state.cell_height);
    renderRow(bottom);
    setTerminalScrollRect();
}

void scrollUp()
{
    scrollUpRegion(state.scroll_top, state.scroll_bottom);
}

void scrollDownRegion(uint16_t top, uint16_t bottom)
{
    if (state.rows == 0 || state.cols == 0 || top >= state.rows || bottom >= state.rows || top > bottom) {
        return;
    }

    for (uint16_t row = bottom; row > top; --row) {
        for (uint16_t col = 0; col < state.cols; ++col) {
            cells[row][col] = cells[row - 1][col];
        }
    }
    for (uint16_t col = 0; col < state.cols; ++col) {
        cells[top][col] = blankCell();
    }

    setScrollRectForRows(top, bottom);
    M5.Display.scroll(0, state.cell_height);
    renderRow(top);
    setTerminalScrollRect();
}

void scrollDown()
{
    scrollDownRegion(state.scroll_top, state.scroll_bottom);
}

void setCursor(uint16_t row, uint16_t col)
{
    clearWrapPending();

    if (state.rows == 0 || state.cols == 0) {
        state.cursor_row = 0;
        state.cursor_col = 0;
        return;
    }

    state.cursor_row = row >= state.rows ? state.rows - 1 : row;
    state.cursor_col = col >= state.cols ? state.cols - 1 : col;
}

void setCursorFromCsi(int32_t one_based_row, int32_t one_based_col)
{
    const uint16_t col = clampU16(one_based_col - 1, state.cols);
    uint16_t row = clampU16(one_based_row - 1, state.rows);

    if (state.origin_mode) {
        const uint16_t region_height = state.scroll_bottom - state.scroll_top + 1;
        row = static_cast<uint16_t>(
            state.scroll_top + clampU16(one_based_row - 1, region_height));
    }

    setCursor(row, col);
}

void moveCursorRelative(int32_t drow, int32_t dcol)
{
    const int32_t row = static_cast<int32_t>(state.cursor_row) + drow;
    const int32_t col = static_cast<int32_t>(state.cursor_col) + dcol;
    setCursor(clampU16(row, state.rows), clampU16(col, state.cols));
}

void saveCursor()
{
    state.saved_col = state.cursor_col;
    state.saved_row = state.cursor_row;
    state.saved_fg = state.current_fg;
    state.saved_bg = state.current_bg;
    state.saved_attrs = state.current_attrs;
    state.saved_g0_charset = state.g0_charset;
    state.saved_g1_charset = state.g1_charset;
    state.saved_use_g1 = state.use_g1;
}

void restoreCursor()
{
    state.current_fg = state.saved_fg;
    state.current_bg = state.saved_bg;
    state.current_attrs = state.saved_attrs;
    state.g0_charset = state.saved_g0_charset;
    state.g1_charset = state.saved_g1_charset;
    state.use_g1 = state.saved_use_g1;
    setCursor(state.saved_row, state.saved_col);
}

void lineFeed()
{
    clearWrapPending();

    if (state.rows == 0) {
        return;
    }

    if (state.cursor_row == state.scroll_bottom) {
        scrollUp();
    } else if (state.cursor_row + 1 < state.rows) {
        ++state.cursor_row;
    }
}

void reverseIndex()
{
    clearWrapPending();

    if (state.rows == 0) {
        return;
    }

    if (state.cursor_row == state.scroll_top) {
        scrollDown();
    } else {
        --state.cursor_row;
    }
}

void carriageReturn()
{
    clearWrapPending();
    state.cursor_col = 0;
}

void horizontalTab()
{
    clearWrapPending();

    if (state.cols == 0) {
        return;
    }

    for (uint16_t col = state.cursor_col + 1; col < state.cols; ++col) {
        if (state.tab_stops[col]) {
            state.cursor_col = col;
            return;
        }
    }

    state.cursor_col = state.cols - 1;
}

void cursorBackwardTab(int32_t count_param)
{
    clearWrapPending();
    if (state.cols == 0) {
        return;
    }

    const int32_t count = maxInt(1, count_param);
    for (int32_t step = 0; step < count; ++step) {
        bool moved = false;
        for (int32_t col = static_cast<int32_t>(state.cursor_col) - 1; col >= 0; --col) {
            if (state.tab_stops[col]) {
                state.cursor_col = static_cast<uint16_t>(col);
                moved = true;
                break;
            }
        }
        if (!moved) {
            state.cursor_col = 0;
            return;
        }
    }
}

void cursorForwardTab(int32_t count_param)
{
    const int32_t count = maxInt(1, count_param);
    for (int32_t step = 0; step < count; ++step) {
        horizontalTab();
    }
}

void backspace()
{
    clearWrapPending();

    if (state.cursor_col > 0) {
        --state.cursor_col;
    }
}

void advanceAfterCellWrite()
{
    if (state.cursor_col + 1 >= state.cols) {
        if (state.wrap_enabled) {
            state.wrap_pending = true;
        }
    } else {
        ++state.cursor_col;
    }
}

void putTextCell(const char *text, uint8_t length, char fallback_ch, CharacterSet charset)
{
    if (state.rows == 0 || state.cols == 0) {
        return;
    }

    if (state.wrap_pending) {
        state.wrap_pending = false;
        if (state.wrap_enabled) {
            state.cursor_col = 0;
            lineFeed();
        }
    }

    Cell& cell = cells[state.cursor_row][state.cursor_col];
    cell.ch = fallback_ch;
    cell.charset = charset;
    cell.fg = state.current_fg;
    cell.bg = state.current_bg;
    cell.attrs = state.current_attrs;
    for (uint8_t i = 0; i < 5; ++i) {
        cell.text[i] = '\0';
    }
    if (text != nullptr && length > 0) {
        const uint8_t copy_length = minInt(length, 4);
        for (uint8_t i = 0; i < copy_length; ++i) {
            cell.text[i] = text[i];
        }
        cell.text[copy_length] = '\0';
    }

    renderCell(state.cursor_row, state.cursor_col, false);
    advanceAfterCellWrite();
}

void putPrintable(uint8_t byte)
{
    const CharacterSet charset = state.use_g1 ? state.g1_charset : state.g0_charset;
    const char text[2] = {static_cast<char>(byte), '\0'};
    putTextCell(text, 1, static_cast<char>(byte), charset);
}

void putReplacementCharacter()
{
    putTextCell("?", 1, '?', CharacterSet::Ascii);
}

void resetTabStops()
{
    for (uint16_t col = 0; col < kMaxColumns; ++col) {
        state.tab_stops[col] = (col != 0 && (col % kTabWidth) == 0);
    }
}

void clearCurrentTabStop()
{
    if (state.cursor_col < kMaxColumns) {
        state.tab_stops[state.cursor_col] = false;
    }
}

void clearAllTabStops()
{
    for (uint16_t col = 0; col < kMaxColumns; ++col) {
        state.tab_stops[col] = false;
    }
}

void setCurrentTabStop()
{
    if (state.cursor_col < kMaxColumns) {
        state.tab_stops[state.cursor_col] = true;
    }
}

void resetScrollRegion()
{
    state.scroll_top = 0;
    state.scroll_bottom = state.rows == 0 ? 0 : static_cast<uint16_t>(state.rows - 1);
}

bool cursorInScrollRegion()
{
    return state.cursor_row >= state.scroll_top && state.cursor_row <= state.scroll_bottom;
}

void setScrollRegion(int32_t top_param, int32_t bottom_param)
{
    if (state.rows == 0) {
        return;
    }

    const int32_t top = top_param - 1;
    const int32_t bottom = bottom_param - 1;
    if (top < 0 || bottom < 0 || top >= bottom || bottom >= state.rows) {
        resetScrollRegion();
    } else {
        state.scroll_top = static_cast<uint16_t>(top);
        state.scroll_bottom = static_cast<uint16_t>(bottom);
    }

    setCursorFromCsi(1, 1);
}

void setOriginMode(bool enabled)
{
    state.origin_mode = enabled;
    setCursorFromCsi(1, 1);
}

void renderCellsInRow(uint16_t row, uint16_t start_col, uint16_t end_col)
{
    if (row >= state.rows || start_col >= state.cols || end_col >= state.cols || start_col > end_col) {
        return;
    }

    if (proportionalRenderingEnabled()) {
        renderRowWithCursor(row, false);
        return;
    }

    for (uint16_t col = start_col; col <= end_col; ++col) {
        renderCell(row, col, false);
        if (col == UINT16_MAX) {
            break;
        }
    }
}

void insertChars(int32_t count_param)
{
    clearWrapPending();
    if (state.rows == 0 || state.cols == 0) {
        return;
    }

    const uint16_t count = static_cast<uint16_t>(
        minInt(maxInt(1, count_param), state.cols - state.cursor_col));
    const uint16_t row = state.cursor_row;
    for (int32_t col = state.cols - 1; col >= static_cast<int32_t>(state.cursor_col + count); --col) {
        cells[row][col] = cells[row][col - count];
    }
    for (uint16_t col = state.cursor_col; col < state.cursor_col + count; ++col) {
        cells[row][col] = blankCell();
    }
    renderCellsInRow(row, state.cursor_col, state.cols - 1);
}

void deleteChars(int32_t count_param)
{
    clearWrapPending();
    if (state.rows == 0 || state.cols == 0) {
        return;
    }

    const uint16_t count = static_cast<uint16_t>(
        minInt(maxInt(1, count_param), state.cols - state.cursor_col));
    const uint16_t row = state.cursor_row;
    for (uint16_t col = state.cursor_col; col + count < state.cols; ++col) {
        cells[row][col] = cells[row][col + count];
    }
    for (uint16_t col = state.cols - count; col < state.cols; ++col) {
        cells[row][col] = blankCell();
    }
    renderCellsInRow(row, state.cursor_col, state.cols - 1);
}

void eraseChars(int32_t count_param)
{
    clearWrapPending();
    if (state.rows == 0 || state.cols == 0) {
        return;
    }

    const uint16_t count = static_cast<uint16_t>(
        minInt(maxInt(1, count_param), state.cols - state.cursor_col));
    for (uint16_t col = state.cursor_col; col < state.cursor_col + count; ++col) {
        clearCell(state.cursor_row, col, false);
    }
    renderCellsInRow(state.cursor_row, state.cursor_col, state.cursor_col + count - 1);
}

void insertLines(int32_t count_param)
{
    clearWrapPending();
    if (state.rows == 0 || state.cols == 0 || !cursorInScrollRegion()) {
        return;
    }

    const uint16_t available = state.scroll_bottom - state.cursor_row + 1;
    const uint16_t count = static_cast<uint16_t>(minInt(maxInt(1, count_param), available));
    for (int32_t row = state.scroll_bottom; row >= static_cast<int32_t>(state.cursor_row + count); --row) {
        for (uint16_t col = 0; col < state.cols; ++col) {
            cells[row][col] = cells[row - count][col];
        }
    }
    for (uint16_t row = state.cursor_row; row < state.cursor_row + count; ++row) {
        for (uint16_t col = 0; col < state.cols; ++col) {
            cells[row][col] = blankCell();
        }
    }
    renderRows(state.cursor_row, state.scroll_bottom);
}

void deleteLines(int32_t count_param)
{
    clearWrapPending();
    if (state.rows == 0 || state.cols == 0 || !cursorInScrollRegion()) {
        return;
    }

    const uint16_t available = state.scroll_bottom - state.cursor_row + 1;
    const uint16_t count = static_cast<uint16_t>(minInt(maxInt(1, count_param), available));
    for (uint16_t row = state.cursor_row; row + count <= state.scroll_bottom; ++row) {
        for (uint16_t col = 0; col < state.cols; ++col) {
            cells[row][col] = cells[row + count][col];
        }
    }
    for (uint16_t row = state.scroll_bottom - count + 1; row <= state.scroll_bottom; ++row) {
        for (uint16_t col = 0; col < state.cols; ++col) {
            cells[row][col] = blankCell();
        }
        if (row == UINT16_MAX) {
            break;
        }
    }
    renderRows(state.cursor_row, state.scroll_bottom);
}

void eraseInLine(int32_t mode)
{
    clearWrapPending();

    if (state.rows == 0 || state.cols == 0) {
        return;
    }

    uint16_t start_col = state.cursor_col;
    uint16_t end_col = state.cursor_col;
    switch (mode) {
    case 1:
        start_col = 0;
        break;
    case 2:
        start_col = 0;
        end_col = state.cols - 1;
        break;
    case 0:
    default:
        end_col = state.cols - 1;
        break;
    }

    for (uint16_t col = start_col; col <= end_col; ++col) {
        clearCell(state.cursor_row, col, false);
        if (col == UINT16_MAX) {
            break;
        }
    }
    renderCellsInRow(state.cursor_row, start_col, end_col);
}

void eraseInDisplay(int32_t mode)
{
    clearWrapPending();

    if (state.rows == 0 || state.cols == 0) {
        return;
    }

    if (mode == 2 || mode == 3) {
        clearAllCells(true);
        return;
    }

    if (mode == 1) {
        for (uint16_t row = 0; row <= state.cursor_row; ++row) {
            const uint16_t end_col = row == state.cursor_row ? state.cursor_col : state.cols - 1;
            for (uint16_t col = 0; col <= end_col; ++col) {
                clearCell(row, col, false);
                if (col == UINT16_MAX) {
                    break;
                }
            }
            renderCellsInRow(row, 0, end_col);
        }
        return;
    }

    for (uint16_t row = state.cursor_row; row < state.rows; ++row) {
        const uint16_t start_col = row == state.cursor_row ? state.cursor_col : 0;
        for (uint16_t col = start_col; col < state.cols; ++col) {
            clearCell(row, col, false);
        }
        renderCellsInRow(row, start_col, state.cols - 1);
    }
}

uint16_t xterm256Color(uint8_t index)
{
    if (index < 16) {
        return kAnsiColorTable[index];
    }
    if (index >= 232) {
        const uint8_t level = static_cast<uint8_t>(8 + (index - 232) * 10);
        return rgb565(level, level, level);
    }

    const uint8_t color = index - 16;
    const uint8_t red = color / 36;
    const uint8_t green = (color / 6) % 6;
    const uint8_t blue = color % 6;
    const uint8_t levels[6] = {0, 95, 135, 175, 215, 255};
    return rgb565(levels[red], levels[green], levels[blue]);
}

void resetGraphicRendition()
{
    state.current_fg = state.default_fg;
    state.current_bg = state.default_bg;
    state.current_attrs = 0;
}

void resetCharacterSets()
{
    state.g0_charset = CharacterSet::Ascii;
    state.g1_charset = CharacterSet::Ascii;
    state.use_g1 = false;
    state.designate_target_slot = CharacterSetSlot::G0;
}

void saveRuntimeSnapshot(RuntimeSnapshot& snapshot)
{
    snapshot.cursor_col = state.cursor_col;
    snapshot.cursor_row = state.cursor_row;
    snapshot.saved_col = state.saved_col;
    snapshot.saved_row = state.saved_row;
    snapshot.scroll_top = state.scroll_top;
    snapshot.scroll_bottom = state.scroll_bottom;
    snapshot.current_fg = state.current_fg;
    snapshot.current_bg = state.current_bg;
    snapshot.current_attrs = state.current_attrs;
    snapshot.saved_fg = state.saved_fg;
    snapshot.saved_bg = state.saved_bg;
    snapshot.saved_attrs = state.saved_attrs;
    snapshot.g0_charset = state.g0_charset;
    snapshot.g1_charset = state.g1_charset;
    snapshot.saved_g0_charset = state.saved_g0_charset;
    snapshot.saved_g1_charset = state.saved_g1_charset;
    snapshot.use_g1 = state.use_g1;
    snapshot.saved_use_g1 = state.saved_use_g1;
    snapshot.wrap_enabled = state.wrap_enabled;
    snapshot.wrap_pending = state.wrap_pending;
    snapshot.origin_mode = state.origin_mode;
    snapshot.cursor_visible = state.cursor_visible;
    for (uint16_t col = 0; col < kMaxColumns; ++col) {
        snapshot.tab_stops[col] = state.tab_stops[col];
    }
}

void restoreRuntimeSnapshot(const RuntimeSnapshot& snapshot)
{
    state.cursor_col = snapshot.cursor_col;
    state.cursor_row = snapshot.cursor_row;
    state.saved_col = snapshot.saved_col;
    state.saved_row = snapshot.saved_row;
    state.scroll_top = snapshot.scroll_top;
    state.scroll_bottom = snapshot.scroll_bottom;
    state.current_fg = snapshot.current_fg;
    state.current_bg = snapshot.current_bg;
    state.current_attrs = snapshot.current_attrs;
    state.saved_fg = snapshot.saved_fg;
    state.saved_bg = snapshot.saved_bg;
    state.saved_attrs = snapshot.saved_attrs;
    state.g0_charset = snapshot.g0_charset;
    state.g1_charset = snapshot.g1_charset;
    state.saved_g0_charset = snapshot.saved_g0_charset;
    state.saved_g1_charset = snapshot.saved_g1_charset;
    state.use_g1 = snapshot.use_g1;
    state.saved_use_g1 = snapshot.saved_use_g1;
    state.wrap_enabled = snapshot.wrap_enabled;
    state.wrap_pending = snapshot.wrap_pending;
    state.origin_mode = snapshot.origin_mode;
    state.cursor_visible = snapshot.cursor_visible;
    for (uint16_t col = 0; col < kMaxColumns; ++col) {
        state.tab_stops[col] = snapshot.tab_stops[col];
    }
    setCursor(state.cursor_row, state.cursor_col);
}

void useAlternateScreen(bool enabled, bool save_restore_runtime)
{
    if (enabled) {
        if (state.using_alternate_screen) {
            return;
        }
        if (save_restore_runtime) {
            saveRuntimeSnapshot(state.primary_runtime_snapshot);
        }
        cells = alternate_cells;
        state.using_alternate_screen = true;
        state.origin_mode = false;
        state.wrap_pending = false;
        resetScrollRegion();
        setCursor(0, 0);
        clearAllCells(true);
        return;
    }

    if (!state.using_alternate_screen) {
        return;
    }
    cells = primary_cells;
    state.using_alternate_screen = false;
    if (save_restore_runtime) {
        restoreRuntimeSnapshot(state.primary_runtime_snapshot);
    } else {
        state.origin_mode = false;
        state.wrap_pending = false;
        resetScrollRegion();
        setCursor(0, 0);
    }
    renderRows(0, state.rows == 0 ? 0 : static_cast<uint16_t>(state.rows - 1));
}

int32_t csiParam(uint8_t index, int32_t default_value)
{
    if (index >= state.csi_param_count || state.csi_params[index] == kMissingParam) {
        return default_value;
    }
    return state.csi_params[index];
}

void applyGraphicRendition()
{
    if (state.csi_param_count == 0) {
        resetGraphicRendition();
        return;
    }

    for (uint8_t i = 0; i < state.csi_param_count; ++i) {
        const int32_t code = state.csi_params[i] == kMissingParam ? 0 : state.csi_params[i];
        if (code == 0) {
            resetGraphicRendition();
        } else if (code == 1) {
            state.current_attrs |= kAttrBold;
            state.current_attrs &= ~kAttrDim;
        } else if (code == 2) {
            state.current_attrs |= kAttrDim;
            state.current_attrs &= ~kAttrBold;
        } else if (code == 4) {
            state.current_attrs |= kAttrUnderline;
        } else if (code == 7) {
            state.current_attrs |= kAttrInverse;
        } else if (code == 8) {
            state.current_attrs |= kAttrHidden;
        } else if (code == 21 || code == 22) {
            state.current_attrs &= static_cast<uint8_t>(~(kAttrBold | kAttrDim));
        } else if (code == 24) {
            state.current_attrs &= ~kAttrUnderline;
        } else if (code == 27) {
            state.current_attrs &= ~kAttrInverse;
        } else if (code == 28) {
            state.current_attrs &= ~kAttrHidden;
        } else if (code == 39) {
            state.current_fg = state.default_fg;
        } else if (code == 49) {
            state.current_bg = state.default_bg;
        } else if (code >= 30 && code <= 37) {
            const uint8_t bright = (state.current_attrs & kAttrBold) != 0 ? 8 : 0;
            state.current_fg = kAnsiColorTable[bright + code - 30];
        } else if (code >= 40 && code <= 47) {
            state.current_bg = kAnsiColorTable[code - 40];
        } else if (code >= 90 && code <= 97) {
            state.current_fg = kAnsiColorTable[8 + code - 90];
        } else if (code >= 100 && code <= 107) {
            state.current_bg = kAnsiColorTable[8 + code - 100];
        } else if ((code == 38 || code == 48) && i + 2 < state.csi_param_count) {
            const bool foreground = code == 38;
            const int32_t color_mode = state.csi_params[i + 1];
            if (color_mode == 5) {
                const int32_t color_index = state.csi_params[i + 2];
                if (color_index >= 0 && color_index <= 255) {
                    const uint16_t color = xterm256Color(static_cast<uint8_t>(color_index));
                    if (foreground) {
                        state.current_fg = color;
                    } else {
                        state.current_bg = color;
                    }
                }
                i = static_cast<uint8_t>(i + 2);
            } else if (color_mode == 2 && i + 4 < state.csi_param_count) {
                const int32_t red = csiParam(i + 2, 0);
                const int32_t green = csiParam(i + 3, 0);
                const int32_t blue = csiParam(i + 4, 0);
                const uint16_t color = rgb565(
                    static_cast<uint8_t>(minInt(255, maxInt(0, red))),
                    static_cast<uint8_t>(minInt(255, maxInt(0, green))),
                    static_cast<uint8_t>(minInt(255, maxInt(0, blue))));
                if (foreground) {
                    state.current_fg = color;
                } else {
                    state.current_bg = color;
                }
                i = static_cast<uint8_t>(i + 4);
            }
        }
    }
}

void executePrivateMode(bool set_mode)
{
    for (uint8_t i = 0; i < state.csi_param_count; ++i) {
        const int32_t mode = csiParam(i, 0);
        switch (mode) {
        case 1:
            state.application_cursor_mode = set_mode;
            break;
        case 6:
            setOriginMode(set_mode);
            break;
        case 7:
            state.wrap_enabled = set_mode;
            if (!set_mode) {
                clearWrapPending();
            }
            break;
        case 25:
            state.cursor_visible = set_mode;
            break;
        case 47:
        case 1047:
            useAlternateScreen(set_mode, false);
            break;
        case 1048:
            if (set_mode) {
                saveCursor();
            } else {
                restoreCursor();
            }
            break;
        case 1049:
            useAlternateScreen(set_mode, true);
            break;
        case 66:
            state.application_keypad_mode = set_mode;
            break;
        case 1000:
        case 1002:
        case 1003:
        case 1004:
        case 1005:
        case 1006:
        case 1015:
            state.mouse_tracking_mode = set_mode;
            break;
        case 2004:
            state.bracketed_paste_mode = set_mode;
            break;
        default:
            break;
        }
    }
}

void reportCursorPosition()
{
    char response[24];
    snprintf(
        response,
        sizeof(response),
        "\x1b[%u;%uR",
        static_cast<unsigned>(state.cursor_row + 1),
        static_cast<unsigned>(state.cursor_col + 1));
    sendResponse(response);
}

void reportDecCursorPosition()
{
    char response[24];
    snprintf(
        response,
        sizeof(response),
        "\x1b[?%u;%uR",
        static_cast<unsigned>(state.cursor_row + 1),
        static_cast<unsigned>(state.cursor_col + 1));
    sendResponse(response);
}

void executeDeviceStatusReport()
{
    const int32_t report = csiParam(0, 0);
    if (report == 5) {
        sendResponse("\x1b[0n");
    } else if (report == 6) {
        reportCursorPosition();
    }
}

void executePrivateDeviceStatusReport()
{
    const int32_t report = csiParam(0, 0);
    if (report == 6) {
        reportDecCursorPosition();
    }
}

void executeCsi(uint8_t final_byte)
{
    const int32_t n = maxInt(1, csiParam(0, 1));

    if (state.csi_private_marker == '?' && (final_byte == 'h' || final_byte == 'l')) {
        executePrivateMode(final_byte == 'h');
        return;
    }

    switch (final_byte) {
    case '@':
        insertChars(n);
        break;
    case 'A':
        moveCursorRelative(-n, 0);
        break;
    case 'B':
        moveCursorRelative(n, 0);
        break;
    case 'C':
        moveCursorRelative(0, n);
        break;
    case 'D':
        moveCursorRelative(0, -n);
        break;
    case 'E':
        moveCursorRelative(n, 0);
        carriageReturn();
        break;
    case 'F':
        moveCursorRelative(-n, 0);
        carriageReturn();
        break;
    case 'G':
        setCursor(state.cursor_row, clampU16(csiParam(0, 1) - 1, state.cols));
        break;
    case 'H':
    case 'f':
        setCursorFromCsi(csiParam(0, 1), csiParam(1, 1));
        break;
    case 'I':
        cursorForwardTab(n);
        break;
    case 'J':
        eraseInDisplay(csiParam(0, 0));
        break;
    case 'K':
        eraseInLine(csiParam(0, 0));
        break;
    case 'L':
        insertLines(n);
        break;
    case 'M':
        deleteLines(n);
        break;
    case 'P':
        deleteChars(n);
        break;
    case 'S':
        for (int32_t i = 0; i < minInt(n, state.scroll_bottom - state.scroll_top + 1); ++i) {
            scrollUp();
        }
        break;
    case 'T':
        for (int32_t i = 0; i < minInt(n, state.scroll_bottom - state.scroll_top + 1); ++i) {
            scrollDown();
        }
        break;
    case 'X':
        eraseChars(n);
        break;
    case 'Z':
        cursorBackwardTab(n);
        break;
    case 'a':
        moveCursorRelative(0, n);
        break;
    case 'c':
        if (state.csi_private_marker == 0) {
            sendResponse("\x1b[?6c");
        } else if (state.csi_private_marker == '>') {
            sendResponse("\x1b[>0;0;0c");
        }
        break;
    case 'd':
        setCursorFromCsi(csiParam(0, 1), state.cursor_col + 1);
        break;
    case 'e':
        moveCursorRelative(n, 0);
        break;
    case 'g':
        if (csiParam(0, 0) == 0) {
            clearCurrentTabStop();
        } else if (csiParam(0, 0) == 3) {
            clearAllTabStops();
        }
        break;
    case 'm':
        applyGraphicRendition();
        break;
    case 'n':
        if (state.csi_private_marker == 0) {
            executeDeviceStatusReport();
        } else if (state.csi_private_marker == '?') {
            executePrivateDeviceStatusReport();
        }
        break;
    case 'r':
        setScrollRegion(csiParam(0, 1), csiParam(1, state.rows));
        break;
    case 's':
        saveCursor();
        break;
    case 'u':
        restoreCursor();
        break;
    default:
        break;
    }
}

void resetCsi()
{
    for (uint8_t i = 0; i < kMaxCsiParams; ++i) {
        state.csi_params[i] = kMissingParam;
    }
    state.csi_param_count = 1;
    state.csi_param_index = 0;
    state.csi_private_marker = 0;
}

void collectCsi(uint8_t byte)
{
    if (byte >= '0' && byte <= '9') {
        int32_t& param = state.csi_params[state.csi_param_index];
        if (param == kMissingParam) {
            param = 0;
        }
        param = minInt(9999, param * 10 + byte - '0');
        return;
    }

    if (byte == ';') {
        if (state.csi_param_index + 1 < kMaxCsiParams) {
            ++state.csi_param_index;
            state.csi_param_count = state.csi_param_index + 1;
        }
        return;
    }

    if (state.csi_param_index == 0
        && state.csi_params[0] == kMissingParam
        && (byte == '?' || byte == '>' || byte == '=' || byte == '<')) {
        state.csi_private_marker = static_cast<char>(byte);
        return;
    }

    if (byte >= 0x40 && byte <= 0x7e) {
        executeCsi(byte);
        state.parser_state = ParserState::Ground;
    }
}

void resetParser()
{
    state.parser_state = ParserState::Ground;
    state.utf8_length = 0;
    state.utf8_expected = 0;
    resetCsi();
}

void fullReset()
{
    cells = primary_cells;
    state.using_alternate_screen = false;
    resetParser();
    state.wrap_enabled = true;
    state.wrap_pending = false;
    state.origin_mode = false;
    state.application_cursor_mode = false;
    state.application_keypad_mode = false;
    state.bracketed_paste_mode = false;
    state.mouse_tracking_mode = false;
    state.cursor_visible = true;
    state.cursor_drawn = false;
    state.saved_col = 0;
    state.saved_row = 0;
    state.saved_fg = state.default_fg;
    state.saved_bg = state.default_bg;
    state.saved_attrs = 0;
    state.saved_g0_charset = CharacterSet::Ascii;
    state.saved_g1_charset = CharacterSet::Ascii;
    state.saved_use_g1 = false;
    resetScrollRegion();
    resetTabStops();
    setCursor(0, 0);
    resetGraphicRendition();
    resetCharacterSets();
    clearAllCells(true);
    const Cell blank = blankCell();
    for (uint16_t row = 0; row < kMaxRows; ++row) {
        for (uint16_t col = 0; col < kMaxColumns; ++col) {
            alternate_cells[row][col] = blank;
        }
    }
}

void stringTerminator()
{
    state.parser_state = ParserState::Ground;
}

void index()
{
    lineFeed();
}

void nextLine()
{
    lineFeed();
    carriageReturn();
}

void processGround(uint8_t byte);

void startUtf8(uint8_t byte)
{
    state.utf8_buffer[0] = static_cast<char>(byte);
    state.utf8_buffer[1] = '\0';
    state.utf8_length = 1;
    if (byte >= 0xc2 && byte <= 0xdf) {
        state.utf8_expected = 2;
    } else if (byte >= 0xe0 && byte <= 0xef) {
        state.utf8_expected = 3;
    } else if (byte >= 0xf0 && byte <= 0xf4) {
        state.utf8_expected = 4;
    } else {
        putReplacementCharacter();
        return;
    }
    state.parser_state = ParserState::Utf8;
}

void processUtf8(uint8_t byte)
{
    if ((byte & 0xc0) != 0x80 || state.utf8_length == 0 || state.utf8_expected == 0) {
        putReplacementCharacter();
        state.utf8_length = 0;
        state.utf8_expected = 0;
        state.parser_state = ParserState::Ground;
        processGround(byte);
        return;
    }

    if (state.utf8_length < 4) {
        state.utf8_buffer[state.utf8_length] = static_cast<char>(byte);
    }
    ++state.utf8_length;
    if (state.utf8_length >= state.utf8_expected) {
        state.utf8_buffer[minInt(state.utf8_expected, 4)] = '\0';
        putTextCell(
            state.utf8_buffer,
            minInt(state.utf8_expected, 4),
            '?',
            CharacterSet::Ascii);
        state.utf8_length = 0;
        state.utf8_expected = 0;
        state.parser_state = ParserState::Ground;
    }
}

void processGround(uint8_t byte)
{
    switch (byte) {
    case 0x00:
    case 0x07:
    case 0x7f:
        break;
    case 0x08:
        backspace();
        break;
    case 0x09:
        horizontalTab();
        break;
    case 0x0a:
    case 0x0b:
        lineFeed();
        break;
    case 0x0c:
        clearAllCells(true);
        setCursor(0, 0);
        break;
    case 0x0d:
        carriageReturn();
        break;
    case 0x0e:
        state.use_g1 = true;
        break;
    case 0x0f:
        state.use_g1 = false;
        break;
    case 0x18:
    case 0x1a:
        resetParser();
        break;
    case 0x1b:
        state.parser_state = ParserState::Escape;
        break;
    case 0x84:
        index();
        break;
    case 0x85:
        nextLine();
        break;
    case 0x88:
        break;
    case 0x8d:
        reverseIndex();
        break;
    case 0x90:
    case 0x98:
    case 0x9e:
    case 0x9f:
        state.string_return = StringParserReturn::Dcs;
        state.parser_state = ParserState::DcsString;
        break;
    case 0x9b:
        resetCsi();
        state.parser_state = ParserState::Csi;
        break;
    case 0x9c:
        stringTerminator();
        break;
    case 0x9d:
        state.string_return = StringParserReturn::Osc;
        state.parser_state = ParserState::OscString;
        break;
    default:
        if (byte >= 0xc2 && byte <= 0xf4) {
            startUtf8(byte);
        } else if (byte >= 0x20 && byte < 0x80) {
            putPrintable(byte);
        } else if (byte >= 0xa0) {
            putReplacementCharacter();
        }
        break;
    }
}

void processEscape(uint8_t byte)
{
    switch (byte) {
    case '[':
        resetCsi();
        state.parser_state = ParserState::Csi;
        break;
    case ']':
        state.string_return = StringParserReturn::Osc;
        state.parser_state = ParserState::OscString;
        break;
    case 'P':
    case '_':
    case '^':
        state.string_return = StringParserReturn::Dcs;
        state.parser_state = ParserState::DcsString;
        break;
    case 'c':
        fullReset();
        state.parser_state = ParserState::Ground;
        break;
    case 'D':
        index();
        state.parser_state = ParserState::Ground;
        break;
    case 'E':
        nextLine();
        state.parser_state = ParserState::Ground;
        break;
    case 'Z':
        sendResponse("\x1b[?6c");
        state.parser_state = ParserState::Ground;
        break;
    case '=':
        state.application_keypad_mode = true;
        state.parser_state = ParserState::Ground;
        break;
    case '>':
        state.application_keypad_mode = false;
        state.parser_state = ParserState::Ground;
        break;
    case 'M':
        reverseIndex();
        state.parser_state = ParserState::Ground;
        break;
    case 'H':
        setCurrentTabStop();
        state.parser_state = ParserState::Ground;
        break;
    case '7':
        saveCursor();
        state.parser_state = ParserState::Ground;
        break;
    case '8':
        restoreCursor();
        state.parser_state = ParserState::Ground;
        break;
    case '(':
        state.designate_target_slot = CharacterSetSlot::G0;
        state.parser_state = ParserState::CharsetDesignate;
        break;
    case ')':
        state.designate_target_slot = CharacterSetSlot::G1;
        state.parser_state = ParserState::CharsetDesignate;
        break;
    case '*':
    case '+':
    case '-':
    case '.':
    case '/':
        state.parser_state = ParserState::EscapeIgnoreOne;
        break;
    default:
        state.parser_state = ParserState::Ground;
        break;
    }
}

void processCharsetDesignate(uint8_t byte)
{
    CharacterSet selected = CharacterSet::Ascii;
    if (byte == '0') {
        selected = CharacterSet::DecSpecialGraphics;
    }

    if (state.designate_target_slot == CharacterSetSlot::G0) {
        state.g0_charset = selected;
    } else {
        state.g1_charset = selected;
    }
    state.parser_state = ParserState::Ground;
}

void processString(uint8_t byte, ParserState string_state)
{
    if (byte == 0x07) {
        state.parser_state = ParserState::Ground;
        return;
    }
    if (byte == 0x9c) {
        stringTerminator();
        return;
    }
    if (byte == 0x1b) {
        state.string_return = string_state == ParserState::OscString
            ? StringParserReturn::Osc
            : StringParserReturn::Dcs;
        state.parser_state = ParserState::StringEscape;
    }
}

void processByte(uint8_t byte)
{
    if (byte == 0x18 || byte == 0x1a) {
        resetParser();
        return;
    }
    if (byte == 0x1b && state.parser_state != ParserState::Ground) {
        state.parser_state = ParserState::Escape;
        return;
    }

    switch (state.parser_state) {
    case ParserState::Ground:
        processGround(byte);
        break;
    case ParserState::Escape:
        processEscape(byte);
        break;
    case ParserState::EscapeIgnoreOne:
        state.parser_state = ParserState::Ground;
        break;
    case ParserState::CharsetDesignate:
        processCharsetDesignate(byte);
        break;
    case ParserState::Csi:
        collectCsi(byte);
        break;
    case ParserState::Utf8:
        processUtf8(byte);
        break;
    case ParserState::OscString:
        processString(byte, ParserState::OscString);
        break;
    case ParserState::DcsString:
        processString(byte, ParserState::DcsString);
        break;
    case ParserState::StringEscape:
        if (byte == '\\') {
            state.parser_state = ParserState::Ground;
        } else {
            state.parser_state = state.string_return == StringParserReturn::Osc
                ? ParserState::OscString
                : ParserState::DcsString;
        }
        break;
    }
}
} // namespace

void begin(const TerminalConfig& config)
{
    state.config = config;
    state.default_fg = config.theme != nullptr ? config.theme->terminal_text : rgb565(0, 255, 0);
    state.default_bg = config.theme != nullptr ? config.theme->screen_background : rgb565(0, 0, 0);
    state.current_fg = state.default_fg;
    state.current_bg = state.default_bg;
    applyDisplayTextStyle(state.default_fg, state.default_bg);
    state.cell_width = maxInt(TERMINAL_CELL_WIDTH, M5.Display.textWidth("M"));
    state.cell_height = maxInt(TERMINAL_CELL_HEIGHT, M5.Display.fontHeight());
    const int32_t columnLimit =
        config.max_columns == 0 ? kMaxColumns : minInt(kMaxColumns, config.max_columns);
    const int32_t rowLimit =
        config.max_rows == 0 ? kMaxRows : minInt(kMaxRows, config.max_rows);
    state.cols = static_cast<uint16_t>(
        minInt(columnLimit, maxInt(1, config.width / state.cell_width)));
    state.rows = static_cast<uint16_t>(
        minInt(rowLimit, maxInt(1, config.height / state.cell_height)));
    state.initialized = true;
    setTerminalScrollRect();
    fullReset();
    drawCursor();
}

void reset()
{
    if (!state.initialized) {
        return;
    }

    eraseCursor();
    fullReset();
    drawCursor();
}

void redraw()
{
    if (!state.initialized) {
        return;
    }

    state.cursor_drawn = false;
    renderRows(0, state.rows == 0 ? 0 : static_cast<uint16_t>(state.rows - 1));
    drawCursor();
}

void writeByte(uint8_t byte)
{
    if (!state.initialized) {
        return;
    }

    eraseCursor();
    processByte(byte);
    drawCursor();
}

void writeBytes(const uint8_t *data, size_t length)
{
    if (data == nullptr) {
        return;
    }

    for (size_t i = 0; i < length; ++i) {
        writeByte(data[i]);
    }
}

uint16_t columns()
{
    return state.cols;
}

uint16_t rows()
{
    return state.rows;
}

bool applicationCursorMode()
{
    return state.application_cursor_mode;
}

bool applicationKeypadMode()
{
    return state.application_keypad_mode;
}

bool bracketedPasteMode()
{
    return state.bracketed_paste_mode;
}

} // namespace terminal
