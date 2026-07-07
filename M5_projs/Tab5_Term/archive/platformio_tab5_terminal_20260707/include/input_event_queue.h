#pragma once

#include "input_mapper.h"

namespace input {

// Small cross-task queue used by keyboard drivers. Producers run in the A164
// polling path or USB host/client tasks; main.cpp drains events and writes the
// mapped bytes to the login UART.
bool beginEventQueue();
bool submitKeyEvent(const KeyEvent& event);
bool readKeyEvent(KeyEvent *event);

} // namespace input
