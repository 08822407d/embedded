#include "app_state_machine.hpp"

#include "config.hpp"

/*
状态机流程概览
--------------
Disarmed：
  - 电机输出关闭
  - 等待用户请求进入工作状态

RefCapture：
  - 用户手动把系统扶到接近平衡的位置
  - 系统等待角速度足够小的一段时间
  - 对这段时间内的角度求平均，作为 `theta_ref`

Balancing：
  - 围绕 `theta_ref` 进行闭环控制
  - 持续执行故障检查

Fault：
  - 外部调用侧应关闭输出
  - 必须先解除使能，才能重新开始
*/

void AppStateMachine::begin() {
    // 上电后先进入最安全的空闲态。
    enterDisarmed();
    last_update_ms_ = 0;
}

void AppStateMachine::requestEnabled(bool enabled) {
    // 用户的使能意图先记下来，再由 `update()` 统一处理。
    requested_enabled_ = enabled;
}

void AppStateMachine::update(uint32_t now_ms, const ImuSample& imu, const MotorFeedback& motor,
                             const SafetyGuard& safety) {
    // 计算距离上次更新过去了多久，用于参考角采集阶段的时间累计。
    uint32_t dt_ms = 0;
    if (last_update_ms_ != 0) {
        dt_ms = now_ms - last_update_ms_;
    }
    last_update_ms_ = now_ms;

    switch (mode_) {
        case AppMode::Disarmed: {
            // 空闲态只等待用户请求进入工作流程。
            if (requested_enabled_) {
                enterRefCapture(now_ms);
            }
            break;
        }
        case AppMode::RefCapture: {
            // 采集参考角期间，用户可以随时取消。
            if (!requested_enabled_) {
                enterDisarmed();
                break;
            }
            // 没有有效 IMU 数据就无法继续。
            if (!imu.valid) {
                break;
            }

            if (safety.isCaptureStable(imu)) {
                // 足够稳定时，累计时间并累加角度，准备做平均。
                capture_stable_ms_ += dt_ms;
                capture_sum_deg_ += imu.roll_deg;
                capture_samples_++;
                if (capture_stable_ms_ >= appcfg::kCaptureStableMs && capture_samples_ > 0) {
                    // 采集完成：把稳定窗口内的平均角度作为参考平衡角。
                    theta_ref_deg_ = capture_sum_deg_ / static_cast<float>(capture_samples_);
                    enterBalancing();
                }
            } else {
                // 一旦不稳定，就清空窗口重新开始采集。
                capture_stable_ms_ = 0;
                capture_sum_deg_ = 0.0f;
                capture_samples_ = 0;
            }

            // 参考角采集期间如果电机已经报错，也直接进入故障态。
            if (motor.valid && motor.error_code != 0) {
                enterFault(FaultCode::MotorError);
            }
            break;
        }
        case AppMode::Balancing: {
            // 用户主动关闭时，直接退回安全空闲态。
            if (!requested_enabled_) {
                enterDisarmed();
                break;
            }

            // 平衡运行期间持续检查故障条件。
            const float theta_err = thetaErrorDeg(imu);
            FaultCode f = safety.checkFault(theta_err, motor, mode_);
            if (f != FaultCode::None) {
                enterFault(f);
            }
            break;
        }
        case AppMode::Fault: {
            // 故障态不会自动恢复，必须由用户先解除使能。
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
    // 控制误差永远相对“采集到的参考平衡角”，而不是相对上电姿态。
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
    // 进入空闲态时，清空参考角采集窗口。
    capture_stable_ms_ = 0;
    capture_sum_deg_ = 0.0f;
    capture_samples_ = 0;
}

void AppStateMachine::enterRefCapture(uint32_t now_ms) {
    (void)now_ms;
    mode_ = AppMode::RefCapture;
    fault_ = FaultCode::None;
    // 每次重新使能时，都从新的采集窗口开始。
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
