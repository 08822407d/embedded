#pragma once

#include "app_types.hpp"

/*
SafetyGuard
===========
封装所有会导致停机的判定规则。
之所以和状态机分开，是为了让安全逻辑更明确，也更容易单独检查。
*/
class SafetyGuard {
public:
    // 参考角采集阶段要求机体角速度足够小。
    bool isCaptureStable(const ImuSample& imu) const;
    // 评估平衡运行中的故障条件。
    FaultCode checkFault(float theta_err_deg, const MotorFeedback& motor, AppMode mode) const;
};
