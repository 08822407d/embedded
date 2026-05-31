// balance —— 反作用轮倒立摆 平衡控制器实现（速度模式：输出飞轮角加速度→积分成飞轮目标转速命令）
//
// 模型（平衡点 θ=0 线性化）：状态 x = [θ, θ̇, ω_w]（机体角/机体角速度/飞轮转速）。
//   机体：I_b·θ̈ = m·g·l·θ + τ_react（倒立摆，重力项 +m·g·l·θ 不稳定，需反馈镇定）。
//   控制 u = 期望飞轮角加速度(∝飞轮力矩)；机体受其反作用。方向由 thetaDown 对齐（见 .h）。
// 注意：增益均为**保守占位初值**，缺机体/飞轮惯量(I_b,I_w)、质心(m,l)、力矩常数标定 → 待**实测整定**。
//   先求"手扶到平衡附近能稳住"，再逐步加大增益；LQR 的 K 需用模型参数算或实测拟合，当前同为占位。
#include <math.h>

#include "balance.h"

static BalanceMethod g_method    = BALANCE_PID;
static int           g_thetaDown = -1;      // 使 θ 减小的飞轮速度变化方向(+1/-1)
static float         g_wcmd      = 0.0f;    // 内部维护的飞轮目标转速命令(rpm)
static const float   WMAX        = 3500.0f; // 飞轮目标转速限幅(<硬超速 4000)

// ---- PID(PD-on-θ + 飞轮去饱和)增益（保守占位，待实测调）----
//   a_wheel(期望飞轮角加速度, rpm/s) = thetaDown·(Kp·θ + Kd·θ̇) − Kw·ω_w(去饱和)
static const float PID_KP = 30.0f;  // rpm/s per °      （角度比例）
static const float PID_KD = 4.0f;   // rpm/s per (°/s)  （角速度阻尼）
static const float PID_KW = 0.5f;   // 1/s              （飞轮转速去饱和，慢慢把飞轮往 0 泄，防累积饱和）

// ---- LQR 备用增益 u = -K·x（占位，待用模型/实测整定）----
static const float LQR_K1 = 35.0f;  // θ 权重
static const float LQR_K2 = 5.0f;   // θ̇ 权重
static const float LQR_K3 = 0.02f;  // ω_w 权重（去饱和）

void          balanceInit(int thetaDownDir) { g_thetaDown = (thetaDownDir >= 0) ? +1 : -1; g_wcmd = 0; g_method = BALANCE_PID; }
void          balanceSetMethod(BalanceMethod m) { g_method = m; }
BalanceMethod balanceMethod() { return g_method; }
void          balanceReset(float wheelRpm) { g_wcmd = wheelRpm; }

static float clampw(float w) { if (w > WMAX) return WMAX; if (w < -WMAX) return -WMAX; return w; }

float balanceStep(float th, float thd, float ww, float dt) {
    float aWheel;  // 期望飞轮角加速度(rpm/s)
    if (g_method == BALANCE_LQR) {
        // u = -(K1·θ + K2·θ̇) 朝平衡分量（方向对齐 thetaDown）+ 去饱和
        aWheel = (float)g_thetaDown * (LQR_K1 * th + LQR_K2 * thd) - LQR_K3 * ww;
    } else {
        // PD-on-θ（θ>0 → 沿 thetaDown 方向加速使 θ 减小）+ 飞轮去饱和
        aWheel = (float)g_thetaDown * (PID_KP * th + PID_KD * thd) - PID_KW * ww;
    }
    g_wcmd = clampw(g_wcmd + aWheel * dt);   // 积分成飞轮目标转速命令
    return g_wcmd;
}
