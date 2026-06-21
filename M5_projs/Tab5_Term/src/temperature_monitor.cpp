#include "temperature_monitor.h"

#include "app_config.h"

#include <Arduino.h>
#include <M5Unified.h>

namespace temperature_monitor {
namespace {
TemperatureReadings latest_readings = {};
bool monitor_initialized = false;
bool update_seen = false;

TemperatureReading makeReading(TemperatureSource source)
{
    return {
        .source = source,
        .valid = false,
        .celsius = 0.0f,
        .updated_ms = 0,
        .sample_count = 0,
        .failure_count = 0,
    };
}

void resetReadings()
{
    latest_readings = {
        .updated_ms = 0,
        .imu = makeReading(TemperatureSource::Imu),
        .chip = makeReading(TemperatureSource::Chip),
    };
    update_seen = false;
}

bool plausibleCelsius(float celsius)
{
    return celsius > -60.0f && celsius < 150.0f;
}

bool readImuTemperature(float *celsius)
{
    if (celsius == nullptr || !M5.Imu.isEnabled()) {
        return false;
    }

    float value = 0.0f;
    if (!M5.Imu.getTemp(&value) || !plausibleCelsius(value)) {
        return false;
    }

    *celsius = value;
    return true;
}

bool readChipTemperature(float *celsius)
{
#if TEMPERATURE_MONITOR_ENABLE_CHIP_TEMP
    if (celsius == nullptr) {
        return false;
    }

    const float value = temperatureRead();
    if (!plausibleCelsius(value)) {
        return false;
    }

    *celsius = value;
    return true;
#else
    (void)celsius;
    return false;
#endif
}

void recordReading(TemperatureReading *reading, bool valid, float celsius, uint32_t now)
{
    if (reading == nullptr) {
        return;
    }

    reading->updated_ms = now;
    reading->valid = valid;
    if (valid) {
        reading->celsius = celsius;
        ++reading->sample_count;
    } else {
        reading->celsius = 0.0f;
        ++reading->failure_count;
    }
}
} // namespace

void begin()
{
    resetReadings();
    monitor_initialized = true;
    update(true);
}

void update(bool force)
{
    if (!monitor_initialized) {
        resetReadings();
        monitor_initialized = true;
    }

#if ENABLE_TEMPERATURE_MONITOR
    const uint32_t now = millis();
    if (!force
        && update_seen
        && now - latest_readings.updated_ms < TEMPERATURE_MONITOR_UPDATE_INTERVAL_MS) {
        return;
    }

    float celsius = 0.0f;
    latest_readings.updated_ms = now;
    update_seen = true;

    recordReading(
        &latest_readings.imu,
        readImuTemperature(&celsius),
        celsius,
        now);
    recordReading(
        &latest_readings.chip,
        readChipTemperature(&celsius),
        celsius,
        now);
#else
    (void)force;
#endif
}

const TemperatureReadings& snapshot()
{
    return latest_readings;
}

TemperatureReading reading(TemperatureSource source)
{
    switch (source) {
    case TemperatureSource::Imu:
        return latest_readings.imu;
    case TemperatureSource::Chip:
        return latest_readings.chip;
    default:
        return makeReading(source);
    }
}

TemperatureReading primaryReading()
{
    if (latest_readings.imu.valid) {
        return latest_readings.imu;
    }
    return latest_readings.chip;
}

const char *sourceLabel(TemperatureSource source)
{
    switch (source) {
    case TemperatureSource::Imu:
        return "IMU";
    case TemperatureSource::Chip:
        return "Chip";
    default:
        return "Unknown";
    }
}

float toFahrenheit(float celsius)
{
    return celsius * 9.0f / 5.0f + 32.0f;
}

float toKelvin(float celsius)
{
    return celsius + 273.15f;
}

} // namespace temperature_monitor
