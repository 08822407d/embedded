#pragma once

#include "app_types.hpp"

/*
ImuEstimator
============
Lightweight estimator for one balancing axis.

Design intent:
- Keep estimator deterministic and cheap for MCU.
- Avoid heavy AHRS dependency in first version.
- Output stable angle/rate pair suitable for PID-style control.
*/
class ImuEstimator {
public:
    // Reset internal states. Call once after IMU init.
    void begin();
    // Update estimator by one fixed-step cycle.
    // dt_s should match the scheduler period.
    void update(float dt_s);
    // Return latest sample snapshot.
    ImuSample sample() const;

private:
    ImuSample sample_{};
    float angle_filt_deg_ = 0.0f;
    float prev_angle_filt_deg_ = 0.0f;
    bool initialized_ = false;
};
