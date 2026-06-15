#pragma once

#include <stdint.h>

#include "input_mapper.h"

namespace input {

uint8_t normalizeHidModifiers(uint8_t hid_modifiers);
KeyCode hidUsageToKeyCode(uint8_t usage);

} // namespace input
