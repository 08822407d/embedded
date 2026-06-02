#pragma once

#include <stdint.h>

// 系统的高层运行状态，主循环和 LED/日志都依赖它。
enum class AppMode : uint8_t {
    Disarmed = 0,  // 安全空闲态，电机输出关闭。
    RefCapture,    // 人工扶正并采集参考平衡角。
    Balancing,     // 闭环平衡控制运行中。
    Fault          // 故障锁定态，需要先解除使能再重新进入。
};

// 故障原因，用于串口日志和状态判断。
enum class FaultCode : uint8_t {
    None = 0,
    TiltExceeded,  // 姿态误差超出安全范围。
    MotorError     // Roller 上报了内部故障。
};

// 经过估计后的 IMU 单轴状态，供状态机和控制器使用。
struct ImuSample {
    float roll_deg = 0.0f;       // 围绕平衡轴的机体角度，已滤波。
    float roll_rate_dps = 0.0f;  // 围绕平衡轴的机体角速度，已滤波。
    uint32_t seq = 0;            // 单调递增序号，可用于判断数据是否更新。
    bool valid = false;          // 在第一次有效更新前为 false。
};

// 从 Roller I2C 读取的关键信息。
struct MotorFeedback {
    int32_t speed_raw = 0;     // 速度反馈寄存器原始值。
    int32_t current_raw = 0;   // 电流反馈寄存器原始值。
    uint8_t sys_status = 0;    // Roller 系统状态寄存器值。
    uint8_t error_code = 0;    // Roller 错误码寄存器值。
    uint32_t seq = 0;          // 反馈更新序号。
    bool valid = false;        // 驱动离线或读取失败时为 false。
};
