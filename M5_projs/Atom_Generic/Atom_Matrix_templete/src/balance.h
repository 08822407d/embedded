// balance —— 反作用轮倒立摆 平衡控制器
//
// 执行方式（decision 006，全程速度模式、运行中不切模式）：控制器输出"期望飞轮角加速度"(∝力矩)，
//   在内部积分成"飞轮目标转速命令"，上层 motorSetSpeedRPM(返回值) 执行。即用速度环施加力矩。
// 提供两法：PID(实为 PD-on-θ + 飞轮去饱和，主用) 与 LQR(状态反馈，备用)，可切换；当前默认 PID。
//
// 符号约定：thetaDown = "使机体 pitch(θ) 减小的飞轮速度变化方向"(+1/-1)，由方向表征得到(=g_swingUpDir·st)。
//   θ>0 时需使 θ 减小 → 飞轮沿 thetaDown 方向加速；θ<0 反之。控制律内部已据此对齐符号。
#pragma once

#include <Arduino.h>

enum BalanceMethod { BALANCE_PID = 0, BALANCE_LQR = 1 };

// 初始化。thetaDownDir = 使 θ 减小的飞轮速度变化方向(+1/-1)。默认 PID。
void          balanceInit(int thetaDownDir);
void          balanceSetMethod(BalanceMethod m);
BalanceMethod balanceMethod();

// 进入平衡瞬间调用：把内部飞轮速度命令对齐到当前实际飞轮转速，避免接管时跳变。
void          balanceReset(float wheelRpm);

// 单步：输入 机体角 θ(°)、机体角速度 θ̇(°/s)、飞轮转速 ω_w(rpm)、dt(s)；
//   返回本周期飞轮目标转速命令(rpm)。上层 motorSetSpeedRPM(返回值)。按当前 method 计算。
float         balanceStep(float thetaDeg, float thetaRateDps, float wheelRpm, float dt);
