// balance —— 反作用轮倒立摆 平衡控制器
//
// 执行方式（**电流/力矩模式**，与主流一致：执行权威是力矩）：控制器**直接输出期望力矩 u(mA)**，
//   上层 motorSetCurrentmA(u) 执行（无需积分成速度命令——那是速度模式的绕法）。
// 提供两法：PID(PD-on-θ + 飞轮去饱和，主用) 与 LQR(状态反馈，备用)，可切换；当前默认 PID。
//
// 符号约定：thetaDown = "使机体 pitch(θ) 减小的电流符号"(+1/-1)。θ>0 时用 thetaDown 方向力矩使 θ 回 0。
#pragma once

#include <Arduino.h>

enum BalanceMethod { BALANCE_PID = 0, BALANCE_LQR = 1 };

// 初始化。thetaDownDir = 使 θ 减小的电流符号(+1/-1)。默认 PID。
void          balanceInit(int thetaDownDir);
void          balanceSetMethod(BalanceMethod m);
BalanceMethod balanceMethod();

// 接口保留（电流模式无内部状态，空实现）。
void          balanceReset(float wheelRpm);

// 单步：输入 机体角 θ(°)、机体角速度 θ̇(°/s)、飞轮转速 ω_w(rpm)、dt(s)；
//   返回**期望力矩 u(mA)**。上层 motorSetCurrentmA(返回值)。按当前 method 计算。
float         balanceStep(float thetaDeg, float thetaRateDps, float wheelRpm, float dt);
