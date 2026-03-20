#pragma once

#include <stdint.h>

/*
Configuration notes
===================
This file is intentionally central: most field tuning should happen here first.

Recommended tuning order for first bring-up:
1) Confirm direction/sign mapping: kUseRollAxis, kAngleSign, gMotorDirection.
2) Confirm safety limits: kFaultTiltDeg, kCurrentCmdAbsMax.
3) Tune control gains: Kp -> Kd -> Ki.
4) Add wheel-speed damping Kw only after basic balance works.

All angle values are in degrees, angular rate in deg/s, and loop rates in Hz.
Motor command values are "Roller command units" (not strict SI units).
*/

// Global motor direction switch.
// +1: normal direction, -1: reversed direction.
extern int gMotorDirection;

namespace appcfg {

// IMU axis selection and orientation mapping.
static constexpr bool kUseRollAxis = true;   // true: use roll, false: use pitch.
static constexpr float kAngleSign = +1.0f;   // Set to -1 if tilt response direction is inverted.
static constexpr float kMountOffsetDeg = 0;  // Fixed mechanical mounting offset around balance axis.

// Loop rates.
static constexpr uint32_t kImuHz = 200;       // IMU update/read frequency.
static constexpr uint32_t kCtrlHz = 100;      // Balance controller execution frequency.
static constexpr uint32_t kFeedbackHz = 50;   // Motor telemetry readback frequency.
static constexpr uint32_t kUiHz = 20;         // LED/UI refresh frequency.
static constexpr uint32_t kLogHz = 10;        // Serial logging frequency.

// IMU filters.
// First-order low-pass filter alpha:
// y = y + alpha * (x - y)
// Higher alpha follows sensor faster but keeps more noise.
static constexpr float kAngleLpfAlpha = 0.25f;  // Angle smoothing.
static constexpr float kRateLpfAlpha = 0.30f;   // Rate smoothing.

// Roller I2C wiring/config. Adjust pins to your actual Atom + Roller wiring.
static constexpr uint8_t kRollerI2cAddr = 0x64;
static constexpr uint8_t kRollerSdaPin = 26;
static constexpr uint8_t kRollerSclPin = 32;
static constexpr uint32_t kRollerI2cHz = 400000;  // 400kHz fast mode.

// Control parameters. These are first-pass startup values.
// Controller structure:
// u = Kp*theta_err + Kd*theta_rate + Ki*integral(theta_err) - Kw*wheel_speed
//
// Sign conventions:
// - theta_err is measured around the balance axis in degrees.
// - wheel_speed is Roller readback raw speed value.
// - Direction of final command is additionally controlled by gMotorDirection.
static constexpr float kKp = 35.0f;        // Stiffness against tilt error.
static constexpr float kKi = 2.0f;         // Slow bias cancellation.
static constexpr float kKd = 1.1f;         // Damping using body rate.
static constexpr float kKw = 0.00035f;     // Weak wheel-speed damping (momentum management).

// Command shaping:
// - Integral limit prevents windup.
// - Abs limit caps maximum torque/current command.
// - Slew limit prevents sudden current steps that can destabilize startup.
static constexpr float kIntegralLimit = 200.0f;
static constexpr float kCurrentCmdAbsMax = 700.0f;
static constexpr float kCurrentCmdSlewPerSec = 2000.0f;

// Arm/capture behavior (manual upright then capture reference angle).
// During reference capture, we intentionally do NOT assume table level.
// The controller stores the current body angle as theta_ref once stable.
static constexpr uint32_t kCaptureStableMs = 600;  // Required stable duration.
static constexpr float kCaptureMaxRateDps = 12.0f; // Max body rate allowed for "stable".

// Safety.
// If balancing is active and |theta_err| exceeds this threshold, output is cut.
static constexpr float kFaultTiltDeg = 30.0f;
// Motor error readback policy: non-zero means fault.
static constexpr uint8_t kFaultMotorErrorNonZero = 1;

// LED/UI.
static constexpr uint8_t kLedBrightness = 20;
static constexpr float kLedDeadbandDeg = 3.0f;  // Center column range.
static constexpr float kLedMidbandDeg = 10.0f;  // Inner side columns range.

}  // namespace appcfg
