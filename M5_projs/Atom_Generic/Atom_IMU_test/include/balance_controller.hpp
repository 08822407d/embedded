#pragma once

#include <stdint.h>

/*
BalanceController
=================
Single-axis controller for reaction-wheel balancing.

Control law:
u = Kp*theta_err + Kd*theta_rate + Ki*integral(theta_err) - Kw*wheel_speed

This class is stateful:
- integral state for Ki term.
- last output for slew-rate limiting.
*/
class BalanceController {
public:
    // Clear integral and output memory when entering/exiting control mode.
    void reset();
    // Run one control step. Returns current command in Roller command units.
    float step(float theta_err_deg, float theta_rate_dps, int32_t wheel_speed_raw, float dt_s);

private:
    float integ_ = 0.0f;
    float last_out_ = 0.0f;
};
