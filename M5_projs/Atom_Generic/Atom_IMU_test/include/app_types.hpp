#pragma once

#include <stdint.h>

// High-level runtime mode used by the main loop and LED/UI.
enum class AppMode : uint8_t {
    Disarmed = 0,  // Safe idle. Motor output is disabled.
    RefCapture,    // Manual upright hold. Capture the reference angle.
    Balancing,     // Closed-loop current control is active.
    Fault          // Latched fault. Requires user de-arm/re-arm.
};

// Fault reason for diagnostics/logging.
enum class FaultCode : uint8_t {
    None = 0,
    TiltExceeded,  // Body tilt error is outside safety bound.
    MotorError     // Roller reports internal error.
};

// Processed IMU sample shared between estimator, controller and state machine.
struct ImuSample {
    float roll_deg = 0.0f;       // Filtered body angle around balancing axis.
    float roll_rate_dps = 0.0f;  // Filtered body angular rate.
    uint32_t seq = 0;            // Monotonic update sequence for freshness checks.
    bool valid = false;          // False until first successful estimator update.
};

// Readback from Roller I2C registers.
struct MotorFeedback {
    int32_t speed_raw = 0;     // Speed readback register value (raw unit).
    int32_t current_raw = 0;   // Current readback register value (raw unit).
    uint8_t sys_status = 0;    // Roller system status register.
    uint8_t error_code = 0;    // Roller error code register.
    uint32_t seq = 0;          // Monotonic feedback sequence.
    bool valid = false;        // False if driver offline or read failed.
};
