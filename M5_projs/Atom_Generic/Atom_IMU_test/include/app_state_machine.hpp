#pragma once

#include "app_types.hpp"
#include "safety_guard.hpp"

/*
AppStateMachine
===============
Runtime mode management for a no-jump balancing workflow:
Disarmed -> RefCapture -> Balancing -> Fault

Key principle:
- Do not assume startup orientation equals balance.
- In RefCapture, user manually holds upright and system captures theta_ref.
*/
class AppStateMachine {
public:
    // Initialize state machine to Disarmed.
    void begin();
    // External arm/disarm request (button or command interface).
    void requestEnabled(bool enabled);
    // Periodic state update.
    void update(uint32_t now_ms, const ImuSample& imu, const MotorFeedback& motor, const SafetyGuard& safety);

    // Read-only state outputs.
    AppMode mode() const;
    FaultCode fault() const;
    float thetaRefDeg() const;                     // Captured reference angle.
    float thetaErrorDeg(const ImuSample& imu) const; // Current control error relative to captured reference.
    bool motorShouldEnable() const;                // Whether output stage should be enabled.
    bool controlActive() const;                    // Whether controller should compute/output commands.

private:
    void enterDisarmed();
    void enterRefCapture(uint32_t now_ms);
    void enterBalancing();
    void enterFault(FaultCode code);

    AppMode mode_ = AppMode::Disarmed;  // Current runtime mode.
    FaultCode fault_ = FaultCode::None; // Last fault cause.
    bool requested_enabled_ = false;    // User arm/disarm intent.
    float theta_ref_deg_ = 0.0f;        // Captured reference angle for theta error.

    // Capture-window bookkeeping.
    uint32_t last_update_ms_ = 0;
    uint32_t capture_stable_ms_ = 0;
    float capture_sum_deg_ = 0.0f;
    uint32_t capture_samples_ = 0;
};
