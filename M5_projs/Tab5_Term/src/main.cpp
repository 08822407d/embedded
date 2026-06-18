#include <Arduino.h>
#include <M5Unified.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "display_boot_guard.h"
#include "display_orientation.h"
#include "input_event_queue.h"
#include "input_mapper.h"
#include "login_uart.h"
#include "power_detect_probe.h"
#include "render_pipeline_diagnostics.h"
#include "status_bar.h"
#include "tab5_keyboard_input.h"
#include "terminal_core.h"
#include "terminal_debug_input.h"
#include "ui_theme.h"
#include "usb_management.h"
#include "usb_keyboard_probe.h"

namespace {
constexpr size_t kMaxBytesPerLoop = LOGIN_UART_RENDER_BYTES_PER_LOOP;
static_assert(kMaxBytesPerLoop > 0, "LOGIN_UART_RENDER_BYTES_PER_LOOP must be positive");
constexpr size_t kMaxUartDrainBytesPerLoop = 4096;
constexpr size_t kUartDrainChunkBytes = 256;
constexpr size_t kMaxInputEventsPerLoop = 64;
constexpr uint32_t kM5UpdateIntervalMs = 20;

uint32_t lastM5UpdateMs = 0;
portMUX_TYPE loginRxMux = portMUX_INITIALIZER_UNLOCKED;
uint8_t loginRxBuffer[LOGIN_UART_SOFTWARE_RX_BUFFER_SIZE];
size_t loginRxHead = 0;
size_t loginRxTail = 0;
size_t loginRxCount = 0;
uint32_t loginUartReadBytes = 0;
uint32_t loginRxBufferedBytes = 0;
uint32_t loginRxDroppedBytes = 0;
uint32_t loginRenderedBytes = 0;
uint32_t loginMirroredBytes = 0;
uint32_t loginRenderBatches = 0;
size_t loginRxMaxDepth = 0;
bool loginRxDrainTaskStarted = false;
bool loginMirrorEnabled = true;
bool loginRenderTransactionActive = false;
uint32_t loginRenderTransactionStartMs = 0;

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

void pushLoginRxByteLocked(uint8_t value)
{
    if (loginRxCount >= sizeof(loginRxBuffer)) {
        ++loginRxDroppedBytes;
        return;
    }

    loginRxBuffer[loginRxHead] = value;
    loginRxHead = (loginRxHead + 1) % sizeof(loginRxBuffer);
    ++loginRxCount;
    ++loginRxBufferedBytes;
    if (loginRxCount > loginRxMaxDepth) {
        loginRxMaxDepth = loginRxCount;
    }
}

void pushLoginRxBytes(const uint8_t *data, size_t length)
{
    if (data == nullptr || length == 0) {
        return;
    }

    portENTER_CRITICAL(&loginRxMux);
    for (size_t index = 0; index < length; ++index) {
        pushLoginRxByteLocked(data[index]);
    }
    portEXIT_CRITICAL(&loginRxMux);
}

size_t popLoginRxBytes(uint8_t *buffer, size_t capacity)
{
    if (buffer == nullptr || capacity == 0) {
        return 0;
    }

    size_t length = 0;
    portENTER_CRITICAL(&loginRxMux);
    while (loginRxCount > 0 && length < capacity) {
        buffer[length++] = loginRxBuffer[loginRxTail];
        loginRxTail = (loginRxTail + 1) % sizeof(loginRxBuffer);
        --loginRxCount;
    }
    portEXIT_CRITICAL(&loginRxMux);
    return length;
}

size_t loginRxDepth()
{
    portENTER_CRITICAL(&loginRxMux);
    const size_t depth = loginRxCount;
    portEXIT_CRITICAL(&loginRxMux);
    return depth;
}

void drainLoginUartToSoftwareBuffer(HardwareSerial& serial)
{
    size_t drained = 0;
    uint8_t buffer[kUartDrainChunkBytes];
    while (serial.available() > 0 && drained < kMaxUartDrainBytesPerLoop) {
        size_t length = 0;
        while (
            length < sizeof(buffer) &&
            drained + length < kMaxUartDrainBytesPerLoop &&
            serial.available() > 0) {
            const int value = serial.read();
            if (value < 0) {
                break;
            }
            buffer[length++] = static_cast<uint8_t>(value);
        }

        if (length == 0) {
            break;
        }

        pushLoginRxBytes(buffer, length);
        portENTER_CRITICAL(&loginRxMux);
        loginUartReadBytes += static_cast<uint32_t>(length);
        portEXIT_CRITICAL(&loginRxMux);
        drained += length;
    }
}

void loginUartReceiveCallback()
{
    drainLoginUartToSoftwareBuffer(login_uart::serial());
}

void startLoginUartAsyncDrain()
{
#if !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    login_uart::serial().onReceive(loginUartReceiveCallback, false);
    loginRxDrainTaskStarted = true;
#endif
}

size_t mirrorLoginBytesToDebug(const uint8_t *data, size_t length)
{
    if (data == nullptr || length == 0) {
        return 0;
    }
    if (!render_pipeline::mirrorEnabled()) {
        return 0;
    }

    static uint8_t mirrorBuffer[kMaxBytesPerLoop];
    const size_t boundedLength =
        length > sizeof(mirrorBuffer) ? sizeof(mirrorBuffer) : length;
    memcpy(mirrorBuffer, data, boundedLength);
    const size_t written = Serial.write(mirrorBuffer, boundedLength);
    Serial.flush();
    return written;
}

void beginLoginRenderTransactionIfNeeded()
{
#if LOGIN_UART_DEFER_RENDER_WHILE_BACKLOG
    if (loginRenderTransactionActive) {
        return;
    }
    terminal::beginWriteTransaction();
    loginRenderTransactionActive = true;
    loginRenderTransactionStartMs = millis();
#endif
}

void finishLoginRenderTransactionIfNeeded()
{
#if LOGIN_UART_DEFER_RENDER_WHILE_BACKLOG
    if (!loginRenderTransactionActive) {
        return;
    }
    terminal::endWriteTransaction();
    loginRenderTransactionActive = false;
#endif
}

bool loginRenderTransactionExpired()
{
#if LOGIN_UART_DEFER_RENDER_WHILE_BACKLOG
    return loginRenderTransactionActive
        && millis() - loginRenderTransactionStartMs >= LOGIN_UART_RENDER_DEFER_MAX_MS;
#else
    return false;
#endif
}

void readLoginUartToDisplayAndDebug()
{
#if !ENABLE_TERMINAL_CDC_INJECTION && !ENABLE_POWER_DETECT_PROBE
    HardwareSerial& serial = login_uart::serial();
    uint8_t buffer[kMaxBytesPerLoop];

    if (!loginRxDrainTaskStarted) {
        drainLoginUartToSoftwareBuffer(serial);
    }

    // Keep rendering bounded so status refresh, USB tasks, keyboard events, and
    // M5.update() continue to run even during large UART bursts. The software
    // RX buffer above drains the hardware UART quickly before rendering starts.
    const size_t length = popLoginRxBytes(buffer, sizeof(buffer));
    if (length == 0) {
        finishLoginRenderTransactionIfNeeded();
        return;
    }

#if LOGIN_UART_DEFER_RENDER_WHILE_BACKLOG
    if (loginRxDepth() > 0) {
        beginLoginRenderTransactionIfNeeded();
    }
#endif

    if (length > 0) {
        terminal::writeBytes(buffer, length);
#if LOGIN_UART_DEFER_RENDER_WHILE_BACKLOG
        const bool shouldFinishTransaction =
            loginRenderTransactionActive &&
            (loginRxDepth() == 0 || loginRenderTransactionExpired());
        if (shouldFinishTransaction) {
            finishLoginRenderTransactionIfNeeded();
        }
#endif
        portENTER_CRITICAL(&loginRxMux);
        loginRenderedBytes += static_cast<uint32_t>(length);
        ++loginRenderBatches;
        portEXIT_CRITICAL(&loginRxMux);
        const size_t mirrored = mirrorLoginBytesToDebug(buffer, length);
        portENTER_CRITICAL(&loginRxMux);
        loginMirroredBytes += static_cast<uint32_t>(mirrored);
        portEXIT_CRITICAL(&loginRxMux);
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

namespace render_pipeline {

Stats stats()
{
    portENTER_CRITICAL(&loginRxMux);
    const Stats snapshot = {
        .uart_read_bytes = loginUartReadBytes,
        .software_buffered_bytes = loginRxBufferedBytes,
        .software_dropped_bytes = loginRxDroppedBytes,
        .rendered_bytes = loginRenderedBytes,
        .mirrored_bytes = loginMirroredBytes,
        .render_batches = loginRenderBatches,
        .mirror_enabled = loginMirrorEnabled,
        .software_rx_depth = loginRxCount,
        .software_rx_capacity = sizeof(loginRxBuffer),
        .software_rx_max_depth = loginRxMaxDepth,
    };
    portEXIT_CRITICAL(&loginRxMux);
    return snapshot;
}

void resetStats()
{
    portENTER_CRITICAL(&loginRxMux);
    loginUartReadBytes = 0;
    loginRxBufferedBytes = 0;
    loginRxDroppedBytes = 0;
    loginRenderedBytes = 0;
    loginMirroredBytes = 0;
    loginRenderBatches = 0;
    loginRxMaxDepth = loginRxCount;
    portEXIT_CRITICAL(&loginRxMux);
}

bool mirrorEnabled()
{
    portENTER_CRITICAL(&loginRxMux);
    const bool enabled = loginMirrorEnabled;
    portEXIT_CRITICAL(&loginRxMux);
    return enabled;
}

void setMirrorEnabled(bool enabled)
{
    portENTER_CRITICAL(&loginRxMux);
    loginMirrorEnabled = enabled;
    portEXIT_CRITICAL(&loginRxMux);
}

} // namespace render_pipeline

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
    startLoginUartAsyncDrain();
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
        "Login UART: baud=%lu persisted=%s rx_buffer=%u 8N1 RX=G%d TX=G%d\r\n",
        static_cast<unsigned long>(uartState.active_baud),
        uartState.persisted_baud == 0 ? "default" : "saved",
        static_cast<unsigned>(uartState.rx_buffer_size),
        LOGIN_UART_RX_PIN,
        LOGIN_UART_TX_PIN);
    Serial.printf(
        "Login UART async RX drain: %s\r\n",
        loginRxDrainTaskStarted ? "enabled" : "loop-fallback");
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
