#include <M5Atom.h>
#include <math.h>

#include "app_state_machine.hpp"
#include "balance_controller.hpp"
#include "config.hpp"
#include "imu_estimator.hpp"
#include "roller_i2c_driver.hpp"
#include "safety_guard.hpp"

/*
Main architecture (single-thread cooperative scheduler)
=======================================================
This first version keeps everything in loop() with fixed-period sections:

1) IMU section      @ kImuHz
   - update attitude/rate estimator.

2) Feedback section @ kFeedbackHz
   - read Roller speed/current/status/error.

3) Control section  @ kCtrlHz
   - run state machine.
   - enable/disable motor output on state transitions.
   - compute current command when balancing.

4) UI section       @ kUiHz
   - show concise mode/tilt visualization on Atom 5x5 LED.

5) Log section      @ kLogHz
   - print diagnostics for tuning and troubleshooting.

The scheduler uses elapsed micros() checks instead of delay() so each section
can run at independent rates with predictable cadence.
*/

// Global motor direction switch requested by user.
// +1: normal, -1: reverse.
int gMotorDirection = +1;

namespace {

// Core modules.
ImuEstimator gImu;
RollerI2CDriver gRoller;
BalanceController gController;
SafetyGuard gSafety;
AppStateMachine gStateMachine;

// Shared runtime state.
MotorFeedback gMotorFeedback{};
float gLastCurrentCmd = 0.0f;
bool gEnableRequested = false;
bool gMotorOutputEnabled = false;
bool gRollerOnline = false;

// Per-section scheduler timestamps in microseconds.
uint32_t gLastImuUs = 0;
uint32_t gLastCtrlUs = 0;
uint32_t gLastFeedbackUs = 0;
uint32_t gLastUiUs = 0;
uint32_t gLastLogUs = 0;

// Helper conversion for fixed-rate sections.
float periodSec(uint32_t hz) {
    return 1.0f / static_cast<float>(hz);
}

// Helper conversion for fixed-rate sections.
uint32_t periodUs(uint32_t hz) {
    return static_cast<uint32_t>(1000000UL / hz);
}

// Convert signed angle magnitude into one of 5 LED columns.
// This is mainly a debugging visualization of control error.
int angleToIndex(float angleDegSigned) {
    float absA = fabsf(angleDegSigned);
    if (absA <= appcfg::kLedDeadbandDeg) return 2;
    if (absA <= appcfg::kLedMidbandDeg) return (angleDegSigned > 0.0f) ? 1 : 3;
    return (angleDegSigned > 0.0f) ? 0 : 4;
}

// Utility to paint all 25 LEDs.
void fillAll(uint32_t color) {
    for (uint8_t y = 0; y < 5; ++y) {
        for (uint8_t x = 0; x < 5; ++x) {
            M5.dis.drawpix(x, y, color);
        }
    }
}

// DISARMED indication: blue center dot.
void drawDisarmed() {
    M5.dis.clear();
    M5.dis.drawpix(2, 2, 0x000030);
}

// REF_CAPTURE indication: blinking amber bar.
void drawRefCapture(bool blinkOn) {
    M5.dis.clear();
    if (blinkOn) {
        M5.dis.drawpix(1, 2, 0x302000);
        M5.dis.drawpix(2, 2, 0x302000);
        M5.dis.drawpix(3, 2, 0x302000);
    }
}

// BALANCING indication: green column shifts with signed angle error.
void drawBalancing(float thetaErrDeg) {
    M5.dis.clear();
    const int idx = angleToIndex(thetaErrDeg);
    const uint8_t x = static_cast<uint8_t>(idx);
    for (uint8_t y = 0; y < 5; ++y) {
        M5.dis.drawpix(x, y, 0x003000);
    }
}

// FAULT indication: solid red.
void drawFault() {
    fillAll(0x200000);
}

// String name used by serial log stream.
const char* modeName(AppMode mode) {
    switch (mode) {
        case AppMode::Disarmed:
            return "DISARMED";
        case AppMode::RefCapture:
            return "REF_CAPTURE";
        case AppMode::Balancing:
            return "BALANCING";
        case AppMode::Fault:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}

}  // namespace

void setup() {
    // Serial diagnostics are essential for first tuning sessions.
    Serial.begin(115200);

    // Atom board + display init.
    M5.begin(true, true, true);
    delay(80);
    M5.dis.setBrightness(appcfg::kLedBrightness);
    M5.dis.clear();

    // IMU init must happen before estimator begins.
    M5.IMU.Init();
    delay(60);
    gImu.begin();

    // Bring up motor interface (I2C + current mode).
    gRollerOnline = gRoller.begin();

    // Start state machine in disarmed mode.
    gStateMachine.begin();
    gStateMachine.requestEnabled(false);

    // Align all scheduler phases to "now".
    const uint32_t nowUs = micros();
    gLastImuUs = nowUs;
    gLastCtrlUs = nowUs;
    gLastFeedbackUs = nowUs;
    gLastUiUs = nowUs;
    gLastLogUs = nowUs;

    drawDisarmed();
}

void loop() {
    M5.update();

    // Short press toggles arm request.
    // Arm only allowed when Roller is online.
    if (M5.Btn.wasPressed()) {
        if (gRollerOnline) {
            gEnableRequested = !gEnableRequested;
            gStateMachine.requestEnabled(gEnableRequested);
        }
    }

    const uint32_t nowUs = micros();
    const uint32_t nowMs = millis();

    // 1) IMU loop.
    if (static_cast<uint32_t>(nowUs - gLastImuUs) >= periodUs(appcfg::kImuHz)) {
        gLastImuUs += periodUs(appcfg::kImuHz);
        gImu.update(periodSec(appcfg::kImuHz));
    }

    // 2) Motor feedback loop.
    if (static_cast<uint32_t>(nowUs - gLastFeedbackUs) >= periodUs(appcfg::kFeedbackHz)) {
        gLastFeedbackUs += periodUs(appcfg::kFeedbackHz);
        gMotorFeedback = gRoller.readFeedback();
    }

    // 3) State + control loop (core logic).
    if (static_cast<uint32_t>(nowUs - gLastCtrlUs) >= periodUs(appcfg::kCtrlHz)) {
        gLastCtrlUs += periodUs(appcfg::kCtrlHz);

        const ImuSample imu = gImu.sample();

        // State machine decides mode transitions and safety latch behavior.
        gStateMachine.update(nowMs, imu, gMotorFeedback, gSafety);

        // Output-stage control is tied to mode transitions, not raw button state.
        const bool shouldEnableMotor = gStateMachine.motorShouldEnable();
        if (shouldEnableMotor != gMotorOutputEnabled) {
            if (shouldEnableMotor) {
                // Fresh controller state prevents stale integral/output memory.
                gController.reset();
                gRoller.setOutput(true);
                gMotorOutputEnabled = true;
            } else {
                // On any non-balancing mode, force safe stop.
                gController.reset();
                gLastCurrentCmd = 0.0f;
                gRoller.stop();
                gMotorOutputEnabled = false;
            }
        }

        // Compute and send command only in Balancing mode.
        if (gStateMachine.controlActive()) {
            const float thetaErr = gStateMachine.thetaErrorDeg(imu);
            gLastCurrentCmd =
                gController.step(thetaErr, imu.roll_rate_dps, gMotorFeedback.speed_raw, periodSec(appcfg::kCtrlHz));
            gRoller.writeCurrentCommand(gLastCurrentCmd);
        }
    }

    // 4) UI loop.
    if (static_cast<uint32_t>(nowUs - gLastUiUs) >= periodUs(appcfg::kUiHz)) {
        gLastUiUs += periodUs(appcfg::kUiHz);

        const AppMode mode = gStateMachine.mode();
        const ImuSample imu = gImu.sample();
        const float thetaErr = gStateMachine.thetaErrorDeg(imu);

        // Offline Roller is treated visually as fault.
        if (!gRollerOnline) {
            drawFault();
        } else if (mode == AppMode::Disarmed) {
            drawDisarmed();
        } else if (mode == AppMode::RefCapture) {
            // Blink helps user hold system still while ref is captured.
            const bool blinkOn = ((nowMs / 250) % 2) == 0;
            drawRefCapture(blinkOn);
        } else if (mode == AppMode::Balancing) {
            drawBalancing(thetaErr);
        } else {
            drawFault();
        }
    }

    // 5) Log loop.
    // This is the main tuning stream: mode, reference angle, current angle/rate,
    // command output, and motor readback.
    if (static_cast<uint32_t>(nowUs - gLastLogUs) >= periodUs(appcfg::kLogHz)) {
        gLastLogUs += periodUs(appcfg::kLogHz);
        const ImuSample imu = gImu.sample();
        const float thetaErr = gStateMachine.thetaErrorDeg(imu);
        Serial.printf(
            "mode=%s req=%d ref=%.2f roll=%.2f err=%.2f rate=%.2f cmd=%.1f speed=%ld cur=%ld ferr=%u dir=%d online=%d\n",
            modeName(gStateMachine.mode()), gEnableRequested ? 1 : 0, gStateMachine.thetaRefDeg(), imu.roll_deg,
            thetaErr, imu.roll_rate_dps, gLastCurrentCmd, static_cast<long>(gMotorFeedback.speed_raw),
            static_cast<long>(gMotorFeedback.current_raw), gStateMachine.fault() == FaultCode::None ? 0 : 1,
            gMotorDirection, gRollerOnline ? 1 : 0);
    }
}
