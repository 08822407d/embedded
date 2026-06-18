#include <Arduino.h>
#include <M5Unified.h>
#include <stdio.h>

#include "app_config.h"
#include "display_boot_guard.h"
#include "display_orientation.h"
#include "input_event_queue.h"
#include "input_mapper.h"
#include "login_uart.h"
#include "power_detect_probe.h"
#include "status_bar.h"
#include "tab5_keyboard_input.h"
#include "terminal_core.h"
#include "terminal_debug_input.h"
#include "ui_theme.h"
#include "usb_management.h"
#include "usb_keyboard_probe.h"

namespace {
constexpr size_t kMaxBytesPerLoop = 96;
constexpr size_t kMaxInputEventsPerLoop = 64;
constexpr uint32_t kM5UpdateIntervalMs = 20;

uint32_t lastM5UpdateMs = 0;

void formatStatusTitle(char *buffer, size_t buffer_size)
{
#if ENABLE_POWER_DETECT_PROBE
    snprintf(buffer, buffer_size, "Tab5 Power Detect Probe");
#elif ENABLE_TERMINAL_STATE_DIAGNOSTICS
    snprintf(buffer, buffer_size, "Tab5 Terminal Regression 115200");
#elif ENABLE_TERMINAL_CDC_INJECTION
    snprintf(buffer, buffer_size, "Tab5 Terminal CDC Inject 115200");
#elif ENABLE_USB_LOGIN_UART_BRIDGE
    snprintf(
        buffer,
        buffer_size,
        "Tab5 UART Bridge %lu 8N1 RX=G%d TX=G%d",
        static_cast<unsigned long>(login_uart::state().active_baud),
        LOGIN_UART_RX_PIN,
        LOGIN_UART_TX_PIN);
#else
    snprintf(
        buffer,
        buffer_size,
        "Tab5 UART Viewer %lu 8N1 RX=G%d",
        static_cast<unsigned long>(login_uart::state().active_baud),
        LOGIN_UART_RX_PIN);
#endif
}

void refreshStatusTitle()
{
    char title[64];
    formatStatusTitle(title, sizeof(title));
    ui::drawStatusBar(title);
}

void writeTerminalResponse(const uint8_t *data, size_t length)
{
    if (data == nullptr || length == 0) {
        return;
    }

#if ENABLE_TERMINAL_CDC_INJECTION
    Serial.write(data, length);
#else
    login_uart::serial().write(data, length);
#endif
}

void setupDisplay()
{
    const auto& theme = ui::terminalTheme();
    char title[64];
    formatStatusTitle(title, sizeof(title));

    ui::applyConfiguredDisplayOrientation();
    const int32_t availableTerminalHeight =
        M5.Display.height() - STATUS_BAR_HEIGHT;
    const int32_t terminalHeight =
        TERMINAL_ROWS * TERMINAL_CELL_HEIGHT;
    const int32_t centeredMargin =
        (availableTerminalHeight - terminalHeight) / 2;
    const int32_t minimumMargin =
        TERMINAL_VERTICAL_MARGIN_ROWS * TERMINAL_CELL_HEIGHT;
    const int32_t terminalMargin =
        centeredMargin > minimumMargin ? centeredMargin : minimumMargin;
    const int32_t terminalY = STATUS_BAR_HEIGHT + terminalMargin;
    ui::beginStatusBar(theme);
    M5.Display.fillScreen(theme.screen_background);
    ui::drawStatusBar(title);
    terminal::begin({
        .x = 0,
        .y = terminalY,
        .width = M5.Display.width(),
        .height = terminalHeight,
        .max_columns = TERMINAL_COLUMNS,
        .max_rows = TERMINAL_ROWS,
        .text_size = TERMINAL_TEXT_SIZE,
        .theme = &theme,
        .response_writer = writeTerminalResponse,
    });
}

void readLoginUartToDisplayAndDebug()
{
#if !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    HardwareSerial& serial = login_uart::serial();
    uint8_t buffer[kMaxBytesPerLoop];
    size_t length = 0;

    // Keep each loop bounded so status refresh, USB tasks, keyboard events, and
    // M5.update() continue to run even during large UART bursts.
    while (serial.available() > 0 && length < kMaxBytesPerLoop) {
        const int value = serial.read();
        if (value < 0) {
            break;
        }

        buffer[length++] = static_cast<uint8_t>(value);
    }

    if (length > 0) {
        terminal::writeBytes(buffer, length);
        Serial.write(buffer, length);
    }
#endif
}

void bridgeUsbToLoginUart()
{
#if ENABLE_USB_LOGIN_UART_BRIDGE && !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    usb_management::update();
#endif
}

void bridgeInputEventsToLoginUart()
{
#if (ENABLE_TAB5_KEYBOARD || ENABLE_USB_KEYBOARD_PROBE) && !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    input::KeyEvent event;
    size_t processed = 0;

    while (processed < kMaxInputEventsPerLoop && input::readKeyEvent(&event)) {
        const input::TerminalModes modes = {
            .application_cursor = terminal::applicationCursorMode(),
            .application_keypad = terminal::applicationKeypadMode(),
        };
        const input::EncodedInput encoded = input::mapKeyEvent(event, modes);
        if (encoded.length > 0) {
            login_uart::serial().write(encoded.bytes, encoded.length);
        }
        ++processed;
    }
#endif
}
} // namespace

void setup()
{
    Serial.begin(USB_DEBUG_BAUD);
    delay(300);

    auto cfg = M5.config();
    M5.begin(cfg);
    display_boot_guard::restartIfDisplayUnusable();
#if ENABLE_USB_KEYBOARD_PROBE && !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    M5.Power.setExtOutput(true, m5::ext_USB);
#endif

#if !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    login_uart::begin();
#endif
    setupDisplay();

    Serial.println();
    Serial.println("Tab5 minimal UART viewer started");
#if ENABLE_USB_LOGIN_UART_BRIDGE && !ENABLE_TERMINAL_CDC_INJECTION
    Serial.println("USB to login UART bridge enabled");
#endif
#if ENABLE_TERMINAL_CDC_INJECTION
    terminal_debug::beginCdcInjection();
#endif
#if ENABLE_POWER_DETECT_PROBE
    power_detect_probe::begin();
#endif
#if (ENABLE_TAB5_KEYBOARD || ENABLE_USB_KEYBOARD_PROBE) && !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    const bool inputQueueReady = input::beginEventQueue();
    if (!inputQueueReady) {
        Serial.println("[input] input event queue allocation failed");
    }
#endif
#if ENABLE_USB_KEYBOARD_PROBE && !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    Serial.println("USB keyboard probe enabled");
    if (inputQueueReady) {
        usbKeyboardProbeBegin();
    }
#endif
#if ENABLE_TAB5_KEYBOARD && !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    if (inputQueueReady) {
        tab5KeyboardInputBegin();
    }
#endif
#if !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    const login_uart::State uartState = login_uart::state();
    Serial.printf(
        "Login UART: baud=%lu persisted=%s 8N1 RX=G%d TX=G%d\r\n",
        static_cast<unsigned long>(uartState.active_baud),
        uartState.persisted_baud == 0 ? "default" : "saved",
        LOGIN_UART_RX_PIN,
        LOGIN_UART_TX_PIN);
#endif
}

void loop()
{
#if !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    const login_uart::ApplyResult baudResult = login_uart::update();
    if (baudResult != login_uart::ApplyResult::None) {
        refreshStatusTitle();
        const login_uart::State state = login_uart::state();
        Serial.printf(
            "TAB5CFG OK active=%lu persisted=%s%s\r\n",
            static_cast<unsigned long>(state.active_baud),
            state.persisted_baud == 0 ? "default" : "saved",
            baudResult == login_uart::ApplyResult::AppliedPersistenceFailed
                ? " persistence-error"
                : "");
    }
#endif
    readLoginUartToDisplayAndDebug();
    terminal_debug::drainCdcInjection();
    power_detect_probe::update();
    bridgeUsbToLoginUart();
    tab5KeyboardInputUpdate();
    bridgeInputEventsToLoginUart();
#if !ENABLE_POWER_DETECT_PROBE
    ui::refreshBatteryStatus(false);
#endif

    const uint32_t now = millis();
    if (now - lastM5UpdateMs >= kM5UpdateIntervalMs) {
        M5.update();
        lastM5UpdateMs = now;
    }

    delay(1);
}
