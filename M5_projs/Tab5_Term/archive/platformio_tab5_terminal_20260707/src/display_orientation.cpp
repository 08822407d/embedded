#include "display_orientation.h"

#include <M5Unified.h>

#include "app_config.h"

namespace ui {
namespace {
constexpr uint8_t kStandardLandscapeRotation = 1;
constexpr uint8_t kHalfTurn = 2;

DisplayOrientation active_orientation = DisplayOrientation::StandardLandscape;
uint8_t active_rotation = kStandardLandscapeRotation;

uint8_t rotationFor(DisplayOrientation orientation)
{
    if (orientation == DisplayOrientation::KeyboardMountedLandscape) {
        return (kStandardLandscapeRotation + kHalfTurn) & 0x03;
    }

    return kStandardLandscapeRotation;
}
} // namespace

void applyDisplayOrientation(DisplayOrientation orientation)
{
    active_orientation = orientation;
    active_rotation = rotationFor(orientation);
    M5.Display.setRotation(active_rotation);
}

void applyConfiguredDisplayOrientation()
{
#if SCREEN_ORIENTATION == SCREEN_ORIENTATION_KEYBOARD_MOUNTED
    applyDisplayOrientation(DisplayOrientation::KeyboardMountedLandscape);
#elif SCREEN_ORIENTATION == SCREEN_ORIENTATION_STANDARD
    applyDisplayOrientation(DisplayOrientation::StandardLandscape);
#else
#error "Unsupported SCREEN_ORIENTATION"
#endif
}

DisplayOrientation displayOrientation()
{
    return active_orientation;
}

uint8_t displayRotation()
{
    return active_rotation;
}

} // namespace ui
