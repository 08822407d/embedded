#include "status_bar.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "terminal_view.h"
#include "temperature_monitor.h"

namespace ui {
namespace {
constexpr int32_t kStatusTextX = 4;
constexpr int32_t kStatusCenterY = STATUS_BAR_HEIGHT / 2;
// Tab5 CHG_STAT was observed to miss repeated cable transitions on the tested
// hardware. The lightning icon uses a debounced INA226 charge-current heuristic.
constexpr uint32_t kBatteryLevelUpdateIntervalMs = 10000;
constexpr uint32_t kChargingUpdateIntervalMs = 500;
constexpr uint32_t kBatteryLevelChargeSettleHoldMs = 30000;
constexpr uint32_t kBatteryLevelUnplugSettleHoldMs = 30000;
constexpr uint32_t kBatteryLevelUnplugSmoothingWindowMs = 180000;
constexpr int32_t kBatteryLevelMaxChargeRisePerUpdate = 1;
constexpr int32_t kBatteryLevelMaxUnplugDropPerUpdate = 1;
constexpr int32_t kBatteryLevelImmediateDropPercent = 10;
constexpr int32_t kChargingEnterCurrentMa = -600;
constexpr int32_t kChargingExitCurrentMa = -300;
constexpr uint8_t kChargingSamplesToSwitch = 2;
constexpr int32_t kUnknownBatteryLevel = -1;
constexpr int32_t kUninitializedBatteryLevel = -2;
constexpr uint32_t kPowerActionDisplayDelayMs = 150;

const char kPowerOffLabel[] = "Power Off";
const char kRestartLabel[] = "Restart";
const char kPowerOffBusyLabel[] = "Power off...";
const char kRestartBusyLabel[] = "Restart...";

enum class UiWidgetState : uint8_t {
    Default = 0,
    Pressed = 1,
    Disabled = 2,
};

enum class PowerMenuItem : int8_t {
    None = -1,
    PowerOff = 0,
    Restart = 1,
};

enum class TemperatureDisplayUnit : uint8_t {
    Celsius = 0,
    Fahrenheit = 1,
    Kelvin = 2,
};

enum class StatusBarAreaPalette : uint8_t {
    Panel = 0,
};

struct UiScale {
    int32_t numerator;
    int32_t denominator;

    constexpr int32_t px(int32_t value) const
    {
        return denominator == 0 ? value : value * numerator / denominator;
    }

    constexpr float text(float value) const
    {
        return denominator == 0
            ? value
            : value * static_cast<float>(numerator) / static_cast<float>(denominator);
    }
};

struct MenuGeometrySpec {
    int32_t width;
    int32_t item_height;
    int32_t label_left_padding_cells;
    int32_t right_inset;
    int32_t top_gap;
    int32_t border_radius;
    int32_t separator_horizontal_inset;
};

struct MenuTextSpec {
    float size;
};

struct PowerMenuItemSpec {
    PowerMenuItem item;
    const char *label;
    const char *busy_label;
};

struct PowerMenuSpec {
    UiScale scale;
    MenuGeometrySpec geometry;
    MenuTextSpec text;
    const PowerMenuItemSpec *items;
    int32_t item_count;
};

struct PowerMenuItemStyle {
    uint16_t background;
    uint16_t text;
};

constexpr PowerMenuItemSpec kPowerMenuItems[] = {
    {
        .item = PowerMenuItem::PowerOff,
        .label = kPowerOffLabel,
        .busy_label = kPowerOffBusyLabel,
    },
    {
        .item = PowerMenuItem::Restart,
        .label = kRestartLabel,
        .busy_label = kRestartBusyLabel,
    },
};

constexpr PowerMenuSpec kPowerMenuSpec = {
    .scale = {
        .numerator = 3,
        .denominator = 2,
    },
    .geometry = {
        .width = 144,
        .item_height = STATUS_BAR_HEIGHT,
        .label_left_padding_cells = 2,
        .right_inset = 4,
        .top_gap = 2,
        .border_radius = 6,
        .separator_horizontal_inset = 8,
    },
    .text = {
        .size = 1.0f,
    },
    .items = kPowerMenuItems,
    .item_count = static_cast<int32_t>(sizeof(kPowerMenuItems) / sizeof(kPowerMenuItems[0])),
};

constexpr int32_t kPowerMenuWidth =
    kPowerMenuSpec.scale.px(kPowerMenuSpec.geometry.width);
constexpr int32_t kPowerMenuItemHeight =
    kPowerMenuSpec.scale.px(kPowerMenuSpec.geometry.item_height);
constexpr int32_t kPowerMenuItemCount = kPowerMenuSpec.item_count;
constexpr int32_t kPowerMenuBorderRadius =
    kPowerMenuSpec.scale.px(kPowerMenuSpec.geometry.border_radius);
constexpr int32_t kPowerMenuRightInset = kPowerMenuSpec.geometry.right_inset;
constexpr int32_t kPowerMenuTopGap = kPowerMenuSpec.geometry.top_gap;
constexpr int32_t kPowerMenuSeparatorInset =
    kPowerMenuSpec.scale.px(kPowerMenuSpec.geometry.separator_horizontal_inset);
constexpr int32_t kPowerMenuLabelLeftPadding =
    kPowerMenuSpec.geometry.label_left_padding_cells * TERMINAL_CELL_WIDTH;
constexpr float kPowerMenuTextSize =
    kPowerMenuSpec.scale.text(kPowerMenuSpec.text.size);

struct StatusBarAreaConfig {
    int32_t height;
    int32_t trailing_gap;
    float text_size;
    int32_t horizontal_padding_cells;
    StatusBarAreaPalette palette;
};

struct StatusBarAreaColors {
    uint16_t background;
    uint16_t text;
};

constexpr StatusBarAreaConfig kStatusBarAreaDefaults = {
    .height = STATUS_BAR_HEIGHT,
    .trailing_gap = 0,
    .text_size = 2.0f,
    .horizontal_padding_cells = 1,
    .palette = StatusBarAreaPalette::Panel,
};

constexpr StatusBarAreaConfig statusBarAreaConfig(
    float text_size = kStatusBarAreaDefaults.text_size,
    int32_t trailing_gap = kStatusBarAreaDefaults.trailing_gap,
    int32_t horizontal_padding_cells = kStatusBarAreaDefaults.horizontal_padding_cells,
    StatusBarAreaPalette palette = kStatusBarAreaDefaults.palette)
{
    return {
        .height = kStatusBarAreaDefaults.height,
        .trailing_gap = trailing_gap,
        .text_size = text_size,
        .horizontal_padding_cells = horizontal_padding_cells,
        .palette = palette,
    };
}

struct TemperatureAreaLayout {
    StatusBarAreaConfig area;
    int32_t degree_radius;
    int32_t degree_gap;
    int32_t degree_unit_gap;
    int32_t degree_y_offset;
    int32_t icon_gap;
    int32_t icon_width;
    int32_t icon_height;
    int32_t icon_stem_width;
    int32_t icon_bulb_radius;
};

constexpr TemperatureAreaLayout kTemperatureArea = {
    .area = statusBarAreaConfig(),
    .degree_radius = 3,
    .degree_gap = 2,
    .degree_unit_gap = 2,
    .degree_y_offset = -7,
    .icon_gap = 6,
    .icon_width = 16,
    .icon_height = 28,
    .icon_stem_width = 5,
    .icon_bulb_radius = 6,
};

struct BatteryAreaLayout {
    StatusBarAreaConfig area;
    int32_t text_icon_gap;
    int32_t icon_body_width;
    int32_t icon_height;
    int32_t icon_cap_width;
    int32_t icon_cap_height;
    int32_t shell_inset;
    int32_t inner_x_inset;
    int32_t inner_y_inset;
    int32_t body_corner_radius;
    int32_t body_inner_corner_radius;
    int32_t track_corner_radius;
    int32_t cap_corner_radius;
};

constexpr BatteryAreaLayout kBatteryArea = {
    .area = statusBarAreaConfig(),
    .text_icon_gap = 6,
    .icon_body_width = 42,
    .icon_height = 20,
    .icon_cap_width = 4,
    .icon_cap_height = 8,
    .shell_inset = 2,
    .inner_x_inset = 3,
    .inner_y_inset = 3,
    .body_corner_radius = 5,
    .body_inner_corner_radius = 3,
    .track_corner_radius = 3,
    .cap_corner_radius = 2,
};

struct Point {
    int32_t x;
    int32_t y;
};

struct Rect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

struct ChargingStatus {
    bool api_charging;
    bool raw_pin_high;
    bool current_charging;
    int32_t battery_current_ma;
    int32_t battery_voltage_mv;
};

const TerminalTheme *active_theme = nullptr;
uint32_t last_battery_level_update_ms = 0;
uint32_t last_charging_update_ms = 0;
int32_t last_battery_level = kUninitializedBatteryLevel;
bool last_charging = false;
bool last_charging_valid = false;
uint32_t charging_started_ms = 0;
uint32_t charging_stopped_ms = 0;
bool inferred_current_charging = false;
bool inferred_current_charging_valid = false;
bool pending_current_charging = false;
uint8_t pending_current_charging_count = 0;
TemperatureDisplayUnit temperature_unit = TemperatureDisplayUnit::Celsius;
bool temperature_status_drawn = false;
char last_temperature_text[16] = {};
bool temperature_touch_active = false;
uint8_t temperature_touch_id = 0;
bool temperature_touch_inside = false;
Rect last_temperature_area = {};
bool last_temperature_area_valid = false;
Rect last_battery_area = {};
bool last_battery_area_valid = false;
bool power_menu_visible = false;
bool power_menu_touch_active = false;
uint8_t power_menu_touch_id = 0;
PowerMenuItem power_menu_captured_item = PowerMenuItem::None;
PowerMenuItem power_menu_pressed_item = PowerMenuItem::None;
#if ENABLE_POWER_STATUS_DIAGNOSTICS
constexpr size_t kDiagnosticWindowSampleCount = 10;

uint32_t charging_sample_count = 0;
uint32_t charging_change_count = 0;
bool last_diag_api_charging = false;
bool last_diag_raw_pin_high = false;
bool last_diag_current_charging = false;
int32_t diag_current_min_ma = 0;
int32_t diag_current_max_ma = 0;
int32_t diag_current_window_ma[kDiagnosticWindowSampleCount] = {};
int32_t diag_voltage_window_mv[kDiagnosticWindowSampleCount] = {};
size_t diag_window_next_index = 0;
size_t diag_window_sample_count = 0;
int64_t diag_current_window_sum_ma = 0;
int64_t diag_voltage_window_sum_mv = 0;
bool last_diag_valid = false;
#endif

const TerminalTheme& theme()
{
    return active_theme != nullptr ? *active_theme : terminalTheme();
}

void setStatusTextStyle(uint16_t text_color, uint16_t background_color, float text_size)
{
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextScroll(false);
    M5.Display.setTextDatum(m5gfx::textdatum_t::top_left);
    M5.Display.setTextColor(text_color, background_color);
    M5.Display.setTextSize(text_size);
}

void formatBatteryPercent(char *buffer, size_t buffer_size, int32_t battery_level)
{
    if (battery_level < 0) {
        snprintf(buffer, buffer_size, "--%%");
        return;
    }

    snprintf(buffer, buffer_size, "%ld%%", static_cast<long>(battery_level));
}

int32_t readBatteryLevel()
{
    int32_t level = M5.Power.getBatteryLevel();
    if (level < 0) {
        return kUnknownBatteryLevel;
    }
    if (level > 100) {
        return 100;
    }
    return level;
}

int32_t displayedBatteryLevelFromRaw(
    int32_t raw_level,
    int32_t displayed_level,
    bool charging,
    uint32_t now)
{
    if (raw_level < 0 || displayed_level < 0) {
        return raw_level;
    }

    if (raw_level == displayed_level) {
        return raw_level;
    }

    if (raw_level > displayed_level) {
        if (!charging) {
            return raw_level;
        }

        if (charging_started_ms != 0
            && now - charging_started_ms < kBatteryLevelChargeSettleHoldMs) {
            return displayed_level;
        }

        const int32_t capped_level =
            displayed_level + kBatteryLevelMaxChargeRisePerUpdate;
        return raw_level < capped_level ? raw_level : capped_level;
    }

    if (charging || raw_level <= kBatteryLevelImmediateDropPercent) {
        return raw_level;
    }

    if (charging_stopped_ms != 0) {
        const uint32_t elapsed_since_unplug = now - charging_stopped_ms;
        if (elapsed_since_unplug < kBatteryLevelUnplugSettleHoldMs) {
            return displayed_level;
        }
        if (elapsed_since_unplug < kBatteryLevelUnplugSmoothingWindowMs) {
            const int32_t capped_level =
                displayed_level - kBatteryLevelMaxUnplugDropPerUpdate;
            return raw_level > capped_level ? raw_level : capped_level;
        }
    }

    return raw_level;
}

bool updateChargingHeuristic(int32_t battery_current_ma)
{
    if (!inferred_current_charging_valid) {
        inferred_current_charging =
            battery_current_ma < kChargingEnterCurrentMa;
        inferred_current_charging_valid = true;
        pending_current_charging = inferred_current_charging;
        pending_current_charging_count = 0;
        return inferred_current_charging;
    }

    bool target = inferred_current_charging;
    if (inferred_current_charging) {
        if (battery_current_ma > kChargingExitCurrentMa) {
            target = false;
        }
    } else if (battery_current_ma < kChargingEnterCurrentMa) {
        target = true;
    }

    if (target == inferred_current_charging) {
        pending_current_charging = target;
        pending_current_charging_count = 0;
        return inferred_current_charging;
    }

    if (pending_current_charging != target || pending_current_charging_count == 0) {
        pending_current_charging = target;
        pending_current_charging_count = 1;
    } else if (pending_current_charging_count < kChargingSamplesToSwitch) {
        ++pending_current_charging_count;
    }

    if (pending_current_charging_count >= kChargingSamplesToSwitch) {
        inferred_current_charging = target;
        pending_current_charging_count = 0;
    }
    return inferred_current_charging;
}

ChargingStatus readChargingStatus()
{
    const bool api_charging = M5.Power.isCharging() == m5::Power_Class::is_charging;
    const int32_t battery_current_ma = M5.Power.getBatteryCurrent();
    const bool current_charging = updateChargingHeuristic(battery_current_ma);
#if ENABLE_POWER_STATUS_DIAGNOSTICS
    const bool raw_pin_high = M5.getIOExpander(1).digitalRead(6);
    const int32_t battery_voltage_mv = M5.Power.getBatteryVoltage();
#else
    const bool raw_pin_high = api_charging;
    constexpr int32_t battery_voltage_mv = 0;
#endif

    return {
        api_charging,
        raw_pin_high,
        current_charging,
        battery_current_ma,
        battery_voltage_mv,
    };
}

bool intervalElapsed(uint32_t now, uint32_t previous, uint32_t interval)
{
    return now - previous >= interval;
}

uint16_t batteryFillColor(int32_t battery_level, bool charging)
{
    const auto& colors = theme();
    if (battery_level < 0) {
        return colors.battery_empty_track;
    }
    if (charging) {
        return colors.battery_charging;
    }
    if (battery_level <= 20) {
        return colors.battery_low;
    }
    if (battery_level <= 50) {
        return colors.battery_medium;
    }
    return colors.battery_high;
}

int32_t minInt(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

int32_t maxInt(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

const PowerMenuItemSpec *powerMenuItemSpecAt(int32_t index)
{
    if (index < 0 || index >= kPowerMenuSpec.item_count) {
        return nullptr;
    }

    return &kPowerMenuSpec.items[index];
}

const PowerMenuItemSpec *powerMenuItemSpecFor(PowerMenuItem item)
{
    for (int32_t index = 0; index < kPowerMenuSpec.item_count; ++index) {
        const PowerMenuItemSpec *spec = powerMenuItemSpecAt(index);
        if (spec != nullptr && spec->item == item) {
            return spec;
        }
    }

    return nullptr;
}

PowerMenuItemStyle powerMenuItemStyle(
    const TerminalTheme& colors,
    UiWidgetState state)
{
    const uint16_t background = state == UiWidgetState::Pressed
        ? colors.menu_pressed_background
        : colors.menu_background;
    return {
        .background = background,
        .text = colors.menu_text,
    };
}

bool splitTemperatureUnit(
    const char *text,
    char *value_text,
    size_t value_text_size,
    char *unit_text,
    size_t unit_text_size);

void formatTemperatureTextForUnit(
    char *buffer,
    size_t buffer_size,
    const temperature_monitor::TemperatureReading& reading,
    TemperatureDisplayUnit unit);

bool containsPoint(const Rect& rect, int32_t x, int32_t y)
{
    return x >= rect.x
        && y >= rect.y
        && x < rect.x + rect.width
        && y < rect.y + rect.height;
}

bool rectEquals(const Rect& lhs, const Rect& rhs)
{
    return lhs.x == rhs.x
        && lhs.y == rhs.y
        && lhs.width == rhs.width
        && lhs.height == rhs.height;
}

void clearStatusArea(const Rect& area)
{
    if (area.width <= 0 || area.height <= 0) {
        return;
    }

    M5.Display.fillRect(
        area.x,
        area.y,
        area.width,
        area.height,
        theme().status_background);
}

StatusBarAreaColors statusBarAreaColors(
    const TerminalTheme& colors,
    const StatusBarAreaConfig& area)
{
    switch (area.palette) {
    case StatusBarAreaPalette::Panel:
    default:
        return {
            .background = colors.battery_panel_background,
            .text = colors.battery_text,
        };
    }
}

int32_t statusBarAreaCharacterWidth(const StatusBarAreaConfig& area)
{
    constexpr int32_t kFont0CharacterWidth = 6;
    const int32_t width =
        static_cast<int32_t>(kFont0CharacterWidth * area.text_size + 0.5f);
    return maxInt(1, width);
}

int32_t statusBarAreaHorizontalPadding(const StatusBarAreaConfig& area)
{
    return statusBarAreaCharacterWidth(area) * area.horizontal_padding_cells;
}

int32_t statusBarAreaWidthForContent(
    const StatusBarAreaConfig& area,
    int32_t content_width)
{
    return maxInt(0, content_width) + statusBarAreaHorizontalPadding(area) * 2;
}

Rect statusBarAreaRectFromRight(
    int32_t right_x,
    const StatusBarAreaConfig& area,
    int32_t content_width)
{
    const int32_t area_width =
        minInt(statusBarAreaWidthForContent(area, content_width), maxInt(0, right_x));
    return {
        right_x - area_width,
        0,
        area_width,
        area.height,
    };
}

int32_t batteryIconWidth()
{
    return kBatteryArea.icon_body_width + kBatteryArea.icon_cap_width;
}

int32_t defaultBatteryContentWidth()
{
    return statusBarAreaCharacterWidth(kBatteryArea.area) * 4
        + kBatteryArea.text_icon_gap
        + batteryIconWidth();
}

Rect batteryAreaRectForContentWidth(int32_t content_width)
{
    return statusBarAreaRectFromRight(
        M5.Display.width() - kBatteryArea.area.trailing_gap,
        kBatteryArea.area,
        content_width);
}

Rect batteryAreaRect()
{
    if (last_battery_area_valid) {
        return last_battery_area;
    }

    return batteryAreaRectForContentWidth(defaultBatteryContentWidth());
}

int32_t temperatureDegreeWidth()
{
    return kTemperatureArea.degree_radius * 2 + 1;
}

int32_t temperatureTextWidth(const char *text)
{
    char value_text[sizeof(last_temperature_text)] = {};
    char unit_text[2] = {};
    if (splitTemperatureUnit(text, value_text, sizeof(value_text), unit_text, sizeof(unit_text))) {
        return M5.Display.textWidth(value_text)
            + kTemperatureArea.degree_gap
            + temperatureDegreeWidth()
            + kTemperatureArea.degree_unit_gap
            + M5.Display.textWidth(unit_text);
    }

    return M5.Display.textWidth(text);
}

int32_t temperatureIconWidth()
{
    return kTemperatureArea.icon_width;
}

int32_t maximumTemperatureTextWidth(const char *fallback_text)
{
    const temperature_monitor::TemperatureReading reading =
        temperature_monitor::primaryReading();
    int32_t width = temperatureTextWidth(fallback_text);

    constexpr TemperatureDisplayUnit kUnits[] = {
        TemperatureDisplayUnit::Celsius,
        TemperatureDisplayUnit::Fahrenheit,
        TemperatureDisplayUnit::Kelvin,
    };
    for (const TemperatureDisplayUnit unit : kUnits) {
        char text[sizeof(last_temperature_text)] = {};
        formatTemperatureTextForUnit(text, sizeof(text), reading, unit);
        width = maxInt(width, temperatureTextWidth(text));
    }
    return width;
}

int32_t temperatureContentWidthForTextWidth(int32_t text_width)
{
    return maxInt(0, text_width) + kTemperatureArea.icon_gap + temperatureIconWidth();
}

int32_t defaultTemperatureContentWidth()
{
    return temperatureContentWidthForTextWidth(
        statusBarAreaCharacterWidth(kTemperatureArea.area) * 4
        + kTemperatureArea.degree_gap
        + temperatureDegreeWidth()
        + kTemperatureArea.degree_unit_gap);
}

Rect temperatureAreaRectForContentWidth(
    int32_t content_width,
    const Rect& battery_area)
{
    return statusBarAreaRectFromRight(
        battery_area.x - kTemperatureArea.area.trailing_gap,
        kTemperatureArea.area,
        content_width);
}

Rect temperatureAreaRect()
{
    if (last_temperature_area_valid) {
        return last_temperature_area;
    }

    return temperatureAreaRectForContentWidth(
        defaultTemperatureContentWidth(),
        batteryAreaRect());
}

int32_t statusControlsLeftX()
{
    return temperatureAreaRect().x;
}

Rect powerMenuRect()
{
    const int32_t display_width = M5.Display.width();
    const int32_t display_height = M5.Display.height();
    const int32_t right_x = display_width - kPowerMenuRightInset;
    const int32_t width = minInt(kPowerMenuWidth, maxInt(0, right_x));
    const int32_t max_menu_height =
        maxInt(0, display_height - STATUS_BAR_HEIGHT - kPowerMenuTopGap);
    const int32_t height =
        minInt(kPowerMenuItemHeight * kPowerMenuItemCount, max_menu_height);
    return {
        maxInt(0, right_x - width),
        STATUS_BAR_HEIGHT + kPowerMenuTopGap,
        width,
        height,
    };
}

PowerMenuItem powerMenuItemAt(int32_t x, int32_t y)
{
    const Rect menu = powerMenuRect();
    if (!containsPoint(menu, x, y) || menu.height < kPowerMenuItemHeight) {
        return PowerMenuItem::None;
    }

    const int32_t index = (y - menu.y) / kPowerMenuItemHeight;
    const PowerMenuItemSpec *item = powerMenuItemSpecAt(index);
    return item == nullptr ? PowerMenuItem::None : item->item;
}

#if ENABLE_POWER_STATUS_DIAGNOSTICS
struct DiagnosticWindowStats {
    size_t sample_count;
    int32_t current_avg_ma;
    int32_t current_min_ma;
    int32_t current_max_ma;
    int32_t voltage_avg_mv;
};

void resetDiagnosticWindow()
{
    diag_window_next_index = 0;
    diag_window_sample_count = 0;
    diag_current_window_sum_ma = 0;
    diag_voltage_window_sum_mv = 0;
}

void recordDiagnosticWindowSample(const ChargingStatus& status)
{
    if (diag_window_sample_count < kDiagnosticWindowSampleCount) {
        ++diag_window_sample_count;
    } else {
        diag_current_window_sum_ma -= diag_current_window_ma[diag_window_next_index];
        diag_voltage_window_sum_mv -= diag_voltage_window_mv[diag_window_next_index];
    }

    diag_current_window_ma[diag_window_next_index] = status.battery_current_ma;
    diag_voltage_window_mv[diag_window_next_index] = status.battery_voltage_mv;
    diag_current_window_sum_ma += status.battery_current_ma;
    diag_voltage_window_sum_mv += status.battery_voltage_mv;
    diag_window_next_index = (diag_window_next_index + 1) % kDiagnosticWindowSampleCount;
}

DiagnosticWindowStats diagnosticWindowStats()
{
    DiagnosticWindowStats stats = {};
    stats.sample_count = diag_window_sample_count;
    if (diag_window_sample_count == 0) {
        return stats;
    }

    stats.current_min_ma = diag_current_window_ma[0];
    stats.current_max_ma = diag_current_window_ma[0];
    for (size_t i = 1; i < diag_window_sample_count; ++i) {
        stats.current_min_ma = minInt(stats.current_min_ma, diag_current_window_ma[i]);
        stats.current_max_ma = maxInt(stats.current_max_ma, diag_current_window_ma[i]);
    }

    stats.current_avg_ma = static_cast<int32_t>(diag_current_window_sum_ma / static_cast<int64_t>(diag_window_sample_count));
    stats.voltage_avg_mv = static_cast<int32_t>(diag_voltage_window_sum_mv / static_cast<int64_t>(diag_window_sample_count));
    return stats;
}

void recordChargingDiagnostic(const ChargingStatus& status, uint32_t now)
{
    if (last_diag_valid) {
        if (status.api_charging != last_diag_api_charging
            || status.raw_pin_high != last_diag_raw_pin_high
            || status.current_charging != last_diag_current_charging) {
            ++charging_change_count;
        }
        diag_current_min_ma = minInt(diag_current_min_ma, status.battery_current_ma);
        diag_current_max_ma = maxInt(diag_current_max_ma, status.battery_current_ma);
    } else {
        diag_current_min_ma = status.battery_current_ma;
        diag_current_max_ma = status.battery_current_ma;
    }
    recordDiagnosticWindowSample(status);

    (void)now;
    ++charging_sample_count;
    last_diag_api_charging = status.api_charging;
    last_diag_raw_pin_high = status.raw_pin_high;
    last_diag_current_charging = status.current_charging;
    last_diag_valid = true;
}

void drawPowerDiagnostics(const ChargingStatus& status, uint32_t now)
{
    const auto& colors = theme();
    const int32_t previous_cursor_x = M5.Display.getCursorX();
    const int32_t previous_cursor_y = M5.Display.getCursorY();
    const auto previous_datum = M5.Display.getTextDatum();
    const int32_t diag_x = 204;
    const int32_t diag_right_x = statusControlsLeftX() - 6;
    constexpr int32_t kDiagnosticLineOffset = 7;

    if (diag_right_x <= diag_x) {
        return;
    }

    const DiagnosticWindowStats window = diagnosticWindowStats();
    char line1[72];
    char line2[72];
    snprintf(
        line1,
        sizeof(line1),
        "p=%d a=%d x=%d i=%+ld v=%ld e=%lu n=%lu",
        status.raw_pin_high ? 1 : 0,
        status.api_charging ? 1 : 0,
        status.current_charging ? 1 : 0,
        static_cast<long>(status.battery_current_ma),
        static_cast<long>(status.battery_voltage_mv),
        static_cast<unsigned long>(charging_change_count),
        static_cast<unsigned long>(charging_sample_count));
    snprintf(
        line2,
        sizeof(line2),
        "w=%u ai=%+ld r=%+ld..%+ld av=%ld all=%+ld..%+ld",
        static_cast<unsigned>(window.sample_count),
        static_cast<long>(window.current_avg_ma),
        static_cast<long>(window.current_min_ma),
        static_cast<long>(window.current_max_ma),
        static_cast<long>(window.voltage_avg_mv),
        static_cast<long>(diag_current_min_ma),
        static_cast<long>(diag_current_max_ma));

    M5.Display.fillRect(
        diag_x,
        0,
        diag_right_x - diag_x,
        STATUS_BAR_HEIGHT,
        colors.status_background);
    setStatusTextStyle(colors.status_text, colors.status_background, 1);
    M5.Display.setTextDatum(m5gfx::textdatum_t::middle_right);
    M5.Display.drawString(line1, diag_right_x, kStatusCenterY - kDiagnosticLineOffset);
    M5.Display.drawString(line2, diag_right_x, kStatusCenterY + kDiagnosticLineOffset);

    M5.Display.setTextDatum(previous_datum);
    applyTerminalTextStyle();
    M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
}
#endif

void fillBolt(const Point *points, int32_t dx, int32_t dy, uint16_t color)
{
    M5.Display.fillTriangle(
        points[0].x + dx,
        points[0].y + dy,
        points[1].x + dx,
        points[1].y + dy,
        points[2].x + dx,
        points[2].y + dy,
        color);
    M5.Display.fillTriangle(
        points[0].x + dx,
        points[0].y + dy,
        points[2].x + dx,
        points[2].y + dy,
        points[5].x + dx,
        points[5].y + dy,
        color);
    M5.Display.fillTriangle(
        points[5].x + dx,
        points[5].y + dy,
        points[2].x + dx,
        points[2].y + dy,
        points[3].x + dx,
        points[3].y + dy,
        color);
    M5.Display.fillTriangle(
        points[5].x + dx,
        points[5].y + dy,
        points[3].x + dx,
        points[3].y + dy,
        points[4].x + dx,
        points[4].y + dy,
        color);
}

void drawChargingBolt(int32_t icon_x, int32_t icon_y)
{
    const auto& colors = theme();
    const int32_t center_x = icon_x + kBatteryArea.icon_body_width / 2;
    const Point body[] = {
        {center_x + 5, icon_y + 1},
        {center_x - 3, icon_y + 9},
        {center_x + 2, icon_y + 9},
        {center_x - 3, icon_y + kBatteryArea.icon_height - 1},
        {center_x + 10, icon_y + 7},
        {center_x + 4, icon_y + 8},
    };
    constexpr Point outline_offsets[] = {
        {-1, -1},
        {0, -1},
        {1, -1},
        {-1, 0},
        {1, 0},
        {-1, 1},
        {0, 1},
        {1, 1},
    };

    for (const Point& offset : outline_offsets) {
        fillBolt(body, offset.x, offset.y, colors.screen_background);
    }
    fillBolt(body, 0, 0, colors.battery_lightning);
}

void fillSmoothBatteryShape(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    int32_t radius,
    uint16_t color)
{
    M5.Display.fillSmoothRoundRect(x, y, width, height, radius, color);
}

void drawBatteryIcon(
    int32_t x,
    int32_t y,
    int32_t battery_level,
    bool charging,
    uint16_t panel_background)
{
    const auto& colors = theme();
    const int32_t cap_x = x + kBatteryArea.icon_body_width;
    const int32_t cap_y = y + (kBatteryArea.icon_height - kBatteryArea.icon_cap_height) / 2;
    const int32_t inner_x = x + kBatteryArea.inner_x_inset;
    const int32_t inner_y = y + kBatteryArea.inner_y_inset;
    const int32_t inner_width = kBatteryArea.icon_body_width - kBatteryArea.inner_x_inset * 2;
    const int32_t inner_height = kBatteryArea.icon_height - kBatteryArea.inner_y_inset * 2;
    int32_t fill_width = 0;

    if (battery_level > 0) {
        fill_width = (inner_width * battery_level + 99) / 100;
        if (fill_width < 1) {
            fill_width = 1;
        } else if (fill_width > inner_width) {
            fill_width = inner_width;
        }
    }

    fillSmoothBatteryShape(
        x,
        y,
        kBatteryArea.icon_body_width,
        kBatteryArea.icon_height,
        kBatteryArea.body_corner_radius,
        colors.battery_outline);
    fillSmoothBatteryShape(
        x + kBatteryArea.shell_inset,
        y + kBatteryArea.shell_inset,
        kBatteryArea.icon_body_width - kBatteryArea.shell_inset * 2,
        kBatteryArea.icon_height - kBatteryArea.shell_inset * 2,
        kBatteryArea.body_inner_corner_radius,
        panel_background);
    fillSmoothBatteryShape(
        cap_x,
        cap_y,
        kBatteryArea.icon_cap_width,
        kBatteryArea.icon_cap_height,
        kBatteryArea.cap_corner_radius,
        colors.battery_outline);
    fillSmoothBatteryShape(
        inner_x,
        inner_y,
        inner_width,
        inner_height,
        kBatteryArea.track_corner_radius,
        colors.battery_empty_track);

    if (fill_width > 0) {
        fillSmoothBatteryShape(
            inner_x,
            inner_y,
            fill_width,
            inner_height,
            maxInt(1, minInt(kBatteryArea.track_corner_radius, fill_width / 2)),
            batteryFillColor(battery_level, charging));
    }

    if (charging) {
        drawChargingBolt(x, y);
    }
}

char temperatureUnitSuffixFor(TemperatureDisplayUnit unit)
{
    switch (unit) {
    case TemperatureDisplayUnit::Celsius:
        return 'C';
    case TemperatureDisplayUnit::Fahrenheit:
        return 'F';
    case TemperatureDisplayUnit::Kelvin:
        return 'K';
    default:
        return 'C';
    }
}

char temperatureUnitSuffix()
{
    return temperatureUnitSuffixFor(temperature_unit);
}

bool temperatureUnitUsesDegree(char suffix)
{
    return suffix == 'C' || suffix == 'F';
}

float convertTemperatureForUnit(float celsius, TemperatureDisplayUnit unit)
{
    switch (unit) {
    case TemperatureDisplayUnit::Celsius:
        return celsius;
    case TemperatureDisplayUnit::Fahrenheit:
        return temperature_monitor::toFahrenheit(celsius);
    case TemperatureDisplayUnit::Kelvin:
        return temperature_monitor::toKelvin(celsius);
    default:
        return celsius;
    }
}

float convertTemperature(float celsius)
{
    return convertTemperatureForUnit(celsius, temperature_unit);
}

int32_t roundTemperature(float value)
{
    return static_cast<int32_t>(value + (value >= 0.0f ? 0.5f : -0.5f));
}

void formatTemperatureTextForUnit(
    char *buffer,
    size_t buffer_size,
    const temperature_monitor::TemperatureReading& reading,
    TemperatureDisplayUnit unit)
{
    const char suffix = temperatureUnitSuffixFor(unit);
    if (!reading.valid) {
        snprintf(buffer, buffer_size, "--%c", suffix);
        return;
    }

    const int32_t display_value =
        roundTemperature(convertTemperatureForUnit(reading.celsius, unit));
    snprintf(buffer, buffer_size, "%ld%c", static_cast<long>(display_value), suffix);
}

void formatTemperatureText(char *buffer, size_t buffer_size)
{
    const temperature_monitor::TemperatureReading reading =
        temperature_monitor::primaryReading();
    formatTemperatureTextForUnit(buffer, buffer_size, reading, temperature_unit);
}

bool splitTemperatureUnit(
    const char *text,
    char *value_text,
    size_t value_text_size,
    char *unit_text,
    size_t unit_text_size)
{
    if (text == nullptr || value_text == nullptr || unit_text == nullptr
        || value_text_size == 0 || unit_text_size < 2) {
        return false;
    }

    const size_t length = strlen(text);
    if (length < 2) {
        return false;
    }

    const char suffix = text[length - 1];
    if (!temperatureUnitUsesDegree(suffix)) {
        return false;
    }

    const size_t value_length =
        length - 1 < value_text_size - 1 ? length - 1 : value_text_size - 1;
    memcpy(value_text, text, value_length);
    value_text[value_length] = '\0';
    unit_text[0] = suffix;
    unit_text[1] = '\0';
    return true;
}

void drawTemperatureIcon(int32_t x, int32_t center_y, uint16_t color)
{
    const int32_t icon_y = center_y - kTemperatureArea.icon_height / 2;
    const int32_t center_x = x + kTemperatureArea.icon_width / 2;
    const int32_t stem_x = center_x - kTemperatureArea.icon_stem_width / 2;
    const int32_t stem_y = icon_y + 1;
    const int32_t stem_height =
        kTemperatureArea.icon_height - kTemperatureArea.icon_bulb_radius - 2;
    const int32_t bulb_y =
        icon_y + kTemperatureArea.icon_height - kTemperatureArea.icon_bulb_radius - 1;
    const int32_t stem_radius = maxInt(1, kTemperatureArea.icon_stem_width / 2);
    const int32_t tick_x =
        center_x + kTemperatureArea.icon_stem_width / 2 + 1;

    M5.Display.fillRoundRect(
        stem_x,
        stem_y,
        kTemperatureArea.icon_stem_width,
        stem_height,
        stem_radius,
        color);
    M5.Display.fillCircle(
        center_x,
        bulb_y,
        kTemperatureArea.icon_bulb_radius,
        color);

    M5.Display.drawFastHLine(tick_x, stem_y + 4, 4, color);
    M5.Display.drawFastHLine(tick_x, stem_y + 9, 3, color);
    M5.Display.drawFastHLine(tick_x, stem_y + 14, 4, color);
}

void drawTemperatureStatus(const char *text)
{
    const auto& colors = theme();
    const StatusBarAreaColors area_colors =
        statusBarAreaColors(colors, kTemperatureArea.area);
    const int32_t previous_cursor_x = M5.Display.getCursorX();
    const int32_t previous_cursor_y = M5.Display.getCursorY();
    const auto previous_datum = M5.Display.getTextDatum();

    setStatusTextStyle(
        area_colors.text,
        area_colors.background,
        kTemperatureArea.area.text_size);
    const int32_t layout_text_width = maximumTemperatureTextWidth(text);
    const int32_t content_width = temperatureContentWidthForTextWidth(layout_text_width);
    const Rect area =
        temperatureAreaRectForContentWidth(content_width, batteryAreaRect());
    if (area.width <= 0 || area.height <= 0) {
        M5.Display.setTextDatum(previous_datum);
        applyTerminalTextStyle();
        M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
        return;
    }

    if (last_temperature_area_valid && !rectEquals(last_temperature_area, area)) {
        clearStatusArea(last_temperature_area);
    }
    last_temperature_area = area;
    last_temperature_area_valid = true;

    M5.Display.fillRect(
        area.x,
        area.y,
        area.width,
        area.height,
        area_colors.background);
    M5.Display.setTextDatum(m5gfx::textdatum_t::middle_left);

    char value_text[sizeof(last_temperature_text)] = {};
    char unit_text[2] = {};
    const int32_t text_width = temperatureTextWidth(text);
    const int32_t drawn_content_width = content_width;
    const int32_t content_x =
        area.x + maxInt(0, (area.width - drawn_content_width) / 2);
    const int32_t text_x = content_x + maxInt(0, layout_text_width - text_width);
    const int32_t center_y = area.y + area.height / 2;
    const bool draw_degree =
        splitTemperatureUnit(text, value_text, sizeof(value_text), unit_text, sizeof(unit_text));
    if (draw_degree) {
        const int32_t value_width = M5.Display.textWidth(value_text);
        const int32_t degree_center_x =
            text_x
            + value_width
            + kTemperatureArea.degree_gap
            + kTemperatureArea.degree_radius;
        const int32_t degree_center_y = center_y + kTemperatureArea.degree_y_offset;
        const int32_t unit_x =
            degree_center_x
            + kTemperatureArea.degree_radius
            + kTemperatureArea.degree_unit_gap;

        M5.Display.drawString(value_text, text_x, center_y);
        M5.Display.drawCircle(
            degree_center_x,
            degree_center_y,
            kTemperatureArea.degree_radius,
            area_colors.text);
        M5.Display.drawString(unit_text, unit_x, center_y);
    } else {
        M5.Display.drawString(text, text_x, center_y);
    }
    drawTemperatureIcon(
        content_x + layout_text_width + kTemperatureArea.icon_gap,
        center_y,
        area_colors.text);

    M5.Display.setTextDatum(previous_datum);
    applyTerminalTextStyle();
    M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
}

void drawBatteryStatus(int32_t battery_level, bool charging)
{
    const auto& colors = theme();
    const StatusBarAreaColors area_colors =
        statusBarAreaColors(colors, kBatteryArea.area);
    char text[16];
    formatBatteryPercent(text, sizeof(text), battery_level);

    const int32_t previous_cursor_x = M5.Display.getCursorX();
    const int32_t previous_cursor_y = M5.Display.getCursorY();
    const auto previous_datum = M5.Display.getTextDatum();
    setStatusTextStyle(
        area_colors.text,
        area_colors.background,
        kBatteryArea.area.text_size);

    const int32_t text_width = M5.Display.textWidth(text);
    const int32_t icon_width = batteryIconWidth();
    const int32_t content_width = text_width + kBatteryArea.text_icon_gap + icon_width;
    const Rect area = batteryAreaRectForContentWidth(content_width);
    if (area.width <= 0 || area.height <= 0) {
        M5.Display.setTextDatum(previous_datum);
        applyTerminalTextStyle();
        M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
        return;
    }

    const bool battery_area_changed =
        !last_battery_area_valid || !rectEquals(last_battery_area, area);
    if (battery_area_changed) {
        if (last_battery_area_valid) {
            clearStatusArea(last_battery_area);
        }
        if (last_temperature_area_valid) {
            clearStatusArea(last_temperature_area);
            last_temperature_area_valid = false;
        }
    }
    last_battery_area = area;
    last_battery_area_valid = true;

    M5.Display.fillRect(
        area.x,
        area.y,
        area.width,
        area.height,
        area_colors.background);

    const int32_t content_x = area.x + maxInt(0, (area.width - content_width) / 2);
    const int32_t icon_x = content_x + text_width + kBatteryArea.text_icon_gap;
    const int32_t icon_y = area.y + (area.height - kBatteryArea.icon_height) / 2;
    const int32_t text_right_x = content_x + text_width;
    const int32_t text_center_y = area.y + area.height / 2;

    M5.Display.setTextDatum(m5gfx::textdatum_t::middle_right);
    M5.Display.drawString(text, text_right_x, text_center_y);
    drawBatteryIcon(icon_x, icon_y, battery_level, charging, area_colors.background);

    if (battery_area_changed && temperature_status_drawn) {
        drawTemperatureStatus(last_temperature_text);
    }

    M5.Display.setTextDatum(previous_datum);
    applyTerminalTextStyle();
    M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
}

const char *powerMenuLabel(PowerMenuItem item)
{
    const PowerMenuItemSpec *spec = powerMenuItemSpecFor(item);
    return spec == nullptr ? "" : spec->label;
}

const char *powerMenuBusyLabel(PowerMenuItem item)
{
    const PowerMenuItemSpec *spec = powerMenuItemSpecFor(item);
    return spec == nullptr ? "" : spec->busy_label;
}

void setPowerMenuTextStyle(uint16_t text_color, uint16_t background_color)
{
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextScroll(false);
    M5.Display.setTextDatum(m5gfx::textdatum_t::middle_left);
    M5.Display.setTextColor(text_color, background_color);
    M5.Display.setTextSize(kPowerMenuTextSize);
}

void drawPowerMenu()
{
    if (!power_menu_visible) {
        return;
    }

    const auto& colors = theme();
    const Rect menu = powerMenuRect();
    if (menu.width <= 0 || menu.height <= 0) {
        return;
    }

    const int32_t previous_cursor_x = M5.Display.getCursorX();
    const int32_t previous_cursor_y = M5.Display.getCursorY();
    const auto previous_datum = M5.Display.getTextDatum();

    M5.Display.fillRoundRect(
        menu.x,
        menu.y,
        menu.width,
        menu.height,
        kPowerMenuBorderRadius,
        colors.menu_background);

    setPowerMenuTextStyle(colors.menu_text, colors.menu_background);
    for (int32_t index = 0; index < kPowerMenuItemCount; ++index) {
        const int32_t item_y = menu.y + index * kPowerMenuItemHeight;
        if (item_y + kPowerMenuItemHeight > menu.y + menu.height) {
            break;
        }

        const PowerMenuItemSpec *item_spec = powerMenuItemSpecAt(index);
        if (item_spec == nullptr) {
            break;
        }

        const PowerMenuItem item = item_spec->item;
        const bool pressed = item == power_menu_pressed_item;
        const PowerMenuItemStyle item_style = powerMenuItemStyle(
            colors,
            pressed ? UiWidgetState::Pressed : UiWidgetState::Default);
        if (pressed) {
            M5.Display.fillRect(
                menu.x + 1,
                item_y + 1,
                maxInt(0, menu.width - 2),
                kPowerMenuItemHeight - 1,
                item_style.background);
        }
        if (index > 0) {
            M5.Display.drawFastHLine(
                menu.x + kPowerMenuSeparatorInset,
                item_y,
                maxInt(0, menu.width - kPowerMenuSeparatorInset * 2),
                colors.menu_separator);
        }

        M5.Display.setTextColor(item_style.text, item_style.background);
        M5.Display.drawString(
            item_spec->label,
            menu.x + kPowerMenuLabelLeftPadding,
            item_y + kPowerMenuItemHeight / 2);
    }

    M5.Display.drawRoundRect(
        menu.x,
        menu.y,
        menu.width,
        menu.height,
        kPowerMenuBorderRadius,
        colors.menu_border);

    M5.Display.setTextDatum(previous_datum);
    applyTerminalTextStyle();
    M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
}

void restoreAfterPowerMenu()
{
    const auto& colors = theme();
    const Rect menu = powerMenuRect();
    if (menu.width > 0 && menu.height > 0) {
        M5.Display.fillRect(menu.x, menu.y, menu.width, menu.height, colors.screen_background);
    }
    redrawTerminalView();
    applyTerminalTextStyle();
}

void resetPowerMenuTouchCapture()
{
    power_menu_touch_active = false;
    power_menu_touch_id = 0;
    power_menu_captured_item = PowerMenuItem::None;
    power_menu_pressed_item = PowerMenuItem::None;
}

void resetTemperatureTouchCapture()
{
    temperature_touch_active = false;
    temperature_touch_id = 0;
    temperature_touch_inside = false;
}

void cycleTemperatureUnit()
{
    switch (temperature_unit) {
    case TemperatureDisplayUnit::Celsius:
        temperature_unit = TemperatureDisplayUnit::Fahrenheit;
        break;
    case TemperatureDisplayUnit::Fahrenheit:
        temperature_unit = TemperatureDisplayUnit::Kelvin;
        break;
    case TemperatureDisplayUnit::Kelvin:
    default:
        temperature_unit = TemperatureDisplayUnit::Celsius;
        break;
    }
    refreshTemperatureStatus(true);
}

void hidePowerMenu(bool restore_terminal)
{
    if (!power_menu_visible) {
        resetPowerMenuTouchCapture();
        return;
    }

    power_menu_visible = false;
    resetPowerMenuTouchCapture();
    if (restore_terminal) {
        restoreAfterPowerMenu();
    }
}

void showPowerMenu()
{
    power_menu_visible = true;
    power_menu_pressed_item = PowerMenuItem::None;
    drawPowerMenu();
}

void drawPowerMenuBusy(const char *label)
{
    const auto& colors = theme();
    const Rect menu = powerMenuRect();
    if (menu.width <= 0 || menu.height <= 0) {
        return;
    }

    const int32_t previous_cursor_x = M5.Display.getCursorX();
    const int32_t previous_cursor_y = M5.Display.getCursorY();
    const auto previous_datum = M5.Display.getTextDatum();

    M5.Display.fillRoundRect(
        menu.x,
        menu.y,
        menu.width,
        menu.height,
        kPowerMenuBorderRadius,
        colors.menu_background);
    M5.Display.drawRoundRect(
        menu.x,
        menu.y,
        menu.width,
        menu.height,
        kPowerMenuBorderRadius,
        colors.menu_border);
    setPowerMenuTextStyle(colors.menu_text, colors.menu_background);
    M5.Display.setTextDatum(m5gfx::textdatum_t::middle_center);
    M5.Display.drawString(label, menu.x + menu.width / 2, menu.y + menu.height / 2);
    M5.Display.waitDisplay();

    M5.Display.setTextDatum(previous_datum);
    applyTerminalTextStyle();
    M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
}

void performPowerMenuAction(PowerMenuItem item)
{
    power_menu_visible = false;
    resetPowerMenuTouchCapture();

    switch (item) {
    case PowerMenuItem::PowerOff:
        drawPowerMenuBusy(powerMenuBusyLabel(item));
        Serial.println("[ui] power menu: power off requested");
        Serial.flush();
        delay(kPowerActionDisplayDelayMs);
        M5.Power.powerOff();
        break;
    case PowerMenuItem::Restart:
        drawPowerMenuBusy(powerMenuBusyLabel(item));
        Serial.println("[ui] power menu: restart requested");
        Serial.flush();
        delay(kPowerActionDisplayDelayMs);
        ESP.restart();
        break;
    case PowerMenuItem::None:
    default:
        break;
    }
}

bool handleTemperatureTouchPressed(const m5::Touch_Class::touch_detail_t& touch)
{
    if (power_menu_visible || !containsPoint(temperatureAreaRect(), touch.x, touch.y)) {
        return false;
    }

    temperature_touch_active = true;
    temperature_touch_id = touch.id;
    temperature_touch_inside = true;
    return true;
}

bool handleTemperatureTouchMove(const m5::Touch_Class::touch_detail_t& touch)
{
    if (!temperature_touch_active || touch.id != temperature_touch_id) {
        return false;
    }

    temperature_touch_inside = containsPoint(temperatureAreaRect(), touch.x, touch.y);
    return true;
}

bool handleTemperatureTouchReleased(const m5::Touch_Class::touch_detail_t& touch)
{
    if (!temperature_touch_active || touch.id != temperature_touch_id) {
        return false;
    }

    const bool activate =
        temperature_touch_inside && containsPoint(temperatureAreaRect(), touch.x, touch.y);
    resetTemperatureTouchCapture();
    if (activate) {
        cycleTemperatureUnit();
    }
    return true;
}

void handlePowerMenuTouchPressed(const m5::Touch_Class::touch_detail_t& touch)
{
    power_menu_touch_active = true;
    power_menu_touch_id = touch.id;
    power_menu_captured_item = PowerMenuItem::None;
    power_menu_pressed_item = PowerMenuItem::None;

    if (power_menu_visible) {
        const PowerMenuItem item = powerMenuItemAt(touch.x, touch.y);
        if (item != PowerMenuItem::None) {
            power_menu_captured_item = item;
            power_menu_pressed_item = item;
            drawPowerMenu();
            return;
        }

        hidePowerMenu(true);
        power_menu_touch_active = true;
        power_menu_touch_id = touch.id;
        return;
    }

    if (containsPoint(batteryAreaRect(), touch.x, touch.y)) {
        showPowerMenu();
        return;
    }

    resetPowerMenuTouchCapture();
}

void handlePowerMenuTouchMove(const m5::Touch_Class::touch_detail_t& touch)
{
    if (!power_menu_touch_active || touch.id != power_menu_touch_id) {
        return;
    }
    if (!power_menu_visible || power_menu_captured_item == PowerMenuItem::None) {
        return;
    }

    const PowerMenuItem item = powerMenuItemAt(touch.x, touch.y);
    const PowerMenuItem next_pressed =
        item == power_menu_captured_item ? power_menu_captured_item : PowerMenuItem::None;
    if (next_pressed != power_menu_pressed_item) {
        power_menu_pressed_item = next_pressed;
        drawPowerMenu();
    }
}

void handlePowerMenuTouchReleased(const m5::Touch_Class::touch_detail_t& touch)
{
    if (!power_menu_touch_active || touch.id != power_menu_touch_id) {
        return;
    }

    PowerMenuItem action = PowerMenuItem::None;
    if (power_menu_visible && power_menu_captured_item != PowerMenuItem::None) {
        const PowerMenuItem item = powerMenuItemAt(touch.x, touch.y);
        if (item == power_menu_captured_item) {
            action = item;
        }
    }

    resetPowerMenuTouchCapture();
    if (action != PowerMenuItem::None) {
        performPowerMenuAction(action);
        return;
    }
    if (power_menu_visible) {
        drawPowerMenu();
    }
}
} // namespace

void beginStatusBar(const TerminalTheme& selected_theme)
{
    active_theme = &selected_theme;
    last_battery_level_update_ms = 0;
    last_charging_update_ms = 0;
    last_battery_level = kUninitializedBatteryLevel;
    last_charging = false;
    last_charging_valid = false;
    charging_started_ms = 0;
    charging_stopped_ms = 0;
    inferred_current_charging = false;
    inferred_current_charging_valid = false;
    pending_current_charging = false;
    pending_current_charging_count = 0;
    temperature_unit = TemperatureDisplayUnit::Celsius;
    temperature_status_drawn = false;
    last_temperature_text[0] = '\0';
    last_temperature_area = {};
    last_temperature_area_valid = false;
    last_battery_area = {};
    last_battery_area_valid = false;
    resetTemperatureTouchCapture();
    power_menu_visible = false;
    resetPowerMenuTouchCapture();
#if ENABLE_POWER_STATUS_DIAGNOSTICS
    charging_sample_count = 0;
    charging_change_count = 0;
    last_diag_api_charging = false;
    last_diag_raw_pin_high = false;
    last_diag_current_charging = false;
    diag_current_min_ma = 0;
    diag_current_max_ma = 0;
    resetDiagnosticWindow();
    last_diag_valid = false;
#endif
}

void drawStatusBar(const char *title)
{
    const auto& colors = theme();
    const int32_t width = M5.Display.width();

    M5.Display.fillRect(0, 0, width, STATUS_BAR_HEIGHT, colors.status_background);
    setStatusTextStyle(colors.status_text, colors.status_background, 1);
    M5.Display.setTextDatum(m5gfx::textdatum_t::middle_left);
    M5.Display.drawString(title, kStatusTextX, kStatusCenterY);
    refreshBatteryStatus(true);
    refreshTemperatureStatus(true);
}

void refreshTemperatureStatus(bool force)
{
    if (active_theme == nullptr) {
        return;
    }

    char text[sizeof(last_temperature_text)] = {};
    formatTemperatureText(text, sizeof(text));
    if (!force && temperature_status_drawn && strcmp(text, last_temperature_text) == 0) {
        return;
    }

    strncpy(last_temperature_text, text, sizeof(last_temperature_text) - 1);
    last_temperature_text[sizeof(last_temperature_text) - 1] = '\0';
    temperature_status_drawn = true;
    drawTemperatureStatus(last_temperature_text);
}

void refreshBatteryStatus(bool force)
{
    const uint32_t now = millis();
    const bool previous_charging_valid = last_charging_valid;
    const bool previous_charging = last_charging;
    const bool need_battery_level =
        force
        || last_battery_level == kUninitializedBatteryLevel
        || intervalElapsed(now, last_battery_level_update_ms, kBatteryLevelUpdateIntervalMs);
    const bool need_charging =
        force
        || !last_charging_valid
        || intervalElapsed(now, last_charging_update_ms, kChargingUpdateIntervalMs);

    if (!need_battery_level && !need_charging) {
        return;
    }

    int32_t battery_level = last_battery_level;
    bool charging = last_charging;
    ChargingStatus charging_status = {
        last_charging,
        last_charging,
        last_charging,
        0,
        0,
    };

    if (need_charging) {
        charging_status = readChargingStatus();
        charging = charging_status.current_charging;
        last_charging_update_ms = now;
        if (previous_charging_valid && !previous_charging && charging) {
            charging_started_ms = now;
            charging_stopped_ms = 0;
        } else if (previous_charging_valid && previous_charging && !charging) {
            charging_started_ms = 0;
            charging_stopped_ms = now;
        }
#if ENABLE_POWER_STATUS_DIAGNOSTICS
        recordChargingDiagnostic(charging_status, now);
#endif
    }
    if (need_battery_level) {
        battery_level =
            displayedBatteryLevelFromRaw(readBatteryLevel(), last_battery_level, charging, now);
        last_battery_level_update_ms = now;
    }

    const bool display_changed =
        force
        || battery_level != last_battery_level
        || !last_charging_valid
        || charging != last_charging;

    if (display_changed) {
        last_battery_level = battery_level;
        last_charging = charging;
        last_charging_valid = true;
        drawBatteryStatus(battery_level, charging);
    }

#if ENABLE_POWER_STATUS_DIAGNOSTICS
    if (need_charging) {
        drawPowerDiagnostics(charging_status, now);
    }
#endif
}

void updateStatusBarTouch()
{
    if (active_theme == nullptr || !M5.Touch.isEnabled()) {
        return;
    }

    const uint8_t touch_count = M5.Touch.getCount();
    for (uint8_t index = 0; index < touch_count; ++index) {
        const auto& touch = M5.Touch.getDetail(index);
        if (touch.wasPressed()) {
            if (!handleTemperatureTouchPressed(touch)) {
                handlePowerMenuTouchPressed(touch);
            }
        }
        if (touch.isPressed()) {
            if (!handleTemperatureTouchMove(touch)) {
                handlePowerMenuTouchMove(touch);
            }
        }
        if (touch.wasReleased()) {
            if (!handleTemperatureTouchReleased(touch)) {
                handlePowerMenuTouchReleased(touch);
            }
        }
    }

    if (touch_count == 0 && temperature_touch_active) {
        resetTemperatureTouchCapture();
    }
    if (touch_count == 0 && power_menu_touch_active) {
        resetPowerMenuTouchCapture();
    }

    if (power_menu_visible) {
        drawPowerMenu();
    }
}

void applyTerminalTextStyle()
{
    const auto& colors = theme();
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextDatum(m5gfx::textdatum_t::top_left);
    M5.Display.setTextColor(colors.terminal_text, colors.screen_background);
    M5.Display.setTextSize(TERMINAL_TEXT_SIZE);
    M5.Display.setTextScroll(true);
}

uint16_t terminalBackground()
{
    return theme().screen_background;
}

} // namespace ui
