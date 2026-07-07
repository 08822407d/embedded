#include "ui_theme.h"

namespace ui {
namespace {
constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return static_cast<uint16_t>(
        ((red & 0xF8) << 8)
        | ((green & 0xFC) << 3)
        | (blue >> 3));
}

constexpr TerminalTheme kClassicGreenTheme = {
    .screen_background = rgb565(0, 0, 0),
    .terminal_text = rgb565(0, 255, 0),

    .status_background = rgb565(0, 100, 0),
    .status_text = rgb565(255, 255, 255),

    .battery_panel_background = rgb565(0, 0, 0),
    .battery_text = rgb565(255, 255, 255),
    .battery_outline = rgb565(245, 245, 245),
    .battery_empty_track = rgb565(18, 18, 18),
    .battery_high = rgb565(52, 199, 89),
    .battery_medium = rgb565(255, 204, 0),
    .battery_low = rgb565(255, 59, 48),
    .battery_charging = rgb565(52, 199, 89),
    .battery_lightning = rgb565(255, 226, 120),

    .menu_background = rgb565(18, 20, 22),
    .menu_border = rgb565(96, 116, 104),
    .menu_text = rgb565(248, 248, 248),
    .menu_pressed_background = rgb565(0, 86, 62),
    .menu_separator = rgb565(48, 56, 52),
};
} // namespace

const TerminalTheme& terminalTheme(TerminalThemeId id)
{
    switch (id) {
    case TerminalThemeId::ClassicGreen:
    default:
        return kClassicGreenTheme;
    }
}

} // namespace ui
