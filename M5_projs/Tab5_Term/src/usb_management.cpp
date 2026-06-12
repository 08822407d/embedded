#include "usb_management.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "login_uart.h"
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
        "TAB5CFG STATE active=%lu persisted=",
        static_cast<unsigned long>(state.active_baud));
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
