#pragma once

#include <stdint.h>

#include "input_mapper.h"

namespace input {

// Normalizes USB HID boot-keyboard usage/modifier bytes into the shared
// input_mapper model. Both the official A164 path and the USB-A host path use
// this layer before terminal-specific escape sequences are generated.
uint8_t normalizeHidModifiers(uint8_t hid_modifiers);
KeyCode hidUsageToKeyCode(uint8_t usage);

} // namespace input
