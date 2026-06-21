#include "usb_management.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "login_uart.h"
#include "module_fan_controller.h"
#include "render_pipeline_diagnostics.h"
#include "screen_capture.h"

namespace usb_management {
namespace {
constexpr uint8_t kFramePrefix[] = {0x1B, ']', '7', '7', '7', ';'};
constexpr uint8_t kFrameTerminator = 0x07;
constexpr size_t kFrameCapacity = 160;
constexpr size_t kMaxBytesPerLoop = 64;
constexpr uint32_t kCandidateTimeoutMs = 500;
constexpr uint32_t kMaxSwitchDelayMs = 10000;

uint8_t frame[kFrameCapacity];
size_t frameLength = 0;
uint32_t candidateStartedMs = 0;

void resetCandidate()
{
    frameLength = 0;
    candidateStartedMs = 0;
}

void flushCandidateToLoginUart()
{
    if (frameLength > 0) {
        login_uart::serial().write(frame, frameLength);
    }
    resetCandidate();
}

void printState()
{
    const login_uart::State state = login_uart::state();
    Serial.printf(
        "TAB5CFG STATE active=%lu rx_buffer=%u persisted=",
        static_cast<unsigned long>(state.active_baud),
        static_cast<unsigned>(state.rx_buffer_size));
    if (state.persisted_baud == 0) {
        Serial.print("default");
    } else {
        Serial.print(state.persisted_baud);
    }

    if (state.pending) {
        Serial.printf(
            " pending=%lu delay=%lu",
            static_cast<unsigned long>(state.pending_baud),
            static_cast<unsigned long>(state.pending_delay_ms));
    } else {
        Serial.print(" pending=none");
    }
    Serial.println(" supported=115200,230400,460800,921600");
}

void printRenderPipelineStats()
{
    const render_pipeline::Stats stats = render_pipeline::stats();
    const login_uart::State uartState = login_uart::state();
    const login_uart::ErrorStats errors = login_uart::errorStats();
    Serial.printf(
        "TAB5PIPE v=1 uart_read=%lu sw_buffered=%lu sw_dropped=%lu "
        "rendered=%lu mirrored=%lu batches=%lu mirror_enabled=%u "
        "sw_depth=%u sw_capacity=%u sw_max_depth=%u uart_rx_buffer=%u uart_break=%lu "
        "uart_buffer_full=%lu uart_fifo_ovf=%lu uart_frame=%lu "
        "uart_parity=%lu uart_other=%lu\r\n",
        static_cast<unsigned long>(stats.uart_read_bytes),
        static_cast<unsigned long>(stats.software_buffered_bytes),
        static_cast<unsigned long>(stats.software_dropped_bytes),
        static_cast<unsigned long>(stats.rendered_bytes),
        static_cast<unsigned long>(stats.mirrored_bytes),
        static_cast<unsigned long>(stats.render_batches),
        stats.mirror_enabled ? 1U : 0U,
        static_cast<unsigned>(stats.software_rx_depth),
        static_cast<unsigned>(stats.software_rx_capacity),
        static_cast<unsigned>(stats.software_rx_max_depth),
        static_cast<unsigned>(uartState.rx_buffer_size),
        static_cast<unsigned long>(errors.break_errors),
        static_cast<unsigned long>(errors.buffer_full_errors),
        static_cast<unsigned long>(errors.fifo_overflow_errors),
        static_cast<unsigned long>(errors.frame_errors),
        static_cast<unsigned long>(errors.parity_errors),
        static_cast<unsigned long>(errors.other_errors));
}

bool parseBooleanOption(const char *value, bool *result)
{
    if (strcmp(value, "0") == 0) {
        *result = false;
        return true;
    }
    if (strcmp(value, "1") == 0) {
        *result = true;
        return true;
    }
    return false;
}

const char *fanControlModeName(module_fan::ControlMode mode)
{
    switch (mode) {
    case module_fan::ControlMode::Manual:
        return "manual";
    case module_fan::ControlMode::Automatic:
    default:
        return "auto";
    }
}

int32_t temperatureDeciCelsius(const module_fan::State& state)
{
    if (!state.temperature_valid) {
        return 0;
    }

    const float scaled = state.temperature_celsius * 10.0f;
    return static_cast<int32_t>(scaled + (scaled >= 0.0f ? 0.5f : -0.5f));
}

void printModuleFanState()
{
    const module_fan::State& state = module_fan::state();
    Serial.printf(
        "TAB5FAN STATE attached=%u mode=%s duty=%u manual=%u rpm=%u "
        "fw=0x%02X missed=%u level=%u temp_valid=%u temp_deci_c=%ld\r\n",
        state.attached ? 1U : 0U,
        fanControlModeName(state.control_mode),
        static_cast<unsigned>(state.duty_percent),
        static_cast<unsigned>(state.manual_duty_percent),
        static_cast<unsigned>(state.rpm),
        static_cast<unsigned>(state.firmware_version),
        static_cast<unsigned>(state.missed_polls),
        static_cast<unsigned>(state.cooling_level),
        state.temperature_valid ? 1U : 0U,
        static_cast<long>(temperatureDeciCelsius(state)));
}

void handleFrame()
{
    const size_t prefixLength = sizeof(kFramePrefix);
    const size_t commandLength = frameLength - prefixLength;
    char command[kFrameCapacity] = {};
    memcpy(command, frame + prefixLength, commandLength);

    if (strcmp(command, "login-uart?") == 0) {
        printState();
        return;
    }

    if (strcmp(command, "render-pipeline?") == 0) {
        printRenderPipelineStats();
        return;
    }

    if (strcmp(command, "render-pipeline-reset") == 0) {
        render_pipeline::resetStats();
        login_uart::resetErrorStats();
        Serial.println("TAB5PIPE OK reset");
        return;
    }

    if (strcmp(command, "render-mirror?") == 0) {
        Serial.printf(
            "TAB5PIPE MIRROR enabled=%u\r\n",
            render_pipeline::mirrorEnabled() ? 1U : 0U);
        return;
    }

    constexpr char kMirrorPrefix[] = "render-mirror=";
    if (strncmp(command, kMirrorPrefix, sizeof(kMirrorPrefix) - 1) == 0) {
        bool enabled = false;
        if (!parseBooleanOption(command + sizeof(kMirrorPrefix) - 1, &enabled)) {
            Serial.println("TAB5PIPE ERR invalid-mirror");
            return;
        }
        render_pipeline::setMirrorEnabled(enabled);
        Serial.printf("TAB5PIPE MIRROR enabled=%u\r\n", enabled ? 1U : 0U);
        return;
    }

    if (strcmp(command, "module-fan?") == 0) {
        printModuleFanState();
        return;
    }

    constexpr char kFanPrefix[] = "module-fan=";
    if (strncmp(command, kFanPrefix, sizeof(kFanPrefix) - 1) == 0) {
        const char *value = command + sizeof(kFanPrefix) - 1;
        if (strcmp(value, "auto") == 0) {
            module_fan::setAutomaticControl();
            Serial.println("TAB5FAN OK mode=auto");
            printModuleFanState();
            return;
        }

        char *end = nullptr;
        const unsigned long parsedDuty = strtoul(value, &end, 10);
        if (end == value || *end != '\0' || parsedDuty > 100) {
            Serial.println("TAB5FAN ERR invalid-duty");
            return;
        }

        if (!module_fan::setManualDuty(static_cast<uint8_t>(parsedDuty))) {
            Serial.println("TAB5FAN ERR unavailable");
            return;
        }

        Serial.printf(
            "TAB5FAN OK mode=manual duty=%lu\r\n",
            parsedDuty);
        printModuleFanState();
        return;
    }

#if ENABLE_SCREEN_CAPTURE_DIAGNOSTICS
    if (strcmp(command, "screen-capture?") == 0) {
        screen_capture::streamDisplay();
        return;
    }
#endif

    constexpr char kSetPrefix[] = "login-uart=";
    if (strncmp(command, kSetPrefix, sizeof(kSetPrefix) - 1) != 0) {
        Serial.println("TAB5CFG ERR unknown-command");
        return;
    }

    char *value = command + sizeof(kSetPrefix) - 1;
    char *options = strchr(value, ',');
    if (options != nullptr) {
        *options = '\0';
        ++options;
    }

    bool useDefault = strcmp(value, "default") == 0;
    char *end = nullptr;
    const unsigned long parsedBaud =
        useDefault ? login_uart::defaultBaud() : strtoul(value, &end, 10);
    if ((!useDefault && (end == value || *end != '\0')) ||
        !login_uart::isSupportedBaud(parsedBaud)) {
        Serial.println("TAB5CFG ERR unsupported-baud");
        return;
    }

    bool persist = false;
    uint32_t delayMs = 0;
    char *save = nullptr;
    for (char *option =
             options == nullptr ? nullptr : strtok_r(options, ",", &save);
         option != nullptr;
         option = strtok_r(nullptr, ",", &save)) {
        if (strncmp(option, "persist=", 8) == 0) {
            if (!parseBooleanOption(option + 8, &persist)) {
                Serial.println("TAB5CFG ERR invalid-persist");
                return;
            }
        } else if (strncmp(option, "delay=", 6) == 0) {
            char *delayEnd = nullptr;
            const unsigned long parsedDelay =
                strtoul(option + 6, &delayEnd, 10);
            if (delayEnd == option + 6 || *delayEnd != '\0' ||
                parsedDelay > kMaxSwitchDelayMs) {
                Serial.println("TAB5CFG ERR invalid-delay");
                return;
            }
            delayMs = static_cast<uint32_t>(parsedDelay);
        } else {
            Serial.println("TAB5CFG ERR unknown-option");
            return;
        }
    }

    const login_uart::PersistenceAction persistence =
        useDefault
        ? login_uart::PersistenceAction::Clear
        : (persist
               ? login_uart::PersistenceAction::Save
               : login_uart::PersistenceAction::Keep);
    if (!login_uart::scheduleBaud(parsedBaud, delayMs, persistence)) {
        Serial.println("TAB5CFG ERR switch-pending");
        return;
    }

    Serial.printf(
        "TAB5CFG OK scheduled=%lu delay=%lu persistence=%s\r\n",
        parsedBaud,
        static_cast<unsigned long>(delayMs),
        useDefault ? "clear" : (persist ? "save" : "keep"));
}

void processByte(uint8_t byte)
{
    if (frameLength == 0) {
        if (byte == kFramePrefix[0]) {
            frame[0] = byte;
            frameLength = 1;
            candidateStartedMs = millis();
        } else {
            login_uart::serial().write(byte);
        }
        return;
    }

    candidateStartedMs = millis();

    if (frameLength < sizeof(kFramePrefix)) {
        frame[frameLength] = byte;
        ++frameLength;
        if (byte != kFramePrefix[frameLength - 1]) {
            flushCandidateToLoginUart();
        }
        return;
    }

    if (byte == kFrameTerminator) {
        handleFrame();
        resetCandidate();
        return;
    }

    if (frameLength >= kFrameCapacity - 1) {
        flushCandidateToLoginUart();
        login_uart::serial().write(byte);
        return;
    }

    frame[frameLength] = byte;
    ++frameLength;
}
} // namespace

void update()
{
    if (frameLength > 0 &&
        millis() - candidateStartedMs >= kCandidateTimeoutMs) {
        flushCandidateToLoginUart();
    }

    size_t processed = 0;
    while (Serial.available() > 0 && processed < kMaxBytesPerLoop) {
        const int value = Serial.read();
        if (value < 0) {
            break;
        }
        processByte(static_cast<uint8_t>(value));
        ++processed;
    }
}

} // namespace usb_management
