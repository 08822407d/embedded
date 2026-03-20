#include "imu_estimator.hpp"

#include <M5Atom.h>

#include "config.hpp"

/*
Estimator principle (single axis)
---------------------------------
The M5 IMU API already provides attitude (pitch/roll). For this first version,
we keep computation simple and robust:
- use one selected attitude axis as angle,
- smooth angle with LPF,
- derive rate numerically and smooth rate again.

This avoids complex filter tuning while giving stable enough signals for control.
*/

// Reset all estimator internals.
void ImuEstimator::begin() {
    sample_ = {};
    angle_filt_deg_ = 0.0f;
    prev_angle_filt_deg_ = 0.0f;
    initialized_ = false;
}

// Estimator update:
// 1) Read attitude from M5 IMU.
// 2) Select balancing axis (roll or pitch).
// 3) Apply sign/mount mapping.
// 4) Low-pass angle and derive filtered rate.
void ImuEstimator::update(float dt_s) {
    if (dt_s <= 0.000001f) {
        return;
    }

    double pitch = 0.0;
    double roll = 0.0;
    M5.IMU.getAttitude(&pitch, &roll);

    // We only control one axis for this balancing prototype.
    float raw_angle = appcfg::kUseRollAxis ? static_cast<float>(roll) : static_cast<float>(pitch);
    raw_angle = appcfg::kAngleSign * raw_angle + appcfg::kMountOffsetDeg;

    if (!initialized_) {
        // First sample: bootstrap filters without a startup spike.
        angle_filt_deg_ = raw_angle;
        prev_angle_filt_deg_ = raw_angle;
        sample_.roll_rate_dps = 0.0f;
        initialized_ = true;
    } else {
        // First-order LPF on angle.
        angle_filt_deg_ += appcfg::kAngleLpfAlpha * (raw_angle - angle_filt_deg_);
        // Numerical derivative + LPF for rate.
        float raw_rate = (angle_filt_deg_ - prev_angle_filt_deg_) / dt_s;
        sample_.roll_rate_dps += appcfg::kRateLpfAlpha * (raw_rate - sample_.roll_rate_dps);
        prev_angle_filt_deg_ = angle_filt_deg_;
    }

    sample_.roll_deg = angle_filt_deg_;
    sample_.seq++;
    sample_.valid = true;
}

// Return-by-value keeps call side simple and lock-free in this single-threaded loop usage.
ImuSample ImuEstimator::sample() const {
    return sample_;
}
