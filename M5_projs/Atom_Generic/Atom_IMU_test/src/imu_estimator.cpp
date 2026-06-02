#include "imu_estimator.hpp"

#include <M5Atom.h>

#include "config.hpp"

/*
单轴估计器原理
--------------
M5 的 IMU 接口已经能直接给出姿态角（pitch/roll）。
第一版里这里保持“简单但稳定”的思路：
- 只取一个控制轴作为平衡角
- 对角度做低通滤波
- 对滤波后的角度做数值微分得到角速度
- 再对角速度做一次低通

这样可以先避免更复杂的姿态融合调参，同时给控制器提供足够稳定的输入。
*/

// 复位估计器内部状态。
void ImuEstimator::begin() {
    sample_ = {};
    angle_filt_deg_ = 0.0f;
    prev_angle_filt_deg_ = 0.0f;
    initialized_ = false;
}

// 估计器单步更新流程：
// 1) 从 M5 IMU 读取姿态角
// 2) 选取当前用于平衡控制的那个轴
// 3) 做方向和安装偏移映射
// 4) 滤波角度并计算角速度
void ImuEstimator::update(float dt_s) {
    if (dt_s <= 0.000001f) {
        return;
    }

    double pitch = 0.0;
    double roll = 0.0;
    M5.IMU.getAttitude(&pitch, &roll);

    // 当前原型只做单轴平衡，因此只取一个角度参与控制。
    float raw_angle = appcfg::kUseRollAxis ? static_cast<float>(roll) : static_cast<float>(pitch);
    raw_angle = appcfg::kAngleSign * raw_angle + appcfg::kMountOffsetDeg;

    if (!initialized_) {
        // 第一拍直接作为滤波初值，避免启动瞬间产生假尖峰。
        angle_filt_deg_ = raw_angle;
        prev_angle_filt_deg_ = raw_angle;
        sample_.roll_rate_dps = 0.0f;
        initialized_ = true;
    } else {
        // 对角度做一阶低通。
        angle_filt_deg_ += appcfg::kAngleLpfAlpha * (raw_angle - angle_filt_deg_);
        // 用数值微分得到角速度，再做一阶低通。
        float raw_rate = (angle_filt_deg_ - prev_angle_filt_deg_) / dt_s;
        sample_.roll_rate_dps += appcfg::kRateLpfAlpha * (raw_rate - sample_.roll_rate_dps);
        prev_angle_filt_deg_ = angle_filt_deg_;
    }

    sample_.roll_deg = angle_filt_deg_;
    sample_.seq++;
    sample_.valid = true;
}

// 返回值拷贝在这里足够轻量，也能保持调用侧简单。
ImuSample ImuEstimator::sample() const {
    return sample_;
}
