#pragma once

#include <Arduino.h>

namespace login_uart {

enum class PersistenceAction : uint8_t {
    Keep,
    Save,
    Clear,
};

enum class ApplyResult : uint8_t {
    None,
    Applied,
    AppliedPersistenceFailed,
};

struct State {
    uint32_t active_baud;
    uint32_t persisted_baud;
    size_t rx_buffer_size;
    bool pending;
    uint32_t pending_baud;
    uint32_t pending_delay_ms;
};

struct ErrorStats {
    uint32_t break_errors;
    uint32_t buffer_full_errors;
    uint32_t fifo_overflow_errors;
    uint32_t frame_errors;
    uint32_t parity_errors;
    uint32_t other_errors;
};

bool begin();
HardwareSerial& serial();
bool isSupportedBaud(uint32_t baud);
uint32_t defaultBaud();
State state();
ErrorStats errorStats();
void resetErrorStats();
bool scheduleBaud(
    uint32_t baud,
    uint32_t delay_ms,
    PersistenceAction persistence);
ApplyResult update();

} // namespace login_uart
