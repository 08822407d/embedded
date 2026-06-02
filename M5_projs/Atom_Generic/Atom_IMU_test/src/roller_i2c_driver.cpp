#include "roller_i2c_driver.hpp"

#include <Wire.h>
#include <math.h>

#include "config.hpp"

/*
驱动层设计思路
--------------
这个封装故意不暴露所有 Roller 寄存器，而只保留第一版平衡控制真正需要的那部分：
- 初始化到电流模式
- 输出级开关
- 电流指令写入
- 基本反馈读取

这样可以减少主流程和底层 API 的耦合，也更方便定位问题。
*/

namespace {
// 本地 clamp，避免在嵌入式环境里额外依赖 `std::clamp`。
float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}
}  // namespace

bool RollerI2CDriver::begin() {
    // 打开 I2C，并确认配置地址上的 Roller 可以正常响应。
    online_ = roller_.begin(&Wire, appcfg::kRollerI2cAddr, appcfg::kRollerSdaPin, appcfg::kRollerSclPin,
                            appcfg::kRollerI2cHz);
    if (!online_) {
        return false;
    }

    // 安全优先的上电策略：
    // 1) 先关闭输出
    // 2) 强制进入电流模式
    // 3) 保持关闭状态，直到状态机明确允许输出
    roller_.setOutput(0);
    roller_.setMode(ROLLER_MODE_CURRENT);
    roller_.setOutput(0);
    output_enabled_ = false;
    return true;
}

void RollerI2CDriver::setOutput(bool enabled) {
    // 离线时忽略；状态不变时也不重复写。
    if (!online_ || output_enabled_ == enabled) {
        return;
    }
    roller_.setOutput(enabled ? 1 : 0);
    output_enabled_ = enabled;
}

void RollerI2CDriver::stop() {
    if (!online_) {
        return;
    }
    // 先发 0 再关输出，能减少停机瞬间的冲击。
    roller_.setCurrent(0);
    setOutput(false);
}

void RollerI2CDriver::writeCurrentCommand(float current_cmd) {
    if (!online_) {
        return;
    }

    // 先做软件限幅，避免控制器偶发尖峰直接打到电机。
    float limited = clampf(current_cmd, -appcfg::kCurrentCmdAbsMax, appcfg::kCurrentCmdAbsMax);
    // 控制器内部用浮点，最终要转换成 Roller 接受的整数指令。
    int32_t cmd = static_cast<int32_t>(lroundf(limited));
    // 全局方向开关用于现场快速反转，无需修改控制器正负号定义。
    cmd *= gMotorDirection;
    roller_.setCurrent(cmd);
}

MotorFeedback RollerI2CDriver::readFeedback() {
    MotorFeedback fb{};
    if (!online_) {
        return fb;
    }

    // 只读取控制和保护所需的最小反馈集合，降低 I2C 开销。
    fb.speed_raw = roller_.getSpeedReadback();
    fb.current_raw = roller_.getCurrentReadback();
    fb.sys_status = roller_.getSysStatus();
    fb.error_code = roller_.getErrorCode();
    fb.seq = ++feedback_seq_;
    fb.valid = true;
    return fb;
}

bool RollerI2CDriver::isOnline() const {
    return online_;
}
