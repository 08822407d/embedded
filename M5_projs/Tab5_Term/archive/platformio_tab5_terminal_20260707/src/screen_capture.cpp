#include "screen_capture.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <stdlib.h>

#include "app_config.h"
#include "display_orientation.h"

namespace screen_capture {
namespace {
#if ENABLE_SCREEN_CAPTURE_DIAGNOSTICS
constexpr uint32_t kWriteTimeoutMs = 5000;
constexpr uint32_t kCrcPolynomial = 0xEDB88320u;

uint32_t updateCrc32(uint32_t crc, const uint8_t *data, size_t length)
{
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1u) ? kCrcPolynomial : 0u);
        }
    }
    return crc;
}

bool writeAll(const uint8_t *data, size_t length)
{
    size_t offset = 0;
    uint32_t lastProgressMs = millis();
    while (offset < length) {
        const size_t written = Serial.write(data + offset, length - offset);
        if (written > 0) {
            offset += written;
            lastProgressMs = millis();
            continue;
        }
        if (millis() - lastProgressMs >= kWriteTimeoutMs) {
            return false;
        }
        delay(1);
    }
    return true;
}
#endif
} // namespace

bool streamDisplay()
{
#if !ENABLE_SCREEN_CAPTURE_DIAGNOSTICS
    return false;
#else
    const uint16_t width = static_cast<uint16_t>(M5.Display.width());
    const uint16_t height = static_cast<uint16_t>(M5.Display.height());
    const size_t rowBytes = static_cast<size_t>(width) * 2;
    auto *pixels = static_cast<lgfx::rgb565_t *>(
        malloc(static_cast<size_t>(width) * sizeof(lgfx::rgb565_t)));
    auto *encoded = static_cast<uint8_t *>(malloc(rowBytes));
    if (pixels == nullptr || encoded == nullptr) {
        free(pixels);
        free(encoded);
        Serial.println("TAB5SHOT ERR allocation");
        return false;
    }

    M5.Display.waitDisplay();
    Serial.printf(
        "TAB5SHOT BEGIN v=1 width=%u height=%u format=rgb565le "
        "bytes=%lu rotation=%u\r\n",
        width,
        height,
        static_cast<unsigned long>(rowBytes * height),
        static_cast<unsigned>(ui::displayRotation()));

    uint32_t crc = 0xFFFFFFFFu;
    bool success = true;
    for (uint16_t y = 0; y < height; ++y) {
        M5.Display.readRect(0, y, width, 1, pixels);
        for (uint16_t x = 0; x < width; ++x) {
            const uint16_t raw = pixels[x].raw;
            encoded[x * 2] = static_cast<uint8_t>(raw & 0xFFu);
            encoded[x * 2 + 1] = static_cast<uint8_t>(raw >> 8);
        }
        crc = updateCrc32(crc, encoded, rowBytes);
        if (!writeAll(encoded, rowBytes)) {
            success = false;
            break;
        }
    }

    free(encoded);
    free(pixels);
    if (!success) {
        return false;
    }

    crc ^= 0xFFFFFFFFu;
    Serial.printf("\r\nTAB5SHOT END crc32=%08lX\r\n", static_cast<unsigned long>(crc));
    Serial.flush();
    return true;
#endif
}

} // namespace screen_capture
