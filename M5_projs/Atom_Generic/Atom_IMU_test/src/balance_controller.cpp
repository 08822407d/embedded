#include "balance_controller.hpp"

#include "config.hpp"

/*
Control rationale
-----------------
Reaction-wheel balancing needs fast corrective torque around one axis.

Terms used:
- P: push back proportional to tilt error.
- D: dampen motion using body angular rate.
- I: remove slow residual bias (sensor/mount asymmetry).
- Kw*wheel_speed: discourage long-term wheel speed drift.

Post-processing:
- hard clamp (safety/current ceiling),
- slew limiter (reduce impulsive command jumps).
*/

namespace {
// Local clamp utility to stay lightweight on MCU.
float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}
}  // namespace

// Reset controller memory when entering/leaving active control.
void BalanceController::reset() {
    integ_ = 0.0f;
    last_out_ = 0.0f;
}

float BalanceController::step(float theta_err_deg, float theta_rate_dps, int32_t wheel_speed_raw, float dt_s) {
    if (dt_s <= 0.000001f) {
        return last_out_;
    }

    // Integral accumulation with anti-windup clamp.
    integ_ += theta_err_deg * dt_s;
    integ_ = clampf(integ_, -appcfg::kIntegralLimit, appcfg::kIntegralLimit);

    // Wheel damping term keeps long-term wheel speed from drifting too high.
    float wheel_term = appcfg::kKw * static_cast<float>(wheel_speed_raw);
    // Main control law (single-axis reaction-wheel balancing).
    float raw_out = appcfg::kKp * theta_err_deg + appcfg::kKd * theta_rate_dps + appcfg::kKi * integ_ - wheel_term;

    // Hard limit protects motor and power stage.
    float limited = clampf(raw_out, -appcfg::kCurrentCmdAbsMax, appcfg::kCurrentCmdAbsMax);

    // Slew-rate limit reduces abrupt torque jumps and startup shocks.
    float max_delta = appcfg::kCurrentCmdSlewPerSec * dt_s;
    float with_slew = clampf(limited, last_out_ - max_delta, last_out_ + max_delta);

    last_out_ = with_slew;
    return with_slew;
}
