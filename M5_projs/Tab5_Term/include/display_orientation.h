#pragma once

#include <stdint.h>

namespace ui {

enum class DisplayOrientation : uint8_t {
    StandardLandscape,
    KeyboardMountedLandscape,
};

void applyDisplayOrientation(DisplayOrientation orientation);
void applyConfiguredDisplayOrientation();
DisplayOrientation displayOrientation();
uint8_t displayRotation();

} // namespace ui
