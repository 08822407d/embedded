// balance —— 反作用轮倒立摆 平衡控制器实现（**电流/力矩模式**：控制器直接输出期望力矩(mA)）
//
// 模型（平衡点 θ=0 线性化，见 system-model §6）：状态 x = [θ, θ̇, ω_w]。
//   机体：I_b·θ̈ = m·g·l·θ − τ（倒立摆，需反馈镇定）；τ=电机力矩 ∝ 电流(mA)，是唯一作用量。
//   ⇒ 平衡控制器**直接输出力矩 u(mA)**，上层 motorSetCurrentmA(u) 执行（无需积分成速度命令——那是速度模式的绕法）。
// 符号：thetaDown = 使 θ 减小的电流符号(+1/-1)（θ>0 时用 thetaDown 方向力矩使 θ 回 0）。
// 注意：增益均为**保守占位**(单位已是 mA)，缺机体/飞轮惯量等标定 → 待实测整定；先求"手扶到平衡附近能稳住"。
#include <math.h>

#include "balance.h"
#include "motor.h"   // MOTOR_MAX_MA

static BalanceMethod g_method    = BALANCE_PID;
static int           g_thetaDown = -1;   // 使 θ 减小的电流符号(+1/-1)

// ---- PID(PD-on-θ + 飞轮去饱和)增益 ----
//   u(mA) = thetaDown·(Kp·θ + Kd·θ̇) − Kw·ω_w(去饱和)
//   ★由系统辨识(2026-06-01)算得：模型 θ̈=a·sinθ+b·I, a=mgl/I_b≈4400°/s², b=Kt/I_b≈16°/s²/mA。
//     取 ωn=10rad/s、ζ=1: Kp=(ωn²+a·π/180)/b≈11, Kd=2ζωn/b≈1.25。(辨识 R²=0.53 偏低，待更好数据精化)
static const float PID_KP = 11.0f;   // mA per °      （角度比例，辨识初值）
static const float PID_KD = 1.25f;   // mA per (°/s)  （角速度阻尼，辨识初值）
static const float PID_KW = 0.05f;   // mA per rpm    （飞轮转速去饱和，慢慢把飞轮往 0 泄）

// ---- LQR 备用增益 u = -K·x（mA，占位待整定）----
static const float LQR_K1 = 10.0f;   // θ
static const float LQR_K2 = 1.5f;    // θ̇
static const float LQR_K3 = 0.06f;   // ω_w(去饱和)

void          balanceInit(int thetaDownDir) { g_thetaDown = (thetaDownDir >= 0) ? +1 : -1; g_method = BALANCE_PID; }
void          balanceSetMethod(BalanceMethod m) { g_method = m; }
BalanceMethod balanceMethod() { return g_method; }
void          balanceReset(float wheelRpm) { (void)wheelRpm; }   // 电流模式无内部状态，接口保留(空)

static float clampT(float u) { if (u > MOTOR_MAX_MA) return MOTOR_MAX_MA; if (u < -MOTOR_MAX_MA) return -MOTOR_MAX_MA; return u; }

// 返回期望力矩(mA)；上层 motorSetCurrentmA(返回值)。dt 当前未用(直接输出力矩，不积分)。
float balanceStep(float th, float thd, float ww, float dt) {
    (void)dt;
    float u;  // 期望力矩(mA)
    if (g_method == BALANCE_LQR) {
        u = (float)g_thetaDown * (LQR_K1 * th + LQR_K2 * thd) - LQR_K3 * ww;
    } else {
        u = (float)g_thetaDown * (PID_KP * th + PID_KD * thd) - PID_KW * ww;
    }
    return clampT(u);
}
