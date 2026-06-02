#include "safety_guard.hpp"

#include <math.h>

#include "config.hpp"

/*
安全策略
--------
当前版本的保护策略是偏保守的：
- 平衡运行中姿态误差过大，立即故障停机
- Roller 返回任何非零错误码，也立即故障停机

后续如果系统稳定下来，可以继续扩展：
- 通信超时计数
- 温度/电压阈值
- 按故障类型区分不同恢复策略
*/

bool SafetyGuard::isCaptureStable(const ImuSample& imu) const {
    // 只有机体运动足够小，才允许把当前姿态记作参考平衡角。
    if (!imu.valid) {
        return false;
    }
    return fabsf(imu.roll_rate_dps) <= appcfg::kCaptureMaxRateDps;
}

FaultCode SafetyGuard::checkFault(float theta_err_deg, const MotorFeedback& motor, AppMode mode) const {
    // 平衡时误差太大，说明已经离开可恢复工作区间。
    if (mode == AppMode::Balancing && fabsf(theta_err_deg) > appcfg::kFaultTiltDeg) {
        return FaultCode::TiltExceeded;
    }
    // 第一版里对电机错误码采取“非零即停”的保守策略。
    if (motor.valid && motor.error_code >= appcfg::kFaultMotorErrorNonZero) {
        return FaultCode::MotorError;
    }
    return FaultCode::None;
}
