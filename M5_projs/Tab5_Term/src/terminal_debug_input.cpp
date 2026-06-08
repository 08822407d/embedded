#include "terminal_debug_input.h"

#include <Arduino.h>

#include "app_config.h"
#include "terminal_core.h"

namespace terminal_debug {

void beginCdcInjection()
{
#if ENABLE_TERMINAL_CDC_INJECTION
    Serial.println("Terminal CDC injection enabled");
    Serial.println("USB CDC input is rendered locally and is not forwarded to login UART");
#endif
}

void drainCdcInjection()
{
#if ENABLE_TERMINAL_CDC_INJECTION
    size_t processed = 0;

    while (Serial.available() > 0 && processed < TERMINAL_CDC_INJECTION_BYTES_PER_LOOP) {
        const int value = Serial.read();
        if (value < 0) {
            break;
        }

        terminal::writeByte(static_cast<uint8_t>(value));
        ++processed;
    }
#endif
}

} // namespace terminal_debug
