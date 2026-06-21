#pragma once

#include <stdint.h>

namespace temperature_monitor {

enum class TemperatureSource : uint8_t {
    Imu = 0,
    Chip = 1,
};

struct TemperatureReading {
    TemperatureSource source;
    bool valid;
    float celsius;
    uint32_t updated_ms;
    uint32_t sample_count;
    uint32_t failure_count;
};

struct TemperatureReadings {
    uint32_t updated_ms;
    TemperatureReading imu;
    TemperatureReading chip;
};

void begin();
void update(bool force = false);
const TemperatureReadings& snapshot();
TemperatureReading reading(TemperatureSource source);
TemperatureReading primaryReading();
const char *sourceLabel(TemperatureSource source);
float toFahrenheit(float celsius);
float toKelvin(float celsius);

} // namespace temperature_monitor
