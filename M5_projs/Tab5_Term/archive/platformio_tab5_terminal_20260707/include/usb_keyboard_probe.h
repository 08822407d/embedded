#pragma once

// Historical name: this module started as a USB-A keyboard probe. It now also
// backs the formal USB-A keyboard path when ENABLE_USB_KEYBOARD_PROBE is set.
void usbKeyboardProbeBegin();
bool usbKeyboardProbeIsEnabled();
