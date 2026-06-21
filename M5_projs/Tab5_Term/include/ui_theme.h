#pragma once

#include <stdint.h>

namespace ui {

enum class TerminalThemeId : uint8_t {
    ClassicGreen = 0,
};

struct TerminalTheme {
    uint16_t screen_background;
    uint16_t terminal_text;

    uint16_t status_background;
    uint16_t status_text;

    uint16_t battery_panel_background;
    uint16_t battery_text;
    uint16_t battery_outline;
    uint16_t battery_empty_track;
    uint16_t battery_high;
    uint16_t battery_medium;
    uint16_t battery_low;
    uint16_t battery_charging;
    uint16_t battery_lightning;

    uint16_t menu_background;
    uint16_t menu_border;
    uint16_t menu_text;
    uint16_t menu_pressed_background;
    uint16_t menu_separator;
};

const TerminalTheme& terminalTheme(
    TerminalThemeId id = TerminalThemeId::ClassicGreen);

} // namespace ui
