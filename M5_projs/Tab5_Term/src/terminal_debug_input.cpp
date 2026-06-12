#include "terminal_debug_input.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "screen_capture.h"
#include "terminal_core.h"

namespace terminal_debug {
namespace {
#if ENABLE_TERMINAL_CDC_INJECTION && ENABLE_TERMINAL_STATE_DIAGNOSTICS
constexpr uint8_t kFramePrefix[] = {0x1b, ']', '7', '7', '7', ';'};
constexpr uint8_t kFrameTerminator = 0x07;
constexpr size_t kFrameCapacity = 96;
constexpr uint32_t kCandidateTimeoutMs = 500;
constexpr size_t kMaxRowTextBytes = 96 * 8;

uint8_t frame[kFrameCapacity];
size_t frameLength = 0;
uint32_t candidateStartedMs = 0;

void resetCandidate()
{
    frameLength = 0;
    candidateStartedMs = 0;
}

void flushCandidateToTerminal()
{
    if (frameLength > 0) {
        terminal::writeBytes(frame, frameLength);
    }
    resetCandidate();
}

void printHex(const char *text, size_t length)
{
    static char encoded[kMaxRowTextBytes * 2 + 1];
    static constexpr char kHex[] = "0123456789ABCDEF";
    const size_t boundedLength =
        length > kMaxRowTextBytes ? kMaxRowTextBytes : length;
    for (size_t index = 0; index < boundedLength; ++index) {
        const uint8_t value = static_cast<uint8_t>(text[index]);
        encoded[index * 2] = kHex[value >> 4];
        encoded[index * 2 + 1] = kHex[value & 0x0f];
    }
    encoded[boundedLength * 2] = '\0';
    Serial.print(encoded);
}

bool parseIndex(char *text, uint16_t limit, uint16_t *result)
{
    if (text == nullptr || result == nullptr) {
        return false;
    }
    char *end = nullptr;
    const unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value >= limit) {
        return false;
    }
    *result = static_cast<uint16_t>(value);
    return true;
}

void printState()
{
    terminal::TerminalStateSnapshot snapshot = {};
    if (!terminal::getStateSnapshot(&snapshot)) {
        Serial.println("TAB5SNAP ERR unavailable");
        return;
    }

    Serial.printf(
        "TAB5SNAP v=1 cols=%u rows=%u cursor=%u,%u saved=%u,%u "
        "scroll=%u,%u fg=%u bg=%u attrs=%u wrap=%u pending=%u "
        "origin=%u appcursor=%u appkeypad=%u paste=%u mouse=%u "
        "cursorvisible=%u alt=%u hash=%08lX\r\n",
        snapshot.columns,
        snapshot.rows,
        snapshot.cursor_column,
        snapshot.cursor_row,
        snapshot.saved_column,
        snapshot.saved_row,
        snapshot.scroll_top,
        snapshot.scroll_bottom,
        snapshot.current_foreground,
        snapshot.current_background,
        snapshot.current_attributes,
        snapshot.wrap_enabled ? 1 : 0,
        snapshot.wrap_pending ? 1 : 0,
        snapshot.origin_mode ? 1 : 0,
        snapshot.application_cursor_mode ? 1 : 0,
        snapshot.application_keypad_mode ? 1 : 0,
        snapshot.bracketed_paste_mode ? 1 : 0,
        snapshot.mouse_tracking_mode ? 1 : 0,
        snapshot.cursor_visible ? 1 : 0,
        snapshot.alternate_screen ? 1 : 0,
        static_cast<unsigned long>(snapshot.buffer_hash));
}

void printRow(uint16_t row)
{
    static char text[kMaxRowTextBytes + 1];
    const size_t length =
        terminal::copyRowText(row, text, sizeof(text), true);
    Serial.printf(
        "TAB5ROW row=%u hash=%08lX text=",
        row,
        static_cast<unsigned long>(terminal::rowHash(row)));
    printHex(text, length);
    Serial.println();
}

void printCell(uint16_t row, uint16_t column)
{
    terminal::TerminalCellSnapshot snapshot = {};
    if (!terminal::getCellSnapshot(row, column, &snapshot)) {
        Serial.println("TAB5CELL ERR unavailable");
        return;
    }

    Serial.printf("TAB5CELL row=%u col=%u text=", row, column);
    printHex(snapshot.text, strlen(snapshot.text));
    Serial.printf(
        " width=%u fg=%u bg=%u attrs=%u dec=%u\r\n",
        static_cast<unsigned>(snapshot.width),
        snapshot.foreground,
        snapshot.background,
        snapshot.attributes,
        snapshot.dec_special_graphics ? 1 : 0);
}

void handleFrame()
{
    const size_t prefixLength = sizeof(kFramePrefix);
    const size_t commandLength = frameLength - prefixLength;
    char command[kFrameCapacity] = {};
    memcpy(command, frame + prefixLength, commandLength);

    if (strcmp(command, "terminal-state?") == 0) {
        printState();
        return;
    }

#if ENABLE_SCREEN_CAPTURE_DIAGNOSTICS
    if (strcmp(command, "screen-capture?") == 0) {
        screen_capture::streamDisplay();
        return;
    }
#endif

    constexpr char kRowPrefix[] = "terminal-row=";
    if (strncmp(command, kRowPrefix, sizeof(kRowPrefix) - 1) == 0) {
        char *value = command + sizeof(kRowPrefix) - 1;
        char *question = strrchr(value, '?');
        if (question != nullptr && question[1] == '\0') {
            *question = '\0';
        }
        uint16_t row = 0;
        if (parseIndex(value, terminal::rows(), &row)) {
            printRow(row);
            return;
        }
        Serial.println("TAB5SNAP ERR invalid-row");
        return;
    }

    constexpr char kCellPrefix[] = "terminal-cell=";
    if (strncmp(command, kCellPrefix, sizeof(kCellPrefix) - 1) == 0) {
        char *rowText = command + sizeof(kCellPrefix) - 1;
        char *comma = strchr(rowText, ',');
        char *question = strrchr(rowText, '?');
        if (comma != nullptr && question != nullptr &&
            question[1] == '\0' && comma < question) {
            *comma = '\0';
            *question = '\0';
            uint16_t row = 0;
            uint16_t column = 0;
            if (parseIndex(rowText, terminal::rows(), &row) &&
                parseIndex(comma + 1, terminal::columns(), &column)) {
                printCell(row, column);
                return;
            }
        }
        Serial.println("TAB5SNAP ERR invalid-cell");
        return;
    }

    Serial.println("TAB5SNAP ERR unknown-command");
}

void processByte(uint8_t byte)
{
    if (frameLength == 0) {
        if (byte == kFramePrefix[0]) {
            frame[0] = byte;
            frameLength = 1;
            candidateStartedMs = millis();
        } else {
            terminal::writeByte(byte);
        }
        return;
    }

    candidateStartedMs = millis();
    if (frameLength < sizeof(kFramePrefix)) {
        frame[frameLength++] = byte;
        if (byte != kFramePrefix[frameLength - 1]) {
            flushCandidateToTerminal();
        }
        return;
    }

    if (byte == kFrameTerminator) {
        handleFrame();
        resetCandidate();
        return;
    }

    if (frameLength >= kFrameCapacity - 1) {
        flushCandidateToTerminal();
        terminal::writeByte(byte);
        return;
    }
    frame[frameLength++] = byte;
}
#endif
} // namespace

void beginCdcInjection()
{
#if ENABLE_TERMINAL_CDC_INJECTION
    Serial.println("Terminal CDC injection enabled");
    Serial.println("USB CDC input is rendered locally and is not forwarded to login UART");
#if ENABLE_TERMINAL_STATE_DIAGNOSTICS
    Serial.println("Terminal state diagnostics enabled: OSC 777 private commands");
#endif
#endif
}

void drainCdcInjection()
{
#if ENABLE_TERMINAL_CDC_INJECTION
#if ENABLE_TERMINAL_STATE_DIAGNOSTICS
    if (frameLength > 0 &&
        millis() - candidateStartedMs >= kCandidateTimeoutMs) {
        flushCandidateToTerminal();
    }
#endif
    size_t processed = 0;

    while (Serial.available() > 0 && processed < TERMINAL_CDC_INJECTION_BYTES_PER_LOOP) {
        const int value = Serial.read();
        if (value < 0) {
            break;
        }

#if ENABLE_TERMINAL_STATE_DIAGNOSTICS
        processByte(static_cast<uint8_t>(value));
#else
        terminal::writeByte(static_cast<uint8_t>(value));
#endif
        ++processed;
    }
#endif
}

} // namespace terminal_debug
