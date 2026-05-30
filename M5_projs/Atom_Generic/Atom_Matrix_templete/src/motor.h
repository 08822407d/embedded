// motor —— RollerCAN 反作用轮电机驱动（I²C，电流/力矩模式）
//
// 约定：模式只在 motorInit() 里设一次为电流模式，**运行中绝不切模式**
//       （切模式需先关输出=撒手，且耗时，详见 agent-context/decisions/002）。
// 总线：RollerCAN 走 Wire(Grove SDA=26/SCL=32)；ATOM 内部 IMU 走 Wire1，互不冲突。
// 频率：暂定 200kHz —— **禁用 100kHz / 400kHz 危险频率**
//       （见 agent-context/protocols/esp32-i2c-frequency-caveat.md）。
#pragma once

#include <Arduino.h>

static const uint8_t  MOTOR_I2C_ADDR = 0x64;     // RollerCAN 默认 I²C 地址
static const uint8_t  MOTOR_I2C_SDA  = 26;       // ATOM Grove SDA
static const uint8_t  MOTOR_I2C_SCL  = 32;       // ATOM Grove SCL
static const uint32_t MOTOR_I2C_FREQ = 200000;   // 占位 200kHz，勿用 100k/400k
static const float    MOTOR_MAX_MA   = 1200.0f;  // 硬件电流上限 ±1200mA

// 初始化：I²C → 关输出 → 电流模式 → 电流0 → 使能。返回 I²C 是否握手成功。
bool    motorInit();
// 初始化为**速度模式**：I²C → 关输出 → 速度模式 → 设速度环最大电流 → 速度0 → 使能。
//   速度模式下控制器会在 maxCurrentmA 内施加最大努力逼近目标转速（探索更大瞬时力矩）。
//   注意：模式一次性选定，运行中不切；速度模式下急停用 motorSetSpeedRPM(0)/motorPowerOff()。
bool    motorInitSpeed(float maxCurrentmA);
// 下发目标电流（mA，正负=方向），内部钳到 ±MOTOR_MAX_MA。
void    motorSetCurrentmA(float mA);
// 下发目标转速（RPM，正负=方向）。仅速度模式有效。
void    motorSetSpeedRPM(float rpm);
// 软停：电流置 0（仍使能，飞轮滑行）。
void    motorStop();
// 断驱动：setOutput(0)。**保底用**——已确认不可恢复时切断，防空转烧毁。
void    motorPowerOff();

// 状态读取
float   motorSpeedRPM();      // 实际飞轮转速
float   motorCurrentmA();     // 实际电流
float   motorVoltage();       // 供电电压 Vin（V）——诊断电机是否供电不足/掉压
uint8_t motorErrorCode();     // 1=过压 2=堵转 4=越界
float   motorCommandedmA();   // 最近一次下发的目标电流（遥测用，不访问 I²C）
