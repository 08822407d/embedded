#pragma once

#include <stdint.h>

namespace module_fan {

enum class ControlMode : uint8_t {
    Automatic = 0,
    Manual = 1,
};

struct State {
    bool attached;
    bool temperature_valid;
    float temperature_celsius;
    ControlMode control_mode;
    uint8_t cooling_level;
    uint8_t duty_percent;
    uint8_t manual_duty_percent;
    uint16_t rpm;
    uint8_t firmware_version;
    uint8_t missed_polls;
};

void begin();
void update();
bool setManualDuty(uint8_t duty_percent);
void setAutomaticControl();
const State& state();

} // namespace module_fan
