#pragma once

#include <stdint.h>

/*
BalanceController
=================
单轴反作用轮平衡控制器。

控制律：
`u = Kp*theta_err + Kd*theta_rate + Ki*integral(theta_err) - Kw*wheel_speed`

这个类是有内部状态的：
- `integ_` 用于积分项
- `last_out_` 用于输出斜率限制
*/
class BalanceController {
public:
    // 在进入/退出控制时清空积分和上一拍输出记忆。
    void reset();
    // 执行一次控制计算，返回 Roller 使用的电流指令值。
    float step(float theta_err_deg, float theta_rate_dps, int32_t wheel_speed_raw, float dt_s);

private:
    float integ_ = 0.0f;
    float last_out_ = 0.0f;
};
