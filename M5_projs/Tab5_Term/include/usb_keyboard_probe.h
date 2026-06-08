#pragma once

#include <stdint.h>

void usbKeyboardProbeBegin();
bool usbKeyboardProbeReadByte(uint8_t *byte);
bool usbKeyboardProbeIsEnabled();
