#include "safety_guard.hpp"

#include <math.h>

#include "config.hpp"

/*
Safety policy
-------------
Current policy is intentionally conservative:
- Large tilt while balancing => immediate fault.
- Any Roller error code => immediate fault.

As tuning matures, this can be expanded with communication timeout counters,
temperature/voltage thresholds, and fault-class-specific recovery rules.
*/

bool SafetyGuard::isCaptureStable(const ImuSample& imu) const {
    // Reference capture should happen only when body movement is small.
    if (!imu.valid) {
        return false;
    }
    return fabsf(imu.roll_rate_dps) <= appcfg::kCaptureMaxRateDps;
}

FaultCode SafetyGuard::checkFault(float theta_err_deg, const MotorFeedback& motor, AppMode mode) const {
    // Large tilt during active balancing means we are outside recoverable envelope.
    if (mode == AppMode::Balancing && fabsf(theta_err_deg) > appcfg::kFaultTiltDeg) {
        return FaultCode::TiltExceeded;
    }
    // Any non-zero motor error is treated as fault in this first version.
    if (motor.valid && motor.error_code >= appcfg::kFaultMotorErrorNonZero) {
        return FaultCode::MotorError;
    }
    return FaultCode::None;
}
