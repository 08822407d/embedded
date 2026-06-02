#pragma once

#include "app_types.hpp"

#include <unit_rolleri2c.hpp>

/*
RollerI2CDriver
===============
对官方 `UnitRollerI2C` 的薄封装。

封装它的原因：
- 把硬件相关细节集中在一处
- 让主流程和控制器不直接依赖底层寄存器调用
- 统一处理方向、使能策略和反馈格式
*/
class RollerI2CDriver {
public:
    // 初始化 I2C 通信，并把电机切到电流模式。
    bool begin();
    // 使能或关闭电机输出级，对应 Roller 的 `setOutput()`。
    void setOutput(bool enabled);
    // 执行安全停机：先输出 0，再关闭输出级。
    void stop();
    // 发送电流指令。
    // 最终方向会再乘上全局变量 `gMotorDirection`。
    void writeCurrentCommand(float current_cmd);
    // 读取控制和安全逻辑需要的最小反馈集合。
    MotorFeedback readFeedback();
    // `begin()` 成功后返回 true。
    bool isOnline() const;

private:
    UnitRollerI2C roller_{};
    bool online_ = false;
    bool output_enabled_ = false;
    uint32_t feedback_seq_ = 0;
};
