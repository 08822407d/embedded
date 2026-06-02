#include <Arduino.h>
#include <M5Unified.h>

#include "app_config.h"

HardwareSerial LoginSerial(1);

namespace {
constexpr uint16_t kBackgroundColor = TFT_BLACK;
constexpr uint16_t kStatusBackgroundColor = TFT_DARKGREEN;
constexpr uint16_t kStatusTextColor = TFT_WHITE;
constexpr uint16_t kTerminalTextColor = TFT_GREEN;
constexpr size_t kMaxBytesPerLoop = 96;
constexpr uint32_t kM5UpdateIntervalMs = 20;

uint32_t lastM5UpdateMs = 0;

void drawStatusBar()
{
    const int32_t width = M5.Display.width();

    M5.Display.fillRect(0, 0, width, STATUS_BAR_HEIGHT, kStatusBackgroundColor);
    M5.Display.setTextColor(kStatusTextColor, kStatusBackgroundColor);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(4, 10);
    M5.Display.print("Tab5 UART Viewer 115200 8N1 RX=G38");
}

void setupDisplay()
{
    M5.Display.setRotation(SCREEN_ROTATION);
    M5.Display.fillScreen(kBackgroundColor);
    drawStatusBar();

    M5.Display.setTextColor(kTerminalTextColor, kBackgroundColor);
    M5.Display.setTextSize(TERMINAL_TEXT_SIZE);
    M5.Display.setTextScroll(true);
    M5.Display.setScrollRect(
        0,
        STATUS_BAR_HEIGHT,
        M5.Display.width(),
        M5.Display.height() - STATUS_BAR_HEIGHT,
        kBackgroundColor);
    M5.Display.setCursor(0, STATUS_BAR_HEIGHT);
}

void writeToDisplay(uint8_t byte)
{
    switch (byte) {
    case '\r':
        M5.Display.setCursor(0, M5.Display.getCursorY());
        break;
    case '\n':
        M5.Display.write('\n');
        break;
    case '\b':
    case 0x7f:
        break;
    case '\t':
        M5.Display.print("    ");
        break;
    default:
        if (byte >= 0x20 && byte <= 0x7e) {
            M5.Display.write(byte);
        }
        break;
    }
}

void writeToDebug(uint8_t byte)
{
    Serial.write(byte);
}
} // namespace

void setup()
{
    Serial.begin(USB_DEBUG_BAUD);
    delay(300);

    auto cfg = M5.config();
    M5.begin(cfg);
    setupDisplay();

    LoginSerial.setRxBufferSize(8192);
    LoginSerial.begin(LOGIN_UART_BAUD, SERIAL_8N1, LOGIN_UART_RX_PIN, LOGIN_UART_TX_PIN);

    Serial.println();
    Serial.println("Tab5 minimal UART viewer started");
    Serial.printf(
        "Login UART: baud=%u 8N1 RX=G%d TX=G%d\r\n",
        LOGIN_UART_BAUD,
        LOGIN_UART_RX_PIN,
        LOGIN_UART_TX_PIN);
}

void loop()
{
    size_t processed = 0;

    while (LoginSerial.available() > 0 && processed < kMaxBytesPerLoop) {
        const int value = LoginSerial.read();
        if (value < 0) {
            break;
        }

        const auto byte = static_cast<uint8_t>(value);
        writeToDisplay(byte);
        writeToDebug(byte);
        ++processed;
    }

    const uint32_t now = millis();
    if (now - lastM5UpdateMs >= kM5UpdateIntervalMs) {
        M5.update();
        lastM5UpdateMs = now;
    }

    delay(1);
}
