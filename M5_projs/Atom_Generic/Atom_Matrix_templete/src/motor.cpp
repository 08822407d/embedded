#include <math.h>

#include "unit_rolleri2c.hpp"

#include "motor.h"

static UnitRollerI2C roller;
static float g_lastCmdMA = 0;  // 最近下发的目标电流（遥测用）

bool motorInit() {
    // 已知风险：IMU(Wire1) 与电机(Wire) 双 I²C 控制器共存时，可能其中一路失效。
    // 若发现"读 IMU"与"读/控电机"不能同时进行，对策是 stop 后两个 I²C 都重新初始化。
    // 详见 agent-context/protocols/rollercan-i2c.md（当前暂不写入该 workaround）。
    bool ok = roller.begin(&Wire, MOTOR_I2C_ADDR, MOTOR_I2C_SDA, MOTOR_I2C_SCL, MOTOR_I2C_FREQ);
    roller.setOutput(0);                  // 先关输出
    roller.setMode(ROLLER_MODE_CURRENT);  // 电流模式，运行中不再改
    roller.setCurrent(0);                 // 初始零力矩，防使能跳变
    roller.setOutput(1);                  // 使能
    return ok;
}

void motorSetCurrentmA(float mA) {
    if (mA >  MOTOR_MAX_MA) mA =  MOTOR_MAX_MA;
    if (mA < -MOTOR_MAX_MA) mA = -MOTOR_MAX_MA;
    g_lastCmdMA = mA;
    roller.setCurrent((int32_t)lroundf(mA * 100.0f));  // raw = mA × 100
}

void motorStop() {
    g_lastCmdMA = 0;
    roller.setCurrent(0);
}

void motorPowerOff() {
    g_lastCmdMA = 0;
    roller.setCurrent(0);
    roller.setOutput(0);
}

float motorSpeedRPM() {
    return roller.getSpeedReadback() / 100.0f;
}

float motorCurrentmA() {
    return roller.getCurrentReadback() / 100.0f;
}

uint8_t motorErrorCode() {
    return roller.getErrorCode();
}

float motorCommandedmA() {
    return g_lastCmdMA;
}
