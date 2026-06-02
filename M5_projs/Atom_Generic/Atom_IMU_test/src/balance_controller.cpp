#include "balance_controller.hpp"

#include "config.hpp"

/*
控制器思路
----------
反作用轮平衡的本质，是在单轴上快速给出纠偏扭矩。

各项的作用：
- P：根据姿态误差直接扶正
- D：根据机体角速度提供阻尼
- I：补偿长期偏差，例如安装不对称或零偏
- Kw*wheel_speed：抑制飞轮长期越转越快

输出后处理：
- 硬限幅：保护电机和电源
- 斜率限制：减少瞬间电流突变
*/

namespace {
// 本地 clamp，保持实现简单轻量。
float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}
}  // namespace

// 进入或退出闭环时清空控制器记忆量。
void BalanceController::reset() {
    integ_ = 0.0f;
    last_out_ = 0.0f;
}

float BalanceController::step(float theta_err_deg, float theta_rate_dps, int32_t wheel_speed_raw, float dt_s) {
    if (dt_s <= 0.000001f) {
        return last_out_;
    }

    // 积分累加，并通过限幅避免积分饱和。
    integ_ += theta_err_deg * dt_s;
    integ_ = clampf(integ_, -appcfg::kIntegralLimit, appcfg::kIntegralLimit);

    // 飞轮速度抑制项，防止长期越转越快。
    float wheel_term = appcfg::kKw * static_cast<float>(wheel_speed_raw);
    // 主控制律：姿态误差 + 姿态角速度 + 积分补偿 - 飞轮偏速项。
    float raw_out = appcfg::kKp * theta_err_deg + appcfg::kKd * theta_rate_dps + appcfg::kKi * integ_ - wheel_term;

    // 硬限幅保护电机和驱动。
    float limited = clampf(raw_out, -appcfg::kCurrentCmdAbsMax, appcfg::kCurrentCmdAbsMax);

    // 斜率限制减少突变，尤其对刚进入平衡态时更重要。
    float max_delta = appcfg::kCurrentCmdSlewPerSec * dt_s;
    float with_slew = clampf(limited, last_out_ - max_delta, last_out_ + max_delta);

    last_out_ = with_slew;
    return with_slew;
}
