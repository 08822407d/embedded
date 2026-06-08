#pragma once

#include "input_mapper.h"

namespace input {

bool beginEventQueue();
bool submitKeyEvent(const KeyEvent& event);
bool readKeyEvent(KeyEvent *event);

} // namespace input
