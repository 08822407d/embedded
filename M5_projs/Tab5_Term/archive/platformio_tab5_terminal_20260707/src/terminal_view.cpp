#include "terminal_view.h"

#include <M5Unified.h>

#include "app_config.h"

namespace ui {
namespace {

TerminalViewGeometry active_geometry = {};
bool terminal_view_initialized = false;

int32_t minInt(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

int32_t maxInt(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

TerminalViewRect normalizedRect(const TerminalViewRect& rect)
{
    return {
        rect.x,
        rect.y,
        maxInt(0, rect.width),
        maxInt(0, rect.height),
    };
}

int32_t preferredTerminalWidth(uint16_t columns)
{
    return columns == 0
        ? 0
        : static_cast<int32_t>(columns) * TERMINAL_CELL_WIDTH;
}

int32_t preferredTerminalHeight(uint16_t rows)
{
    return rows == 0
        ? 0
        : static_cast<int32_t>(rows) * TERMINAL_CELL_HEIGHT;
}

TerminalViewRect centeredTerminalContent(
    const TerminalViewRect& bounds,
    uint16_t columns,
    uint16_t rows)
{
    const int32_t preferred_width = preferredTerminalWidth(columns);
    const int32_t preferred_height = preferredTerminalHeight(rows);
    const int32_t content_width = maxInt(
        1,
        preferred_width > 0 ? minInt(bounds.width, preferred_width) : bounds.width);
    const int32_t content_height = maxInt(
        1,
        preferred_height > 0 ? minInt(bounds.height, preferred_height) : bounds.height);

    return {
        bounds.x + maxInt(0, (bounds.width - content_width) / 2),
        bounds.y + maxInt(0, (bounds.height - content_height) / 2),
        content_width,
        content_height,
    };
}

void clearBounds(const TerminalViewRect& bounds, const TerminalTheme *theme)
{
    if (bounds.width <= 0 || bounds.height <= 0 || theme == nullptr) {
        return;
    }

    M5.Display.fillRect(
        bounds.x,
        bounds.y,
        bounds.width,
        bounds.height,
        theme->screen_background);
}

} // namespace

void beginTerminalView(const TerminalViewConfig& config)
{
    const TerminalViewRect bounds = normalizedRect(config.bounds);
    const TerminalViewRect content =
        centeredTerminalContent(bounds, config.columns, config.rows);

    clearBounds(bounds, config.theme);
    terminal::begin({
        .x = content.x,
        .y = content.y,
        .width = content.width,
        .height = content.height,
        .max_columns = config.columns,
        .max_rows = config.rows,
        .text_size = config.text_size,
        .theme = config.theme,
        .response_writer = config.response_writer,
    });

    terminal::TerminalGeometry core_geometry = {};
    if (terminal::getGeometry(&core_geometry)) {
        active_geometry = {
            .bounds = bounds,
            .content = {
                core_geometry.x,
                core_geometry.y,
                core_geometry.rendered_width,
                core_geometry.rendered_height,
            },
            .cell_width = core_geometry.cell_width,
            .cell_height = core_geometry.cell_height,
            .columns = core_geometry.columns,
            .rows = core_geometry.rows,
        };
    } else {
        active_geometry = {
            .bounds = bounds,
            .content = content,
            .cell_width = TERMINAL_CELL_WIDTH,
            .cell_height = TERMINAL_CELL_HEIGHT,
            .columns = 0,
            .rows = 0,
        };
    }
    terminal_view_initialized = true;
}

void redrawTerminalView()
{
    terminal::redraw();
}

bool getTerminalViewGeometry(TerminalViewGeometry *geometry)
{
    if (!terminal_view_initialized || geometry == nullptr) {
        return false;
    }

    *geometry = active_geometry;
    return true;
}

} // namespace ui
