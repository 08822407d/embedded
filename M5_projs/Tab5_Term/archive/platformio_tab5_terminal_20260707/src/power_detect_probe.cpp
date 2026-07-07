#include "power_detect_probe.h"

#if ENABLE_POWER_DETECT_PROBE

#include <Arduino.h>
#include <M5Unified.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace power_detect_probe {
namespace {
constexpr uint32_t kSampleIntervalMs = 500;
constexpr size_t kSampleCapacity = 4096;
constexpr size_t kCommandBufferSize = 48;

struct PowerSample {
    uint32_t sequence;
    uint32_t millis;
    int32_t current_ma;
    int16_t voltage_mv;
    int16_t battery_level;
    uint8_t flags;
};

constexpr uint8_t kFlagApiCharging = 1 << 0;
constexpr uint8_t kFlagRawPinHigh = 1 << 1;
constexpr uint8_t kFlagSerialConnected = 1 << 2;

PowerSample samples[kSampleCapacity];
size_t next_index = 0;
size_t sample_count = 0;
uint32_t total_samples = 0;
uint32_t dropped_samples = 0;
uint32_t last_sample_ms = 0;
char command_buffer[kCommandBufferSize] = {};
size_t command_length = 0;

bool apiCharging()
{
    return M5.Power.isCharging() == m5::Power_Class::is_charging;
}

bool rawPinHigh()
{
    return M5.getIOExpander(1).digitalRead(6);
}

int16_t batteryLevel()
{
    const int32_t level = M5.Power.getBatteryLevel();
    if (level < 0) {
        return -1;
    }
    if (level > 100) {
        return 100;
    }
    return static_cast<int16_t>(level);
}

PowerSample readSample(uint32_t now)
{
    uint8_t flags = 0;
    if (apiCharging()) {
        flags |= kFlagApiCharging;
    }
    if (rawPinHigh()) {
        flags |= kFlagRawPinHigh;
    }
    if (Serial) {
        flags |= kFlagSerialConnected;
    }

    return {
        .sequence = total_samples,
        .millis = now,
        .current_ma = M5.Power.getBatteryCurrent(),
        .voltage_mv = M5.Power.getBatteryVoltage(),
        .battery_level = batteryLevel(),
        .flags = flags,
    };
}

void recordSample(uint32_t now)
{
    samples[next_index] = readSample(now);
    next_index = (next_index + 1) % kSampleCapacity;
    if (sample_count < kSampleCapacity) {
        ++sample_count;
    } else {
        ++dropped_samples;
    }
    ++total_samples;
}

void clearSamples()
{
    next_index = 0;
    sample_count = 0;
    total_samples = 0;
    dropped_samples = 0;
    last_sample_ms = 0;
}

size_t oldestIndex()
{
    if (sample_count < kSampleCapacity) {
        return 0;
    }
    return next_index;
}

void printHelp()
{
    Serial.println("PWRLOG HELP commands: PWRLOG?, PWRLOG DUMP, PWRLOG CLEAR");
}

void printStatus()
{
    Serial.printf(
        "PWRLOG STATUS version=1 sample_ms=%lu capacity=%u count=%u total=%lu dropped=%lu\r\n",
        static_cast<unsigned long>(kSampleIntervalMs),
        static_cast<unsigned>(kSampleCapacity),
        static_cast<unsigned>(sample_count),
        static_cast<unsigned long>(total_samples),
        static_cast<unsigned long>(dropped_samples));
}

void printSampleCsvHeader()
{
    Serial.println("seq,ms,api,pin,usb,current_ma,voltage_mv,battery_level");
}

void printSampleCsv(const PowerSample& sample)
{
    Serial.printf(
        "%lu,%lu,%u,%u,%u,%ld,%d,%d\r\n",
        static_cast<unsigned long>(sample.sequence),
        static_cast<unsigned long>(sample.millis),
        (sample.flags & kFlagApiCharging) ? 1 : 0,
        (sample.flags & kFlagRawPinHigh) ? 1 : 0,
        (sample.flags & kFlagSerialConnected) ? 1 : 0,
        static_cast<long>(sample.current_ma),
        static_cast<int>(sample.voltage_mv),
        static_cast<int>(sample.battery_level));
}

void dumpSamples()
{
    printStatus();
    Serial.println("PWRLOG BEGIN");
    printSampleCsvHeader();

    const size_t first = oldestIndex();
    for (size_t offset = 0; offset < sample_count; ++offset) {
        const size_t index = (first + offset) % kSampleCapacity;
        printSampleCsv(samples[index]);
        if ((offset & 0x3f) == 0x3f) {
            delay(1);
        }
    }

    Serial.println("PWRLOG END");
}

void handleCommand(const char *command)
{
    if (strcmp(command, "PWRLOG?") == 0 || strcmp(command, "PWRLOG STATUS") == 0) {
        printStatus();
    } else if (strcmp(command, "PWRLOG DUMP") == 0) {
        dumpSamples();
    } else if (strcmp(command, "PWRLOG CLEAR") == 0) {
        clearSamples();
        Serial.println("PWRLOG CLEARED");
    } else if (strcmp(command, "PWRLOG HELP") == 0) {
        printHelp();
    } else if (command[0] != '\0') {
        Serial.printf("PWRLOG ERROR unknown-command=%s\r\n", command);
    }
}

void processCommandByte(uint8_t byte)
{
    if (byte == '\r' || byte == '\n') {
        command_buffer[command_length] = '\0';
        handleCommand(command_buffer);
        command_length = 0;
        command_buffer[0] = '\0';
        return;
    }

    if (command_length + 1 < sizeof(command_buffer)) {
        command_buffer[command_length++] = static_cast<char>(byte);
        command_buffer[command_length] = '\0';
    } else {
        command_length = 0;
        command_buffer[0] = '\0';
        Serial.println("PWRLOG ERROR command-too-long");
    }
}

void drainCommands()
{
    while (Serial.available() > 0) {
        const int value = Serial.read();
        if (value >= 0) {
            processCommandByte(static_cast<uint8_t>(value));
        }
    }
}
} // namespace

void begin()
{
    const uint32_t now = millis();
    clearSamples();
    last_sample_ms = now;
    recordSample(now);
    Serial.println("PWRLOG READY sample_ms=500 capacity=4096");
    printHelp();
}

void update()
{
    const uint32_t now = millis();
    if (last_sample_ms == 0 || now - last_sample_ms >= kSampleIntervalMs) {
        last_sample_ms = now;
        recordSample(now);
    }
    drainCommands();
}

} // namespace power_detect_probe

#endif
