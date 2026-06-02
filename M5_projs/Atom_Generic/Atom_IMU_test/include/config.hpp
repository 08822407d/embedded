#pragma once

#include <stdint.h>

/*
配置说明
========
本文件是项目的集中参数入口，现场调试时通常优先修改这里。

推荐调试顺序：
1) 先确认方向和符号：`kUseRollAxis`、`kAngleSign`、`gMotorDirection`
2) 再确认保护参数：`kFaultTiltDeg`、`kCurrentCmdAbsMax`
3) 再调控制参数：`Kp -> Kd -> Ki`
4) 最后再加 `Kw`，用于抑制飞轮长期偏速

这里的角度单位为“度”，角速度单位为“度/秒”，频率单位为“Hz”。
电机指令值使用 Roller 的工程量，不严格对应 SI 物理单位。
*/

// 全局电机方向开关。
// `+1` 为默认方向，`-1` 为反向。
extern int gMotorDirection;

namespace appcfg {

// IMU 轴选择和方向映射。
static constexpr bool kUseRollAxis = true;   // `true` 用 roll，`false` 用 pitch。
static constexpr float kAngleSign = +1.0f;   // 倾斜方向判断反了就改成 `-1`。
static constexpr float kMountOffsetDeg = 0;  // 机械安装造成的固定角度偏移。

// 各模块运行频率。
static constexpr uint32_t kImuHz = 200;       // IMU 采样和估计更新频率。
static constexpr uint32_t kCtrlHz = 100;      // 平衡控制器运行频率。
static constexpr uint32_t kFeedbackHz = 50;   // 电机读反馈频率。
static constexpr uint32_t kUiHz = 20;         // LED 刷新频率。
static constexpr uint32_t kLogHz = 10;        // 串口日志输出频率。

// IMU 滤波参数。
// 这里使用一阶低通：
// `y = y + alpha * (x - y)`
// alpha 越大，跟随越快，但噪声也会更明显。
static constexpr float kAngleLpfAlpha = 0.25f;  // 角度滤波强度。
static constexpr float kRateLpfAlpha = 0.30f;   // 角速度滤波强度。

// Roller 的 I2C 接线参数，需要按你的实际接线修改。
static constexpr uint8_t kRollerI2cAddr = 0x64;
static constexpr uint8_t kRollerSdaPin = 26;
static constexpr uint8_t kRollerSclPin = 32;
static constexpr uint32_t kRollerI2cHz = 400000;  // 400kHz Fast Mode。

// 控制参数。当前是第一版的起步值，不是最终值。
// 控制律形式：
// `u = Kp*theta_err + Kd*theta_rate + Ki*integral(theta_err) - Kw*wheel_speed`
//
// 其中：
// `theta_err` 是机体相对参考平衡角的误差，单位是“度”
// `wheel_speed` 是 Roller 返回的原始速度值
// 最终输出方向还会再乘以 `gMotorDirection`
static constexpr float kKp = 35.0f;        // 姿态误差刚度，负责“扶正”。
static constexpr float kKi = 2.0f;         // 慢速消除静差和偏置。
static constexpr float kKd = 1.1f;         // 利用角速度提供阻尼。
static constexpr float kKw = 0.00035f;     // 轻微抑制飞轮长期偏速。

// 输出整形参数。
// - `IntegralLimit`：限制积分，防止积分饱和。
// - `CurrentCmdAbsMax`：限制最大输出，保护电机和电源。
// - `CurrentCmdSlewPerSec`：限制输出变化速度，避免电流突跳。
static constexpr float kIntegralLimit = 200.0f;
static constexpr float kCurrentCmdAbsMax = 700.0f;
static constexpr float kCurrentCmdSlewPerSec = 2000.0f;

// 人工扶正并采集参考角时的判定参数。
// 这里明确不假设桌面水平，也不假设上电姿态就是平衡态。
// 只要人工把系统扶到接近平衡的位置，并在短时间内保持足够稳定，
// 就把那一刻的机体角度记为 `theta_ref`。
static constexpr uint32_t kCaptureStableMs = 600;   // 需要持续稳定多久才算采集成功。
static constexpr float kCaptureMaxRateDps = 12.0f;  // 参考角采集阶段允许的最大角速度。

// 安全保护参数。
// 平衡运行中，如果姿态误差超过该阈值，就立即触发故障并停机。
static constexpr float kFaultTiltDeg = 30.0f;
// 电机错误码判定策略：当前版本里非零就算故障。
static constexpr uint8_t kFaultMotorErrorNonZero = 1;

// LED 显示参数。
static constexpr uint8_t kLedBrightness = 20;
static constexpr float kLedDeadbandDeg = 3.0f;   // 中间列对应的误差范围。
static constexpr float kLedMidbandDeg = 10.0f;   // 中间两侧列对应的误差范围。

}  // namespace appcfg
