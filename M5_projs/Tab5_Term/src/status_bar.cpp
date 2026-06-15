#include "status_bar.h"

#include <M5Unified.h>
#include <stdint.h>
#include <stdio.h>

#include "app_config.h"

namespace ui {
namespace {
constexpr int32_t kStatusTextX = 4;
constexpr int32_t kStatusCenterY = STATUS_BAR_HEIGHT / 2;
// Tab5 CHG_STAT was observed to miss repeated cable transitions on the tested
// hardware. The lightning icon uses a debounced INA226-current heuristic.
constexpr uint32_t kBatteryLevelUpdateIntervalMs = 10000;
constexpr uint32_t kChargingUpdateIntervalMs = 500;
constexpr int32_t kExternalPowerEnterCurrentMa = -300;
constexpr int32_t kExternalPowerExitCurrentMa = -600;
constexpr uint8_t kExternalPowerSamplesToSwitch = 2;
constexpr int32_t kUnknownBatteryLevel = -1;
constexpr int32_t kUninitializedBatteryLevel = -2;

struct BatteryAreaLayout {
    int32_t width;
    int32_t height;
    int32_t right_inset;
    int32_t percent_text_size;
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
    .width = 124,
    .height = STATUS_BAR_HEIGHT,
    .right_inset = 0,
    .percent_text_size = 2,
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

struct ChargingStatus {
    bool api_charging;
    bool raw_pin_high;
    bool external_power;
    int32_t battery_current_ma;
    int32_t battery_voltage_mv;
};

const TerminalTheme *active_theme = nullptr;
uint32_t last_battery_level_update_ms = 0;
uint32_t last_charging_update_ms = 0;
int32_t last_battery_level = kUninitializedBatteryLevel;
bool last_charging = false;
bool last_charging_valid = false;
bool inferred_external_power = false;
bool inferred_external_power_valid = false;
bool pending_external_power = false;
uint8_t pending_external_power_count = 0;
#if ENABLE_POWER_STATUS_DIAGNOSTICS
constexpr size_t kDiagnosticWindowSampleCount = 10;

uint32_t charging_sample_count = 0;
uint32_t charging_change_count = 0;
bool last_diag_api_charging = false;
bool last_diag_raw_pin_high = false;
bool last_diag_external_power = false;
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

bool updateExternalPowerHeuristic(int32_t battery_current_ma)
{
    if (!inferred_external_power_valid) {
        inferred_external_power =
            battery_current_ma > kExternalPowerEnterCurrentMa;
        inferred_external_power_valid = true;
        pending_external_power = inferred_external_power;
        pending_external_power_count = 0;
        return inferred_external_power;
    }

    bool target = inferred_external_power;
    if (inferred_external_power) {
        if (battery_current_ma < kExternalPowerExitCurrentMa) {
            target = false;
        }
    } else if (battery_current_ma > kExternalPowerEnterCurrentMa) {
        target = true;
    }

    if (target == inferred_external_power) {
        pending_external_power = target;
        pending_external_power_count = 0;
        return inferred_external_power;
    }

    if (pending_external_power != target || pending_external_power_count == 0) {
        pending_external_power = target;
        pending_external_power_count = 1;
    } else if (pending_external_power_count < kExternalPowerSamplesToSwitch) {
        ++pending_external_power_count;
    }

    if (pending_external_power_count >= kExternalPowerSamplesToSwitch) {
        inferred_external_power = target;
        pending_external_power_count = 0;
    }
    return inferred_external_power;
}

ChargingStatus readChargingStatus()
{
    const bool api_charging = M5.Power.isCharging() == m5::Power_Class::is_charging;
    const int32_t battery_current_ma = M5.Power.getBatteryCurrent();
    const bool external_power = updateExternalPowerHeuristic(battery_current_ma);
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
        external_power,
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

#if ENABLE_POWER_STATUS_DIAGNOSTICS
struct DiagnosticWindowStats {
    size_t sample_count;
    int32_t current_avg_ma;
    int32_t current_min_ma;
    int32_t current_max_ma;
    int32_t voltage_avg_mv;
};

int32_t batteryAreaLeftX()
{
    const int32_t area_right_x = M5.Display.width() - kBatteryArea.right_inset;
    const int32_t area_width = minInt(kBatteryArea.width, area_right_x);
    return area_right_x - area_width;
}

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
            || status.external_power != last_diag_external_power) {
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
    last_diag_external_power = status.external_power;
    last_diag_valid = true;
}

void drawPowerDiagnostics(const ChargingStatus& status, uint32_t now)
{
    const auto& colors = theme();
    const int32_t previous_cursor_x = M5.Display.getCursorX();
    const int32_t previous_cursor_y = M5.Display.getCursorY();
    const auto previous_datum = M5.Display.getTextDatum();
    const int32_t diag_x = 204;
    const int32_t diag_right_x = batteryAreaLeftX() - 6;

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
        status.external_power ? 1 : 0,
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
    M5.Display.drawString(line1, diag_right_x, 9);
    M5.Display.drawString(line2, diag_right_x, 23);

    M5.Display.setTextDatum(previous_datum);
    applyTerminalTextStyle();
    M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
}
#endif

void fillBolt(const Point *points, uint16_t color)
{
    M5.Display.fillTriangle(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, color);
    M5.Display.fillTriangle(points[0].x, points[0].y, points[2].x, points[2].y, points[5].x, points[5].y, color);
    M5.Display.fillTriangle(points[5].x, points[5].y, points[2].x, points[2].y, points[3].x, points[3].y, color);
    M5.Display.fillTriangle(points[5].x, points[5].y, points[3].x, points[3].y, points[4].x, points[4].y, color);
}

void drawChargingBolt(int32_t icon_x, int32_t icon_y)
{
    const auto& colors = theme();
    const int32_t center_x = icon_x + kBatteryArea.icon_body_width / 2;
    const int32_t mid_y = icon_y + kBatteryArea.icon_height / 2;
    const Point outline[] = {
        {center_x + 6, icon_y + 1},
        {center_x - 8, mid_y + 3},
        {center_x - 1, mid_y + 2},
        {center_x - 4, icon_y + kBatteryArea.icon_height - 1},
        {center_x + 9, mid_y - 4},
        {center_x + 2, mid_y - 3},
    };
    const Point body[] = {
        {center_x + 5, icon_y + 3},
        {center_x - 5, mid_y + 2},
        {center_x + 1, mid_y + 1},
        {center_x - 2, icon_y + kBatteryArea.icon_height - 4},
        {center_x + 7, mid_y - 3},
        {center_x + 1, mid_y - 2},
    };

    fillBolt(outline, colors.screen_background);
    fillBolt(body, colors.battery_lightning);
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

void drawBatteryIcon(int32_t x, int32_t y, int32_t battery_level, bool charging)
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
        colors.battery_panel_background);
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

void drawBatteryStatus(int32_t battery_level, bool charging)
{
    const auto& colors = theme();
    char text[16];
    formatBatteryPercent(text, sizeof(text), battery_level);

    const int32_t previous_cursor_x = M5.Display.getCursorX();
    const int32_t previous_cursor_y = M5.Display.getCursorY();
    const auto previous_datum = M5.Display.getTextDatum();
    const int32_t area_right_x = M5.Display.width() - kBatteryArea.right_inset;
    const int32_t area_width = minInt(kBatteryArea.width, area_right_x);
    const int32_t area_x = area_right_x - area_width;
    const int32_t area_y = 0;

    M5.Display.fillRect(
        area_x,
        area_y,
        area_width,
        kBatteryArea.height,
        colors.battery_panel_background);
    setStatusTextStyle(
        colors.battery_text,
        colors.battery_panel_background,
        kBatteryArea.percent_text_size);

    const int32_t text_width = M5.Display.textWidth(text);
    const int32_t icon_width = kBatteryArea.icon_body_width + kBatteryArea.icon_cap_width;
    const int32_t content_width = text_width + kBatteryArea.text_icon_gap + icon_width;
    const int32_t content_x = area_x + maxInt(0, (area_width - content_width) / 2);
    const int32_t icon_x = content_x + text_width + kBatteryArea.text_icon_gap;
    const int32_t icon_y = area_y + (kBatteryArea.height - kBatteryArea.icon_height) / 2;
    const int32_t text_right_x = content_x + text_width;
    const int32_t text_center_y = area_y + kBatteryArea.height / 2;

    M5.Display.setTextDatum(m5gfx::textdatum_t::middle_right);
    M5.Display.drawString(text, text_right_x, text_center_y);
    drawBatteryIcon(icon_x, icon_y, battery_level, charging);

    M5.Display.setTextDatum(previous_datum);
    applyTerminalTextStyle();
    M5.Display.setCursor(previous_cursor_x, previous_cursor_y);
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
    inferred_external_power = false;
    inferred_external_power_valid = false;
    pending_external_power = false;
    pending_external_power_count = 0;
#if ENABLE_POWER_STATUS_DIAGNOSTICS
    charging_sample_count = 0;
    charging_change_count = 0;
    last_diag_api_charging = false;
    last_diag_raw_pin_high = false;
    last_diag_external_power = false;
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
}

void refreshBatteryStatus(bool force)
{
    const uint32_t now = millis();
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

    if (need_battery_level) {
        battery_level = readBatteryLevel();
        last_battery_level_update_ms = now;
    }
    if (need_charging) {
        charging_status = readChargingStatus();
        charging = charging_status.external_power;
        last_charging_update_ms = now;
#if ENABLE_POWER_STATUS_DIAGNOSTICS
        recordChargingDiagnostic(charging_status, now);
#endif
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
