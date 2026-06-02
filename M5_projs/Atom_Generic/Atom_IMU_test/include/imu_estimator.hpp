#pragma once

#include "app_types.hpp"

/*
ImuEstimator
============
单轴姿态估计模块。

设计目的：
- 尽量轻量，适合 MCU 实时运行
- 第一版不引入更重的姿态融合框架
- 输出相对稳定的角度/角速度，供 PID 类控制器直接使用
*/
class ImuEstimator {
public:
    // 复位内部状态。应在 IMU 初始化完成后调用一次。
    void begin();
    // 执行一次固定步长更新。
    // `dt_s` 应与调度周期保持一致。
    void update(float dt_s);
    // 返回当前的状态快照。
    ImuSample sample() const;

private:
    ImuSample sample_{};
    float angle_filt_deg_ = 0.0f;
    float prev_angle_filt_deg_ = 0.0f;
    bool initialized_ = false;
};
