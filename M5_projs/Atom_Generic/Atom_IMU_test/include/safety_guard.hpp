#pragma once

#include "app_types.hpp"

/*
SafetyGuard
===========
Contains policy checks that can force control disable.
Separated from state machine to keep safety rules explicit and testable.
*/
class SafetyGuard {
public:
    // During reference capture, require low angular velocity.
    bool isCaptureStable(const ImuSample& imu) const;
    // Evaluate balancing-time fault conditions.
    FaultCode checkFault(float theta_err_deg, const MotorFeedback& motor, AppMode mode) const;
};
