#include <M5Atom.h>
#include <math.h>

#include "app_state_machine.hpp"
#include "balance_controller.hpp"
#include "config.hpp"
#include "imu_estimator.hpp"
#include "roller_i2c_driver.hpp"
#include "safety_guard.hpp"

/*
主程序结构（单线程协作式调度）
==============================
第一版没有引入 RTOS 任务划分，而是在 `loop()` 里按固定周期执行几个子模块：

1) IMU 部分      @ `kImuHz`
   - 更新姿态角和角速度估计

2) 电机反馈部分  @ `kFeedbackHz`
   - 读取 Roller 的速度、电流、状态和错误码

3) 状态机与控制部分 @ `kCtrlHz`
   - 更新状态机
   - 根据状态切换控制电机输出级
   - 在平衡态下计算当前电流指令

4) UI 部分       @ `kUiHz`
   - 用 Atom 的 5x5 LED 显示状态和误差方向

5) 日志部分      @ `kLogHz`
   - 输出调试信息，便于现场调参

这里使用 `micros()` 做非阻塞定时，而不是大量 `delay()`，
目的是让不同模块可以用各自频率运行，同时保持整体节奏稳定。
*/

// 全局电机方向开关。
// `+1` 表示默认方向，`-1` 表示反向。
int gMotorDirection = +1;

namespace {

// 核心模块实例。
ImuEstimator gImu;
RollerI2CDriver gRoller;
BalanceController gController;
SafetyGuard gSafety;
AppStateMachine gStateMachine;

// 运行时共享状态。
MotorFeedback gMotorFeedback{};
float gLastCurrentCmd = 0.0f;
bool gEnableRequested = false;
bool gMotorOutputEnabled = false;
bool gRollerOnline = false;

// 各调度分区的时间戳，单位是微秒。
uint32_t gLastImuUs = 0;
uint32_t gLastCtrlUs = 0;
uint32_t gLastFeedbackUs = 0;
uint32_t gLastUiUs = 0;
uint32_t gLastLogUs = 0;

// 频率转秒，用于控制器和估计器。
float periodSec(uint32_t hz) {
    return 1.0f / static_cast<float>(hz);
}

// 频率转微秒，用于调度判定。
uint32_t periodUs(uint32_t hz) {
    return static_cast<uint32_t>(1000000UL / hz);
}

// 把有符号角度误差映射成 5 列 LED 的索引。
// 这只是调试可视化，不参与控制。
int angleToIndex(float angleDegSigned) {
    float absA = fabsf(angleDegSigned);
    if (absA <= appcfg::kLedDeadbandDeg) return 2;
    if (absA <= appcfg::kLedMidbandDeg) return (angleDegSigned > 0.0f) ? 1 : 3;
    return (angleDegSigned > 0.0f) ? 0 : 4;
}

// 把 5x5 LED 全部填成同一种颜色。
void fillAll(uint32_t color) {
    for (uint8_t y = 0; y < 5; ++y) {
        for (uint8_t x = 0; x < 5; ++x) {
            M5.dis.drawpix(x, y, color);
        }
    }
}

// `Disarmed` 状态的 LED：中心蓝点。
void drawDisarmed() {
    M5.dis.clear();
    M5.dis.drawpix(2, 2, 0x000030);
}

// `RefCapture` 状态的 LED：闪烁的黄色横条。
void drawRefCapture(bool blinkOn) {
    M5.dis.clear();
    if (blinkOn) {
        M5.dis.drawpix(1, 2, 0x302000);
        M5.dis.drawpix(2, 2, 0x302000);
        M5.dis.drawpix(3, 2, 0x302000);
    }
}

// `Balancing` 状态的 LED：绿色列随误差方向左右移动。
void drawBalancing(float thetaErrDeg) {
    M5.dis.clear();
    const int idx = angleToIndex(thetaErrDeg);
    const uint8_t x = static_cast<uint8_t>(idx);
    for (uint8_t y = 0; y < 5; ++y) {
        M5.dis.drawpix(x, y, 0x003000);
    }
}

// `Fault` 状态的 LED：全红。
void drawFault() {
    fillAll(0x200000);
}

// 串口日志中使用的模式名称。
const char* modeName(AppMode mode) {
    switch (mode) {
        case AppMode::Disarmed:
            return "DISARMED";
        case AppMode::RefCapture:
            return "REF_CAPTURE";
        case AppMode::Balancing:
            return "BALANCING";
        case AppMode::Fault:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}

}  // namespace

void setup() {
    // 第一版调试强依赖串口日志，因此默认开启。
    Serial.begin(115200);

    // 初始化 Atom 板卡和点阵显示。
    M5.begin(true, true, true);
    delay(80);
    M5.dis.setBrightness(appcfg::kLedBrightness);
    M5.dis.clear();

    // IMU 必须先初始化，再启动估计器。
    M5.IMU.Init();
    delay(60);
    gImu.begin();

    // 初始化 Roller I2C 接口，并切到电流模式。
    gRollerOnline = gRoller.begin();

    // 状态机从空闲态开始。
    gStateMachine.begin();
    gStateMachine.requestEnabled(false);

    // 把所有调度时钟对齐到当前时刻。
    const uint32_t nowUs = micros();
    gLastImuUs = nowUs;
    gLastCtrlUs = nowUs;
    gLastFeedbackUs = nowUs;
    gLastUiUs = nowUs;
    gLastLogUs = nowUs;

    drawDisarmed();
}

void loop() {
    M5.update();

    // 按键短按切换“请求使能”状态。
    // 只有 Roller 在线时才允许进入工作流程。
    if (M5.Btn.wasPressed()) {
        if (gRollerOnline) {
            gEnableRequested = !gEnableRequested;
            gStateMachine.requestEnabled(gEnableRequested);
        }
    }

    const uint32_t nowUs = micros();
    const uint32_t nowMs = millis();

    // 1) IMU 更新区。
    if (static_cast<uint32_t>(nowUs - gLastImuUs) >= periodUs(appcfg::kImuHz)) {
        gLastImuUs += periodUs(appcfg::kImuHz);
        gImu.update(periodSec(appcfg::kImuHz));
    }

    // 2) 电机反馈读取区。
    if (static_cast<uint32_t>(nowUs - gLastFeedbackUs) >= periodUs(appcfg::kFeedbackHz)) {
        gLastFeedbackUs += periodUs(appcfg::kFeedbackHz);
        gMotorFeedback = gRoller.readFeedback();
    }

    // 3) 状态机 + 控制区，这是主逻辑核心。
    if (static_cast<uint32_t>(nowUs - gLastCtrlUs) >= periodUs(appcfg::kCtrlHz)) {
        gLastCtrlUs += periodUs(appcfg::kCtrlHz);

        const ImuSample imu = gImu.sample();

        // 状态机负责决定模式切换，以及故障是否锁定。
        gStateMachine.update(nowMs, imu, gMotorFeedback, gSafety);

        // 电机输出级是否打开，跟随状态机，而不是直接跟按键绑定。
        const bool shouldEnableMotor = gStateMachine.motorShouldEnable();
        if (shouldEnableMotor != gMotorOutputEnabled) {
            if (shouldEnableMotor) {
                // 进入闭环前先重置控制器，避免积分和上一拍输出残留。
                gController.reset();
                gRoller.setOutput(true);
                gMotorOutputEnabled = true;
            } else {
                // 只要离开平衡态，就执行安全停机。
                gController.reset();
                gLastCurrentCmd = 0.0f;
                gRoller.stop();
                gMotorOutputEnabled = false;
            }
        }

        // 只有在 `Balancing` 状态下才真正计算并下发控制指令。
        if (gStateMachine.controlActive()) {
            const float thetaErr = gStateMachine.thetaErrorDeg(imu);
            gLastCurrentCmd =
                gController.step(thetaErr, imu.roll_rate_dps, gMotorFeedback.speed_raw, periodSec(appcfg::kCtrlHz));
            gRoller.writeCurrentCommand(gLastCurrentCmd);
        }
    }

    // 4) LED/UI 更新区。
    if (static_cast<uint32_t>(nowUs - gLastUiUs) >= periodUs(appcfg::kUiHz)) {
        gLastUiUs += periodUs(appcfg::kUiHz);

        const AppMode mode = gStateMachine.mode();
        const ImuSample imu = gImu.sample();
        const float thetaErr = gStateMachine.thetaErrorDeg(imu);

        // Roller 离线时，视觉上直接按故障态处理。
        if (!gRollerOnline) {
            drawFault();
        } else if (mode == AppMode::Disarmed) {
            drawDisarmed();
        } else if (mode == AppMode::RefCapture) {
            // 闪烁提示用户此时应尽量把系统扶稳不动。
            const bool blinkOn = ((nowMs / 250) % 2) == 0;
            drawRefCapture(blinkOn);
        } else if (mode == AppMode::Balancing) {
            drawBalancing(thetaErr);
        } else {
            drawFault();
        }
    }

    // 5) 串口日志区。
    // 这是调参时最重要的观察窗口：
    // 模式、参考角、当前角度、角速度、控制输出、电机反馈都会从这里看到。
    if (static_cast<uint32_t>(nowUs - gLastLogUs) >= periodUs(appcfg::kLogHz)) {
        gLastLogUs += periodUs(appcfg::kLogHz);
        const ImuSample imu = gImu.sample();
        const float thetaErr = gStateMachine.thetaErrorDeg(imu);
        Serial.printf(
            "mode=%s req=%d ref=%.2f roll=%.2f err=%.2f rate=%.2f cmd=%.1f speed=%ld cur=%ld ferr=%u dir=%d online=%d\n",
            modeName(gStateMachine.mode()), gEnableRequested ? 1 : 0, gStateMachine.thetaRefDeg(), imu.roll_deg,
            thetaErr, imu.roll_rate_dps, gLastCurrentCmd, static_cast<long>(gMotorFeedback.speed_raw),
            static_cast<long>(gMotorFeedback.current_raw), gStateMachine.fault() == FaultCode::None ? 0 : 1,
            gMotorDirection, gRollerOnline ? 1 : 0);
    }
}
