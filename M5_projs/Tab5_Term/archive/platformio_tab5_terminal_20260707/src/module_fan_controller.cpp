#include "module_fan_controller.h"

#include "app_config.h"
#include "temperature_monitor.h"

#include <Arduino.h>
#include <M5Unified.h>

namespace module_fan {
namespace {

State fan_state = {};

#if ENABLE_MODULE_FAN_CONTROLLER
constexpr uint8_t kFanAddress = MODULE_FAN_M5BUS_I2C_ADDRESS;
constexpr uint32_t kI2CFrequencyHz = MODULE_FAN_I2C_FREQUENCY_HZ;
constexpr uint32_t kProbeIntervalMs = MODULE_FAN_PROBE_INTERVAL_MS;
constexpr uint32_t kControlIntervalMs = MODULE_FAN_CONTROL_INTERVAL_MS;
constexpr uint8_t kMissedPollsToRelease = MODULE_FAN_MISSED_POLLS_TO_RELEASE;
constexpr uint8_t kSensorFailsafeDutyPercent = MODULE_FAN_SENSOR_FAILSAFE_DUTY_PERCENT;

constexpr uint8_t kRegControl = 0x00;
constexpr uint8_t kRegPwmFrequency = 0x10;
constexpr uint8_t kRegPwmDutyCycle = 0x20;
constexpr uint8_t kRegRpm = 0x30;
constexpr uint8_t kRegFirmwareVersion = 0xFE;
constexpr uint8_t kRegI2CAddress = 0xFF;

constexpr uint8_t kFanDisable = 0x00;
constexpr uint8_t kFanEnable = 0x01;
constexpr uint8_t kPwmFrequency24KHz = 0x02;
constexpr uint8_t kInvalidDutyPercent = 0xFF;

struct CoolingLevel {
    float enter_celsius;
    float exit_celsius;
    uint8_t duty_percent;
};

constexpr CoolingLevel kCoolingLevels[] = {
    // Raspberry Pi 5-style defaults: 50/60/67.5/75 C with 5 C hysteresis.
    {50.0f, 45.0f, 29},
    {60.0f, 55.0f, 49},
    {67.5f, 62.5f, 69},
    {75.0f, 70.0f, 98},
};

uint32_t last_probe_ms = 0;
uint32_t last_control_ms = 0;
uint8_t current_cooling_level = 0;
uint8_t last_written_duty_percent = kInvalidDutyPercent;
ControlMode requested_control_mode = ControlMode::Automatic;
uint8_t requested_manual_duty_percent = 0;

uint8_t clampDutyPercent(uint8_t duty_percent)
{
    return duty_percent > 100 ? 100 : duty_percent;
}

bool readFanRegister(uint8_t reg, uint8_t *data, size_t length)
{
    if (data == nullptr || length == 0 || !M5.In_I2C.isEnabled()) {
        return false;
    }
    return M5.In_I2C.readRegister(kFanAddress, reg, data, length, kI2CFrequencyHz);
}

bool readFanRegister8(uint8_t reg, uint8_t *value)
{
    return readFanRegister(reg, value, 1);
}

bool writeFanRegister8(uint8_t reg, uint8_t value)
{
    if (!M5.In_I2C.isEnabled()) {
        return false;
    }
    return M5.In_I2C.writeRegister8(kFanAddress, reg, value, kI2CFrequencyHz);
}

bool probeFan(uint8_t *firmware_version)
{
    if (!M5.In_I2C.isEnabled() || !M5.In_I2C.scanID(kFanAddress, kI2CFrequencyHz)) {
        return false;
    }

    uint8_t reported_address = 0;
    if (!readFanRegister8(kRegI2CAddress, &reported_address)
        || reported_address != kFanAddress) {
        return false;
    }

    uint8_t version = 0;
    if (!readFanRegister8(kRegFirmwareVersion, &version)) {
        return false;
    }
    if (firmware_version != nullptr) {
        *firmware_version = version;
    }
    return true;
}

bool readFanControlTemperature(float *temperature_celsius)
{
    if (temperature_celsius == nullptr) {
        return false;
    }

    temperature_monitor::update();

    const temperature_monitor::TemperatureReading imu_temperature =
        temperature_monitor::reading(temperature_monitor::TemperatureSource::Imu);
    if (imu_temperature.valid) {
        *temperature_celsius = imu_temperature.celsius;
        return true;
    }

#if MODULE_FAN_USE_CHIP_TEMP_FALLBACK
    const temperature_monitor::TemperatureReading chip_temperature =
        temperature_monitor::reading(temperature_monitor::TemperatureSource::Chip);
    if (chip_temperature.valid) {
        *temperature_celsius = chip_temperature.celsius;
        return true;
    }
#endif

    return false;
}

uint8_t selectCoolingLevel(float temperature_celsius)
{
    uint8_t next_level = current_cooling_level;
    constexpr uint8_t level_count =
        sizeof(kCoolingLevels) / sizeof(kCoolingLevels[0]);

    while (next_level < level_count
        && temperature_celsius >= kCoolingLevels[next_level].enter_celsius) {
        ++next_level;
    }

    while (next_level > 0
        && temperature_celsius < kCoolingLevels[next_level - 1].exit_celsius) {
        --next_level;
    }

    return next_level;
}

uint8_t dutyPercentForLevel(uint8_t cooling_level)
{
    if (cooling_level == 0) {
        return 0;
    }
    constexpr uint8_t level_count =
        sizeof(kCoolingLevels) / sizeof(kCoolingLevels[0]);
    if (cooling_level > level_count) {
        cooling_level = level_count;
    }
    return kCoolingLevels[cooling_level - 1].duty_percent;
}

bool applyFanDuty(uint8_t duty_percent)
{
    duty_percent = clampDutyPercent(duty_percent);
    if (duty_percent == last_written_duty_percent) {
        return true;
    }

    bool ok = true;
    if (duty_percent == 0) {
        ok = writeFanRegister8(kRegPwmDutyCycle, 0)
            && writeFanRegister8(kRegControl, kFanDisable);
    } else {
        ok = writeFanRegister8(kRegPwmDutyCycle, duty_percent)
            && writeFanRegister8(kRegControl, kFanEnable);
    }

    if (ok) {
        last_written_duty_percent = duty_percent;
        fan_state.duty_percent = duty_percent;
    }
    return ok;
}

void syncRequestedControlState()
{
    fan_state.control_mode = requested_control_mode;
    fan_state.manual_duty_percent = requested_manual_duty_percent;
}

void requestImmediateControlUpdate()
{
    last_control_ms = millis() - kControlIntervalMs;
}

bool readFanRpm(uint16_t *rpm)
{
    uint8_t data[2] = {};
    if (!readFanRegister(kRegRpm, data, sizeof(data))) {
        return false;
    }
    if (rpm != nullptr) {
        *rpm = static_cast<uint16_t>(data[0])
            | (static_cast<uint16_t>(data[1]) << 8);
    }
    return true;
}

void releaseFanState(const char *reason)
{
    if (fan_state.attached) {
        Serial.printf("[module-fan] released: %s\r\n", reason);
    }

    fan_state = {};
    syncRequestedControlState();
    current_cooling_level = 0;
    last_written_duty_percent = kInvalidDutyPercent;
}

bool attachFan(uint32_t now)
{
    uint8_t firmware_version = 0;
    if (!probeFan(&firmware_version)) {
        return false;
    }

    fan_state = {};
    fan_state.attached = true;
    fan_state.firmware_version = firmware_version;
    syncRequestedControlState();
    current_cooling_level = 0;
    last_written_duty_percent = kInvalidDutyPercent;

    bool configured = writeFanRegister8(kRegPwmFrequency, kPwmFrequency24KHz);
    const uint8_t initial_duty =
        requested_control_mode == ControlMode::Manual
        ? requested_manual_duty_percent
        : 0;
    configured = applyFanDuty(initial_duty) && configured;
    last_control_ms = now - kControlIntervalMs;

    Serial.printf(
        "[module-fan] attached addr=0x%02X fw=0x%02X configured=%s\r\n",
        kFanAddress,
        firmware_version,
        configured ? "yes" : "no");
    return true;
}

void updateAttachedFan(uint32_t now)
{
    if (now - last_control_ms < kControlIntervalMs) {
        return;
    }
    last_control_ms = now;

    float temperature_celsius = 0.0f;
    const bool temperature_valid = readFanControlTemperature(&temperature_celsius);
    fan_state.temperature_valid = temperature_valid;
    if (temperature_valid) {
        fan_state.temperature_celsius = temperature_celsius;
    }

    syncRequestedControlState();
    uint8_t target_duty = requested_manual_duty_percent;
    if (requested_control_mode == ControlMode::Automatic) {
        if (temperature_valid) {
            current_cooling_level = selectCoolingLevel(temperature_celsius);
        }
        target_duty = temperature_valid
            ? dutyPercentForLevel(current_cooling_level)
            : kSensorFailsafeDutyPercent;
    } else {
        current_cooling_level = 0;
    }

    fan_state.cooling_level = current_cooling_level;

    bool communication_ok = applyFanDuty(target_duty);
    uint16_t rpm = 0;
    communication_ok = readFanRpm(&rpm) && communication_ok;
    if (communication_ok) {
        fan_state.rpm = rpm;
        fan_state.missed_polls = 0;
        return;
    }

    if (fan_state.missed_polls < 0xFF) {
        ++fan_state.missed_polls;
    }
    if (fan_state.missed_polls >= kMissedPollsToRelease) {
        releaseFanState("communication lost");
    }
}
#endif

} // namespace

void begin()
{
#if ENABLE_MODULE_FAN_CONTROLLER
    requested_control_mode = ControlMode::Automatic;
    requested_manual_duty_percent = 0;
    releaseFanState("begin");
    last_probe_ms = 0;
    last_control_ms = 0;
    M5.Power.setExtOutput(true, m5::ext_PA);
#endif
}

void update()
{
#if ENABLE_MODULE_FAN_CONTROLLER
    const uint32_t now = millis();
    if (!fan_state.attached) {
        if (last_probe_ms == 0 || now - last_probe_ms >= kProbeIntervalMs) {
            last_probe_ms = now;
            attachFan(now);
        }
        return;
    }

    updateAttachedFan(now);
#endif
}

bool setManualDuty(uint8_t duty_percent)
{
#if ENABLE_MODULE_FAN_CONTROLLER
    if (duty_percent > 100) {
        return false;
    }

    requested_control_mode = ControlMode::Manual;
    requested_manual_duty_percent = duty_percent;
    syncRequestedControlState();
    requestImmediateControlUpdate();
    return true;
#else
    (void)duty_percent;
    return false;
#endif
}

void setAutomaticControl()
{
#if ENABLE_MODULE_FAN_CONTROLLER
    requested_control_mode = ControlMode::Automatic;
    requested_manual_duty_percent = 0;
    syncRequestedControlState();
    requestImmediateControlUpdate();
#endif
}

const State& state()
{
    return fan_state;
}

} // namespace module_fan
