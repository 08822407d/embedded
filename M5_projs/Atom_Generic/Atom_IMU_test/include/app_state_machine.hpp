#pragma once

#include "app_types.hpp"
#include "safety_guard.hpp"

/*
AppStateMachine
===============
不带“起跳”功能时的运行状态机：
`Disarmed -> RefCapture -> Balancing -> Fault`

核心原则：
- 不假设上电姿态就是平衡态
- 在 `RefCapture` 阶段由用户手动扶正
- 系统在稳定窗口内采集参考角 `theta_ref`
*/
class AppStateMachine {
public:
    // 初始化到 Disarmed。
    void begin();
    // 外部请求使能或关闭，例如按键或串口命令。
    void requestEnabled(bool enabled);
    // 周期性更新状态机。
    void update(uint32_t now_ms, const ImuSample& imu, const MotorFeedback& motor, const SafetyGuard& safety);

    // 只读状态输出接口。
    AppMode mode() const;
    FaultCode fault() const;
    float thetaRefDeg() const;                       // 已采集到的参考平衡角。
    float thetaErrorDeg(const ImuSample& imu) const;  // 当前相对参考角的控制误差。
    bool motorShouldEnable() const;                  // 当前是否应该打开电机输出级。
    bool controlActive() const;                      // 当前是否允许控制器输出指令。

private:
    void enterDisarmed();
    void enterRefCapture(uint32_t now_ms);
    void enterBalancing();
    void enterFault(FaultCode code);

    AppMode mode_ = AppMode::Disarmed;   // 当前运行模式。
    FaultCode fault_ = FaultCode::None;  // 最近一次故障原因。
    bool requested_enabled_ = false;     // 用户请求的使能状态。
    float theta_ref_deg_ = 0.0f;         // 已采集的参考角，用于计算姿态误差。

    // 参考角采集窗口的累计变量。
    uint32_t last_update_ms_ = 0;
    uint32_t capture_stable_ms_ = 0;
    float capture_sum_deg_ = 0.0f;
    uint32_t capture_samples_ = 0;
};
