#include "app_state_machine.hpp"

#include "config.hpp"

/*
State flow summary
------------------
Disarmed:
  - Motor output off.
  - Wait for arm request.

RefCapture:
  - User manually holds near upright.
  - System waits for low angular-rate window and averages angle.
  - Average becomes theta_ref.

Balancing:
  - Closed-loop control active around theta_ref.
  - Fault checks continuously applied.

Fault:
  - Output disabled by caller side.
  - Requires de-arm to clear.
*/

void AppStateMachine::begin() {
    // Start in a safe state with motor disabled.
    enterDisarmed();
    last_update_ms_ = 0;
}

void AppStateMachine::requestEnabled(bool enabled) {
    // User intent is latched and evaluated in update().
    requested_enabled_ = enabled;
}

void AppStateMachine::update(uint32_t now_ms, const ImuSample& imu, const MotorFeedback& motor,
                             const SafetyGuard& safety) {
    // Compute elapsed ms for time-based state conditions (capture hold duration).
    uint32_t dt_ms = 0;
    if (last_update_ms_ != 0) {
        dt_ms = now_ms - last_update_ms_;
    }
    last_update_ms_ = now_ms;

    switch (mode_) {
        case AppMode::Disarmed: {
            // Waiting for user arm request.
            if (requested_enabled_) {
                enterRefCapture(now_ms);
            }
            break;
        }
        case AppMode::RefCapture: {
            // User can cancel arm request at any time.
            if (!requested_enabled_) {
                enterDisarmed();
                break;
            }
            // Need valid IMU data to continue capture.
            if (!imu.valid) {
                break;
            }

            if (safety.isCaptureStable(imu)) {
                // Stable enough: accumulate samples for average reference angle.
                capture_stable_ms_ += dt_ms;
                capture_sum_deg_ += imu.roll_deg;
                capture_samples_++;
                if (capture_stable_ms_ >= appcfg::kCaptureStableMs && capture_samples_ > 0) {
                    // Capture complete: reference angle is mean of stable window.
                    theta_ref_deg_ = capture_sum_deg_ / static_cast<float>(capture_samples_);
                    enterBalancing();
                }
            } else {
                // Unstable during capture: restart averaging window.
                capture_stable_ms_ = 0;
                capture_sum_deg_ = 0.0f;
                capture_samples_ = 0;
            }

            // Even during capture, propagate motor-reported errors.
            if (motor.valid && motor.error_code != 0) {
                enterFault(FaultCode::MotorError);
            }
            break;
        }
        case AppMode::Balancing: {
            // User de-arm transitions directly to safe idle.
            if (!requested_enabled_) {
                enterDisarmed();
                break;
            }

            // Evaluate fault rules while balancing.
            const float theta_err = thetaErrorDeg(imu);
            FaultCode f = safety.checkFault(theta_err, motor, mode_);
            if (f != FaultCode::None) {
                enterFault(f);
            }
            break;
        }
        case AppMode::Fault: {
            // Fault is latched until user explicitly de-arms.
            if (!requested_enabled_) {
                enterDisarmed();
            }
            break;
        }
        default:
            enterDisarmed();
            break;
    }
}

AppMode AppStateMachine::mode() const {
    return mode_;
}

FaultCode AppStateMachine::fault() const {
    return fault_;
}

float AppStateMachine::thetaRefDeg() const {
    return theta_ref_deg_;
}

float AppStateMachine::thetaErrorDeg(const ImuSample& imu) const {
    // Control error is always relative to captured reference, not startup angle.
    return imu.roll_deg - theta_ref_deg_;
}

bool AppStateMachine::motorShouldEnable() const {
    return mode_ == AppMode::Balancing;
}

bool AppStateMachine::controlActive() const {
    return mode_ == AppMode::Balancing;
}

void AppStateMachine::enterDisarmed() {
    mode_ = AppMode::Disarmed;
    fault_ = FaultCode::None;
    // Reset capture accumulators.
    capture_stable_ms_ = 0;
    capture_sum_deg_ = 0.0f;
    capture_samples_ = 0;
}

void AppStateMachine::enterRefCapture(uint32_t now_ms) {
    (void)now_ms;
    mode_ = AppMode::RefCapture;
    fault_ = FaultCode::None;
    // Fresh capture window each time arm is requested.
    capture_stable_ms_ = 0;
    capture_sum_deg_ = 0.0f;
    capture_samples_ = 0;
}

void AppStateMachine::enterBalancing() {
    mode_ = AppMode::Balancing;
    fault_ = FaultCode::None;
}

void AppStateMachine::enterFault(FaultCode code) {
    mode_ = AppMode::Fault;
    fault_ = code;
}
