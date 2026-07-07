#pragma once

#include "app_config.h"

#if ENABLE_POWER_DETECT_PROBE
namespace power_detect_probe {

void begin();
void update();

} // namespace power_detect_probe
#else
namespace power_detect_probe {

inline void begin() {}
inline void update() {}

} // namespace power_detect_probe
#endif
