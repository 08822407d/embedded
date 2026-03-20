#include "roller_i2c_driver.hpp"

#include <Wire.h>
#include <math.h>

#include "config.hpp"

/*
Driver strategy
---------------
This wrapper intentionally does not expose every Roller register.
It only provides the subset needed by first balancing bring-up:
- startup to CURRENT mode,
- output enable/disable,
- current command write,
- basic telemetry readback.

Keeping this layer narrow makes failures easier to diagnose.
*/

namespace {
// Local clamp utility to avoid dependency on std::clamp in embedded builds.
float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}
}  // namespace

bool RollerI2CDriver::begin() {
    // Open I2C bus and verify Roller is reachable at configured address.
    online_ = roller_.begin(&Wire, appcfg::kRollerI2cAddr, appcfg::kRollerSdaPin, appcfg::kRollerSclPin,
                            appcfg::kRollerI2cHz);
    if (!online_) {
        return false;
    }

    // Safety-first startup:
    // - keep output disabled,
    // - force current mode so controller writes map directly to torque/current command.
    roller_.setOutput(0);
    roller_.setMode(ROLLER_MODE_CURRENT);
    roller_.setOutput(0);
    output_enabled_ = false;
    return true;
}

void RollerI2CDriver::setOutput(bool enabled) {
    // Avoid duplicate writes and ignore calls when offline.
    if (!online_ || output_enabled_ == enabled) {
        return;
    }
    roller_.setOutput(enabled ? 1 : 0);
    output_enabled_ = enabled;
}

void RollerI2CDriver::stop() {
    if (!online_) {
        return;
    }
    // Explicit zero command before output disable reduces transient kick.
    roller_.setCurrent(0);
    setOutput(false);
}

void RollerI2CDriver::writeCurrentCommand(float current_cmd) {
    if (!online_) {
        return;
    }

    // Apply software clamp to protect from controller spikes.
    float limited = clampf(current_cmd, -appcfg::kCurrentCmdAbsMax, appcfg::kCurrentCmdAbsMax);
    // Convert from float controller output to Roller integer command.
    int32_t cmd = static_cast<int32_t>(lroundf(limited));
    // Global direction switch allows in-field reversal without touching controller signs.
    cmd *= gMotorDirection;
    roller_.setCurrent(cmd);
}

MotorFeedback RollerI2CDriver::readFeedback() {
    MotorFeedback fb{};
    if (!online_) {
        return fb;
    }

    // Keep readback small to limit I2C overhead in the main loop.
    fb.speed_raw = roller_.getSpeedReadback();
    fb.current_raw = roller_.getCurrentReadback();
    fb.sys_status = roller_.getSysStatus();
    fb.error_code = roller_.getErrorCode();
    fb.seq = ++feedback_seq_;
    fb.valid = true;
    return fb;
}

bool RollerI2CDriver::isOnline() const {
    return online_;
}
