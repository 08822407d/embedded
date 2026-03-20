#pragma once

#include "app_types.hpp"

#include <unit_rolleri2c.hpp>

/*
RollerI2CDriver
===============
Thin wrapper around UnitRollerI2C.

Why wrap the vendor API:
- Keep hardware-specific details in one place.
- Keep main/control code independent from direct register calls.
- Centralize direction handling, output enable policy and readback format.
*/
class RollerI2CDriver {
public:
    // Initialize I2C communication and configure motor mode to CURRENT.
    bool begin();
    // Enable/disable motor output stage (equivalent to roller setOutput).
    void setOutput(bool enabled);
    // Force safe stop: zero command and disable output.
    void stop();
    // Send current command in controller units.
    // Final sign is multiplied by global gMotorDirection.
    void writeCurrentCommand(float current_cmd);
    // Read a compact set of telemetry required by control/safety.
    MotorFeedback readFeedback();
    // True when begin() succeeded.
    bool isOnline() const;

private:
    UnitRollerI2C roller_{};
    bool online_ = false;
    bool output_enabled_ = false;
    uint32_t feedback_seq_ = 0;
};
