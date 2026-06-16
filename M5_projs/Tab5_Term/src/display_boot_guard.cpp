#include "display_boot_guard.h"

#include <Arduino.h>
#include <M5Unified.h>

#include "app_config.h"

namespace display_boot_guard {
namespace {
constexpr uint32_t kBootGuardMagic = 0x54423544; // "TB5D"

RTC_DATA_ATTR uint32_t boot_guard_magic = 0;
RTC_DATA_ATTR uint8_t display_restart_attempts = 0;

bool dimensionsLookUsable(int32_t width, int32_t height)
{
    return width >= DISPLAY_BOOT_GUARD_MIN_WIDTH
        && height >= DISPLAY_BOOT_GUARD_MIN_HEIGHT
        && width <= DISPLAY_BOOT_GUARD_MAX_WIDTH
        && height <= DISPLAY_BOOT_GUARD_MAX_HEIGHT;
}

void resetAttemptCounter()
{
    boot_guard_magic = kBootGuardMagic;
    display_restart_attempts = 0;
}
} // namespace

bool displayLooksUsable()
{
    if (M5.Display.panel() == nullptr) {
        return false;
    }

    return dimensionsLookUsable(M5.Display.width(), M5.Display.height());
}

void restartIfDisplayUnusable()
{
    if (displayLooksUsable()) {
        resetAttemptCounter();
        return;
    }

    if (boot_guard_magic != kBootGuardMagic) {
        boot_guard_magic = kBootGuardMagic;
        display_restart_attempts = 0;
    }

    Serial.printf(
        "[display] unusable after M5.begin: panel=%p size=%ldx%ld attempt=%u/%u\r\n",
        M5.Display.panel(),
        static_cast<long>(M5.Display.panel() == nullptr ? -1 : M5.Display.width()),
        static_cast<long>(M5.Display.panel() == nullptr ? -1 : M5.Display.height()),
        static_cast<unsigned>(display_restart_attempts),
        static_cast<unsigned>(DISPLAY_BOOT_GUARD_MAX_RESTARTS));

    if (display_restart_attempts < DISPLAY_BOOT_GUARD_MAX_RESTARTS) {
        ++display_restart_attempts;
        Serial.println("[display] restarting to retry display detection");
        Serial.flush();
        delay(DISPLAY_BOOT_GUARD_RESTART_DELAY_MS);
        ESP.restart();
    }

    Serial.println("[display] max display-detection retries reached; continuing for diagnostics");
}

} // namespace display_boot_guard
