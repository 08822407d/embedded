#include <Arduino.h>
#include <math.h>

#include <M5Atom.h>  // 仅用其 5×5 点阵 M5.dis 做校准/状态视觉提示

#include "imu.h"
#include "motor.h"
#include "dir_test.h"

// ===== 标定值（来自 agent-context/system-model.md）=====
static const float REST_B_DEG = +30.0f;  // 静止态 B（起始）
static const float REST_A_DEG = -31.0f;  // 静止态 A

// ===== 安全/恢复阈值（可调，未实测）=====
static const float DANGER_HI_DEG   = +34.0f;   // 越此 = 越过 B 外棱 → 翻向第三面
static const float DANGER_LO_DEG   = -34.0f;   // 低此 = 越过 A 外棱 → 翻
static const float REST_BAND_DEG   = 4.0f;     // 落入 A/B ±此带视为已归位
static const float SPEED_LIMIT_RPM = 4000.0f;  // 飞轮转速硬上限(防runaway烧毁)；起跳表征需更高转速余量，短脉冲不会持续空转
static const float LATERAL_DANGER_DEG = 42.0f; // 侧向(垂直可控轴)倾角阈值（用户定上限 42°；现用陀螺融合侧向角，抗伪象）
static const float FUSION_ALPHA       = 0.98f; // 互补滤波陀螺权重(τ≈1s)；加速度仅慢校正，陀螺主导动态

// ===== 起跳阈值表征参数（只朝平衡方向，自复位，无人值守安全）=====
static const float    BREAK_START_MA = 400.0f;        // 从方向辨识止步处接着加
static const float    BREAK_MAX_MA   = MOTOR_MAX_MA;  // 升到硬件上限 1200mA
static const float    BREAK_STEP_MA  = 100.0f;
static const uint32_t BREAK_PULSE_MS  = 200;          // 短脉冲：给力矩冲量，限飞轮空转
static const uint32_t BREAK_SETTLE_MS = 1000;         // 静置让重力把机体拉回 B（自复位）
static const float    BREAKAWAY_DEG   = 12.0f;        // 融合 pitch 朝平衡前进超此=脱离 B 阱

// ===== 探测参数 =====
static const float    PROBE_MA      = 100.0f;  // 起始探测电流
static const float    PROBE_MAX_MA  = 400.0f;  // 探测电流上限（80mA 实测推不动机体，需更大）
static const float    PROBE_STEP_MA = 60.0f;
static const uint32_t PROBE_MS      = 200;     // 脉宽
static const float    DETECT_DEG    = 2.0f;    // 可辨 pitch 变化阈值
static const float    CONFIRM_MA    = 150.0f;
static const uint32_t SETTLE_MS     = 400;     // 脉冲间稳定（同样采样记录）

// ===== 保底恢复 =====
static const float    RECOVER_MA         = MOTOR_MAX_MA;  // 全力反向
static const uint32_t RECOVER_TIMEOUT_MS = 1500;

static const uint32_t TICK_MS = 20;  // 采样/看门狗/遥测周期（50Hz；时间戳为准）

static int   g_restoreSign = 0;   // 使 pitch 减小的电流符号（+1/-1），0=未知
static bool  g_faulted     = false;
static float    g_lastSpeed   = 0;   // 最近一次读到的飞轮转速（看门狗用）
static float    g_lastLateral = 0;   // 最近一次侧向(out-of-plane)倾角（看门狗用）
static float    g_fusedPitch  = 0;   // 互补滤波后的 pitch（看门狗/判向用：陀螺主导，抗线加速度伪象）
static float    g_gyBias      = 0;   // pitch 轴(gy)陀螺零偏（°/s），运行前现标
static float    g_gxBias      = 0;   // lateral 轴(gx)陀螺零偏（°/s），运行前现标
static float    g_lastCur     = 0;   // 最近一次读到的实际电流（mA），避免重复 I²C 读
static uint8_t  g_lastErr     = 0;   // 最近一次电机错误码（1过压/2堵转/4越界）
static uint32_t g_lastSampleMs = 0;  // 上一帧时间戳，用于求真实 dt（积分用）

// 采一帧：读原始6轴 + 换算姿态 + 互补滤波 + 电机命令/实际电流/转速/错误，打时间戳 CSV 输出。
// 返回融合后的 pitch（°）。同时更新 g_lastSpeed / g_lastLateral。
static float sampleAndLog(const char *phase) {
    uint32_t now = millis();
    float dt = (g_lastSampleMs == 0) ? (TICK_MS / 1000.0f) : (now - g_lastSampleMs) / 1000.0f;
    g_lastSampleMs = now;

    float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
    imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
    double pitch = 0, roll = 0;
    imuComputeAttitude(ax, ay, az, &pitch, &roll);
    // 侧向(out-of-plane)倾角：加速度原始角(动态有伪象) → 陀螺(gx 扣零偏)互补融合，作可信安全判据。
    float lat = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.29578f;
    g_lastLateral = (float)imuFusionStepLateral(lat, gx - g_gxBias, dt);
    // 互补滤波：pitch 轴陀螺(gy 扣零偏)主导动态、加速度慢校正。
    //   动态下信陀螺（飞轮线加速度污染加速度角；陀螺仅高频振动且零均值，积分≈0）。
    g_fusedPitch = (float)imuFusionStepPitch(pitch, gy - g_gyBias, dt);

    float   cmd = motorCommandedmA();
    float   cur = motorCurrentmA();
    float   spd = motorSpeedRPM();
    float   vin = motorVoltage();
    uint8_t err = motorErrorCode();
    g_lastSpeed = spd;
    g_lastCur   = cur;
    g_lastErr   = err;

    // CSV: t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err,vin
    //   pitch=加速度原始角(动态有伪象)  pf=陀螺融合pitch  lat=陀螺融合侧向角(安全判据)
    //   vin=供电电压(诊断电机供电/掉压：满载若明显掉压=供电电流不足)
    Serial.printf("%lu,%s,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f,%u,%.2f\n",
                  (unsigned long)now, phase, ax, ay, az, gx, gy, gz,
                  pitch, g_fusedPitch, roll, g_lastLateral, cmd, cur, spd, err, vin);
    return g_fusedPitch;
}

enum DangerKind { DK_NONE, DK_INPLANE, DK_LATERAL };

// 判定危险类型。LATERAL=侧向(垂直可控轴)倾倒→不可恢复；INPLANE=面内越界/超速。
static DangerKind checkDanger(float pitch) {
    if (fabsf(g_lastLateral) > LATERAL_DANGER_DEG) return DK_LATERAL;
    if (pitch > DANGER_HI_DEG || pitch < DANGER_LO_DEG ||
        fabsf(g_lastSpeed) > SPEED_LIMIT_RPM) return DK_INPLANE;
    return DK_NONE;
}

static bool atRest(float pitch) {
    return (fabsf(pitch - REST_A_DEG) < REST_BAND_DEG) ||
           (fabsf(pitch - REST_B_DEG) < REST_BAND_DEG);
}

// 保底：试图恢复到静止态；不行就断电。返回 true=已恢复，false=已 FAULT 断电。
static bool failsafe() {
    Serial.println("# !! DANGER: 越过危险角/超速，进入保底");
    motorStop();

    if (g_restoreSign == 0) {  // 极性未知 → 无法安全反向 → 断电
        Serial.println("# !! 极性未知，无法恢复 → 断电");
        motorPowerOff();
        g_faulted = true;
        return false;
    }

    float p0  = sampleAndLog("RECOVER");
    // restoreSign = 使 pitch 减小的电流符号。翻过 B(高)→需减小→restoreSign；翻过 A(低)→需增大→-restoreSign。
    float dir = (p0 > DANGER_HI_DEG) ? (float)g_restoreSign : (float)(-g_restoreSign);

    motorSetCurrentmA(dir * RECOVER_MA);
    uint32_t t0 = millis();
    while (millis() - t0 < RECOVER_TIMEOUT_MS) {
        delay(TICK_MS);
        float p = sampleAndLog("RECOVER");
        if (fabsf(g_lastLateral) > LATERAL_DANGER_DEG) {  // 恢复中发生侧向倾倒 → 不可恢复
            motorPowerOff();
            g_faulted = true;
            Serial.println("# !! 恢复中侧向倾倒 → 断电（不可恢复）");
            return false;
        }
        if (atRest(p) && fabsf(p) < DANGER_HI_DEG && fabsf(g_lastSpeed) < SPEED_LIMIT_RPM) {
            motorStop();
            Serial.printf("# 恢复成功，pitch=%.2f\n", p);
            return true;
        }
    }

    motorPowerOff();  // 限时回不去 → 不可恢复 → 断电，防空转烧毁
    g_faulted = true;
    Serial.println("# !! 无法回到静止态 A/B → 断电（保底）");
    return false;
}

// 统一处理危险：LATERAL→立即断电(不可恢复)；INPLANE→尝试恢复。返回 true=已处理(应中止)。
static bool handleDanger(float pitch) {
    DangerKind dk = checkDanger(pitch);
    if (dk == DK_LATERAL) {
        Serial.println("# !! 侧向(垂直可控轴)倾倒，电机无 authority → 立即断电（不可恢复）");
        motorPowerOff();
        g_faulted = true;
        return true;
    }
    if (dk == DK_INPLANE) {
        failsafe();
        return true;
    }
    return false;
}

// 施加电流脉冲，期间每 tick 采样+遥测+看门狗。返回 true=正常完成；false=触发保底。
// *outPeak = 脉冲期间 pitch 相对起点的带符号峰值偏移。
static bool pulse(float mA, uint32_t ms, const char *phase, float *outPeak) {
    float start = sampleAndLog(phase);
    float peak  = 0;
    motorSetCurrentmA(mA);  // 命令一次，设备保持（不必每 tick 重发）
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        delay(TICK_MS);
        float p = sampleAndLog(phase);
        float d = p - start;
        if (fabsf(d) > fabsf(peak)) peak = d;
        if (handleDanger(p)) {
            if (outPeak) *outPeak = peak;
            return false;
        }
    }
    motorStop();
    if (outPeak) *outPeak = peak;
    return true;
}

// 稳定等待：同样采样+遥测+看门狗（捕捉回弹动态）。返回 true=正常。
static bool settle(uint32_t ms, const char *phase) {
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        delay(TICK_MS);
        float p = sampleAndLog(phase);
        if (handleDanger(p)) return false;
    }
    return true;
}

// 渐增探测一个方向，返回是否正常完成；*delta = 测到的峰值偏移。
static bool probeDir(float signMA, float *delta) {
    for (float mA = PROBE_MA; mA <= PROBE_MAX_MA + 0.1f; mA += PROBE_STEP_MA) {
        float peak = 0;
        if (!pulse(signMA * mA, PROBE_MS, "PROBE", &peak)) return false;
        Serial.printf("# probe %+.0fmA: peakDPitch=%.2f\n", signMA * mA, peak);
        *delta = peak;
        if (!settle(SETTLE_MS, "SETTLE")) return false;
        if (fabsf(peak) >= DETECT_DEG) return true;  // 已可辨
    }
    return true;
}

// 起跳阈值表征：只朝平衡方向(toward,=g_restoreSign)逐级加流找"脱离 B 阱"的电流。
//   每级：确认回到 B → 短脉冲(脉冲内若已脱阱立即切流) → 软停 → 静置让其自复位回 B。
//   用"朝平衡峰值前进度数 advance = REST_B - minPitch"判脱阱；脱阱即记录阈值并停止。
//   安全：朝平衡是安全方向(最坏落到静止态 A)；防翻保底(handleDanger)全程在线。
static void breakawayTest(int toward) {
    Serial.println("# === 起跳阈值表征：朝平衡方向逐级加流（自复位，脱阱即停）===");
    M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝：起跳表征中
    float threshMA = 0;

    for (float mA = BREAK_START_MA; mA <= BREAK_MAX_MA + 0.1f; mA += BREAK_STEP_MA) {
        // 施压前确认已回到 B 附近（自复位到位才施压，避免叠加）
        float pPre = sampleAndLog("BRK_PRE");
        if (fabsf(pPre - REST_B_DEG) > REST_BAND_DEG * 2) {
            if (!settle(BREAK_SETTLE_MS, "BRK_PRE")) return;  // 没回到 B 就再等一会
            pPre = sampleAndLog("BRK_PRE");
        }

        float    minPitch = pPre, maxSpeed = 0;
        bool     broke    = false;
        motorSetCurrentmA((float)toward * mA);  // 朝平衡方向施压
        uint32_t t0 = millis();
        while (millis() - t0 < BREAK_PULSE_MS) {
            delay(TICK_MS);
            float p = sampleAndLog("BRK");
            if (p < minPitch) minPitch = p;
            if (fabsf(g_lastSpeed) > maxSpeed) maxSpeed = fabsf(g_lastSpeed);
            if (handleDanger(p)) return;                       // 触发保底→整体中止（已断电/已恢复）
            if (REST_B_DEG - p > BREAKAWAY_DEG) { broke = true; break; }  // 脱阱→立即停推，限过冲
        }
        motorStop();

        // 脉冲后静置：捕捉是否继续朝平衡脱阱，并自复位回 B
        uint32_t t1 = millis();
        while (millis() - t1 < BREAK_SETTLE_MS) {
            delay(TICK_MS);
            float p = sampleAndLog("BRK_SET");
            if (p < minPitch) minPitch = p;
            if (handleDanger(p)) return;
        }

        float advance = REST_B_DEG - minPitch;  // 朝平衡前进度数（正=朝平衡）
        Serial.printf("# 起跳 %+.0fmA: 朝平衡前进=%.2f° 峰值转速=%.0frpm minPitch=%.2f\n",
                      (float)toward * mA, advance, maxSpeed, minPitch);
        if (broke || advance > BREAKAWAY_DEG) {
            threshMA = mA;
            Serial.printf("# => 脱离 B 阱！起跳阈值 ≈ %.0fmA（前进 %.1f° > %.0f°）。停止加流。\n",
                          mA, advance, BREAKAWAY_DEG);
            break;
        }
    }

    motorStop();
    if (threshMA == 0)
        Serial.printf("# 升至 %.0fmA 仍未脱阱：稳态电流推不动，起摆需动量泵(加速储能→急刹灌冲量)。\n",
                      BREAK_MAX_MA);
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿：表征结束
}

// 速度模式专用危险处理：速度模式下不能用电流反向恢复，遇险即 setSpeed(0)+断输出。
//   返回 true=已处理(应中止)。
static bool speedDanger(float pitch) {
    DangerKind dk = checkDanger(pitch);
    if (dk == DK_NONE) return false;
    Serial.printf("# !! 速度模式危险(kind=%d pitch=%.1f lat=%.1f spd=%.0f) → setSpeed0 + 断电\n",
                  (int)dk, pitch, g_lastLateral, g_lastSpeed);
    motorSetSpeedRPM(0);
    motorPowerOff();
    g_faulted = true;
    return true;
}

// 速度模式版稳定等待（用 speedDanger 而非电流模式的 handleDanger）。
static bool settleSpeedSafe(uint32_t ms, const char *phase) {
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        delay(TICK_MS);
        float p = sampleAndLog(phase);
        if (speedDanger(p)) return false;
    }
    return true;
}

int dirTestRestoreCurrentSign() {
    return g_restoreSign;
}

// 共用开局：打表头 → 标定陀螺零偏(点阵橙) → 播种互补滤波 → 静置 3s → 校验在 B。
//   返回 true=校准成功且在静止态 B；false=已 ABORT/断电（调用方应直接 return）。
static bool prepAtRestB() {
    Serial.println("# t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err,vin");
    Serial.println("# 请确认系统处于静止态 B，3 秒后开始...");
    M5.dis.fillpix(CRGB(0x30, 0x18, 0x00));  // 橙：正在校准
    if (!imuCalibrateGyro(200)) {
        M5.dis.fillpix(CRGB(0x40, 0x00, 0x00));  // 红：校准失败
        Serial.println("# ABORT：陀螺标定多次检测到运动，未取得静止窗 → 不驱动电机。");
        motorPowerOff();
        return false;
    }
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿：校准完成
    {
        float bx = 0, by = 0, bz = 0;
        imuGyroBias(&bx, &by, &bz);
        g_gyBias = by;  // pitch 轴 = gy
        g_gxBias = bx;  // lateral 轴 = gx
        Serial.printf("# 陀螺零偏: gx=%.3f gy=%.3f gz=%.3f °/s（pitch扣gy, 侧向扣gx）\n", bx, by, bz);

        float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
        imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
        double p = 0, r = 0;
        imuComputeAttitude(ax, ay, az, &p, &r);
        float lat0 = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.29578f;
        imuFusionInit(p, FUSION_ALPHA);              // pitch 互补滤波播种
        imuFusionInitLateral(lat0, FUSION_ALPHA);    // lateral 互补滤波播种
        g_fusedPitch  = (float)p;
        g_lastLateral = lat0;
    }
    for (int i = 0; i < 3000 / (int)TICK_MS; i++) { delay(TICK_MS); sampleAndLog("WAIT"); }

    float p0 = sampleAndLog("CHECK");
    Serial.printf("# 起始 pitch=%.2f（预期 ~%.1f）\n", p0, REST_B_DEG);
    if (fabsf(p0 - REST_B_DEG) > REST_BAND_DEG) {
        Serial.println("# ABORT：不在静止态 B。");
        motorPowerOff();
        return false;
    }
    return true;
}

void dirTestRun() {
    if (!prepAtRestB()) return;

    // 正向、反向渐增探测
    float dPos = 0, dNeg = 0;
    Serial.println("# 正向探测：");
    if (!probeDir(+1.0f, &dPos)) return;
    if (!settle(SETTLE_MS, "SETTLE")) return;
    Serial.println("# 反向探测：");
    if (!probeDir(-1.0f, &dNeg)) return;

    // 判定：哪个电流符号使 pitch 减小（朝平衡）
    if (dPos < -DETECT_DEG / 2 && dNeg > DETECT_DEG / 2) {
        g_restoreSign = +1;
    } else if (dNeg < -DETECT_DEG / 2 && dPos > DETECT_DEG / 2) {
        g_restoreSign = -1;
    } else {
        Serial.printf("# 结果不明确（dPos=%.2f dNeg=%.2f），停机。\n", dPos, dNeg);
        motorStop();
        motorPowerOff();
        return;
    }
    Serial.printf("# => 朝平衡(pitch 减小)的电流符号 = %+d\n", g_restoreSign);

    // 确认：朝恢复方向小幅，验证 pitch 确实减小
    float peak = 0;
    if (!pulse((float)g_restoreSign * CONFIRM_MA, PROBE_MS, "CONFIRM", &peak)) return;
    Serial.printf("# 确认脉冲：peakDPitch=%.2f（预期 <0）\n", peak);
    if (!settle(SETTLE_MS, "SETTLE")) return;
    Serial.printf("# 方向辨识完成，朝平衡电流符号 = %+d。\n", g_restoreSign);

    // 接力：起跳阈值表征（朝平衡方向加流找脱阱点）
    breakawayTest(g_restoreSign);

    motorStop();
    Serial.println("# 全部测试完成，电机已软停。");
}

// 速度模式 authority 测试：逐级目标转速，看实际峰值转速/电流/机体摆幅，对比电流模式(~1.5°)。
//   toward=-1：负转速=朝平衡（与电流模式 restoreSign=-1 同向，已 3 次实测确认）。
//   安全：横滚/侧向沿用保守阈值(≤34/15，均在用户 42° 上限内)；遇险 setSpeed0+断电。
void speedModeTest() {
    if (!prepAtRestB()) return;
    const int      toward      = -1;
    const float    targets[]   = {500, 1000, 1500, 2000, 3000};
    const uint32_t SPD_PULSE_MS = 400;  // 速度模式给更长加速时间观察顶速

    Serial.println("# === 速度模式 authority 测试：逐级目标转速，对比扭矩/摆幅 ===");
    M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝

    for (unsigned i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
        float tgt = targets[i];
        // 施压前确认回到 B（自复位到位）
        float pPre = sampleAndLog("SPD_PRE");
        if (fabsf(pPre - REST_B_DEG) > REST_BAND_DEG * 2) {
            if (!settleSpeedSafe(BREAK_SETTLE_MS, "SPD_PRE")) return;
            pPre = sampleAndLog("SPD_PRE");
        }

        float minPitch = pPre, maxPitch = pPre, maxSpeed = 0, maxCur = 0;
        bool  broke    = false;
        motorSetSpeedRPM((float)toward * tgt);  // 朝平衡方向目标转速
        uint32_t t0 = millis();
        while (millis() - t0 < SPD_PULSE_MS) {
            delay(TICK_MS);
            float p = sampleAndLog("SPD");
            if (p < minPitch) minPitch = p;
            if (p > maxPitch) maxPitch = p;
            if (fabsf(g_lastSpeed) > maxSpeed) maxSpeed = fabsf(g_lastSpeed);
            if (fabsf(g_lastCur)   > maxCur)   maxCur   = fabsf(g_lastCur);
            if (speedDanger(p)) return;
            if (pPre - p > BREAKAWAY_DEG) { broke = true; break; }  // 脱阱→停推
        }
        motorSetSpeedRPM(0);  // 主动刹到 0

        uint32_t t1 = millis();
        while (millis() - t1 < BREAK_SETTLE_MS) {
            delay(TICK_MS);
            float p = sampleAndLog("SPD_SET");
            if (p < minPitch) minPitch = p;
            if (speedDanger(p)) return;
        }

        float adv  = pPre - minPitch;   // 朝平衡前进度数（>0）
        float away = maxPitch - pPre;   // 朝危险(B 外棱)偏移（>0 表示方向反了）
        Serial.printf("# 速度目标 %+.0frpm: 峰值转速=%.0frpm 峰值电流=%.0fmA 朝平衡前进=%.2f° 反向偏移=%.2f°\n",
                      (float)toward * tgt, maxSpeed, maxCur, adv, away);
        if (broke || adv > BREAKAWAY_DEG) {
            Serial.printf("# => 速度模式脱阱！目标 %.0frpm（前进 %.1f°）。停止。\n", tgt, adv);
            break;
        }
    }

    motorSetSpeedRPM(0);
    motorPowerOff();
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿
    Serial.println("# 全部测试完成，电机已断电。");
}

// 上电(XT30 高压)专用危险处理：真力矩下**只断电、不主动反向恢复**（避免无人值守时反向甩动失控）。
//   过压(err=1)同样立即断电。返回 true=已断电应中止。
static bool poweredDangerStop(float pitch) {
    DangerKind dk = checkDanger(pitch);
    if (dk == DK_NONE && g_lastErr != 1) return false;
    Serial.printf("# !! 危险(kind=%d err=%u pitch=%.1f lat=%.1f spd=%.0f) → 立即断电\n",
                  (int)dk, g_lastErr, pitch, g_lastLateral, g_lastSpeed);
    motorStop();
    motorPowerOff();
    g_faulted = true;
    return true;
}

// 上电谨慎起跳表征（电流模式，XT30 接 12-15V 后用）：
//   仅朝平衡方向(已知 -1)，从极小电流 50mA 起步、步进 50、短脉冲、脱阱即停、自复位；
//   遇险/过压只断电；含上电 Vin/错误自检。真力矩下尽量小冲量找阈值，降低过冲翻越风险。
void poweredBreakawayTest() {
    if (!prepAtRestB()) return;

    float   vin  = motorVoltage();
    uint8_t err0 = motorErrorCode();
    Serial.printf("# 上电自检: Vin=%.2fV err=%u\n", vin, err0);
    if (err0 == 1) { Serial.println("# ABORT: 电机过压(E:1) → 断电"); motorPowerOff(); return; }
    if (vin < 6.0f) Serial.println("# 注意: Vin<6V，XT30 高压似乎未接入；仍按低功率谨慎进行。");

    const int      toward    = -1;            // 负电流朝平衡（3 次实测确认）
    const float    PB_START  = 50.0f;         // 真力矩下从极小起步
    const float    PB_STEP   = 50.0f;
    const uint32_t PB_PULSE  = 150;           // 更短脉冲，限单次冲量
    const uint32_t PB_SETTLE = 1200;
    const float    PB_BRK    = 8.0f;          // 朝平衡前进超此=脱阱（比 12 更早切，限过冲）

    Serial.println("# === 上电起跳表征(谨慎)：朝平衡 50→1200mA，脱阱即停，遇险断电 ===");
    M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝
    float thr = 0;

    for (float mA = PB_START; mA <= MOTOR_MAX_MA + 0.1f; mA += PB_STEP) {
        float pPre = sampleAndLog("PB_PRE");
        if (fabsf(pPre - REST_B_DEG) > REST_BAND_DEG * 2) {  // 没回到 B 先等自复位
            uint32_t t = millis();
            while (millis() - t < PB_SETTLE) {
                delay(TICK_MS);
                float p = sampleAndLog("PB_PRE");
                if (poweredDangerStop(p)) return;
            }
            pPre = sampleAndLog("PB_PRE");
        }

        float minPitch = pPre, maxSpd = 0, maxCur = 0;
        bool  broke    = false;
        motorSetCurrentmA((float)toward * mA);
        uint32_t t0 = millis();
        while (millis() - t0 < PB_PULSE) {
            delay(TICK_MS);
            float p = sampleAndLog("PB");
            if (p < minPitch) minPitch = p;
            if (fabsf(g_lastSpeed) > maxSpd) maxSpd = fabsf(g_lastSpeed);
            if (fabsf(g_lastCur)   > maxCur) maxCur = fabsf(g_lastCur);
            if (poweredDangerStop(p)) return;
            if (pPre - p > PB_BRK) { broke = true; break; }  // 脱阱→立即停推
        }
        motorStop();

        uint32_t t1 = millis();
        while (millis() - t1 < PB_SETTLE) {
            delay(TICK_MS);
            float p = sampleAndLog("PB_SET");
            if (p < minPitch) minPitch = p;
            if (poweredDangerStop(p)) return;
        }

        float adv = pPre - minPitch;
        Serial.printf("# 起跳 %+.0fmA: 朝平衡前进=%.2f° 峰值转速=%.0frpm 峰值电流=%.0fmA Vin=%.2f\n",
                      (float)toward * mA, adv, maxSpd, maxCur, motorVoltage());
        if (broke || adv > PB_BRK) {
            thr = mA;
            Serial.printf("# => 脱阱！起跳阈值 ≈ %.0fmA（前进 %.1f°）。停止。\n", mA, adv);
            break;
        }
    }

    motorStop();
    motorPowerOff();
    if (thr == 0) Serial.printf("# 升至 %.0fmA 仍未脱阱。\n", MOTOR_MAX_MA);
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿
    Serial.println("# 全部测试完成，电机已断电。");
}

// ===== 电机台架能力测试（测最大电流/最大转速，不做起跳）=====
static float g_benchStart = 0;  // 台架测试起始 pitch（看门狗按相对起点的偏移，不依赖绝对 B）

// 台架看门狗：过压 / 超速 / 侧向越界 / 机体相对起点偏移过大 → 断电（不依赖机体在 B）。
static bool benchDangerStop(float pitch) {
    bool bad = (g_lastErr == 1) ||
               (fabsf(g_lastSpeed)   > SPEED_LIMIT_RPM) ||
               (fabsf(g_lastLateral) > LATERAL_DANGER_DEG) ||
               (fabsf(pitch - g_benchStart) > 25.0f);
    if (!bad) return false;
    Serial.printf("# !! 台架危险(err=%u spd=%.0f lat=%.1f dPitch=%.1f) → 断电\n",
                  g_lastErr, g_lastSpeed, g_lastLateral, pitch - g_benchStart);
    motorStop();
    motorPowerOff();
    g_faulted = true;
    return true;
}

// 一个能力脉冲：给 mA、持续 ms，记录峰值实际电流/转速；遇险即断电中止。返回 false=已中止。
static bool benchBurst(float mA, uint32_t ms, const char *phase, float *peakCur, float *peakSpd) {
    *peakCur = 0; *peakSpd = 0;
    motorSetCurrentmA(mA);
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        delay(TICK_MS);
        float p = sampleAndLog(phase);
        if (fabsf(g_lastCur)   > *peakCur) *peakCur = fabsf(g_lastCur);
        if (fabsf(g_lastSpeed) > *peakSpd) *peakSpd = fabsf(g_lastSpeed);
        if (benchDangerStop(p)) return false;
    }
    motorStop();
    return true;
}

// 台架静置（电流模式，用 benchDangerStop 看门狗）。返回 false=已断电中止。
static bool benchSettle(uint32_t ms, const char *phase) {
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        delay(TICK_MS);
        float p = sampleAndLog(phase);
        if (benchDangerStop(p)) return false;
    }
    return true;
}

void motorBenchTest() {
    // 轻量开局：标定陀螺(点阵橙)+播种融合+Vin 自检（**不要求机体在 B**，只测电机能力）
    Serial.println("# t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err,vin");
    Serial.println("# 电机台架能力测试(12V)：测最大电流/最大转速，不做起跳；机体受硬线约束，仅看电机读数。");
    M5.dis.fillpix(CRGB(0x30, 0x18, 0x00));  // 橙：校准
    if (!imuCalibrateGyro(200)) {
        M5.dis.fillpix(CRGB(0x40, 0x00, 0x00));
        Serial.println("# ABORT：陀螺标定检测到运动 → 断电。");
        motorPowerOff();
        return;
    }
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿
    {
        float bx = 0, by = 0, bz = 0;
        imuGyroBias(&bx, &by, &bz);
        g_gyBias = by; g_gxBias = bx;
        float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
        imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
        double p = 0, r = 0;
        imuComputeAttitude(ax, ay, az, &p, &r);
        float lat0 = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.29578f;
        imuFusionInit(p, FUSION_ALPHA);
        imuFusionInitLateral(lat0, FUSION_ALPHA);
        g_fusedPitch = (float)p; g_lastLateral = lat0; g_benchStart = (float)p;
    }
    float vin = motorVoltage();
    uint8_t err0 = motorErrorCode();
    Serial.printf("# 上电自检: Vin=%.2fV err=%u（预期 ~12V；>18V 会 E:1 过压）\n", vin, err0);
    if (err0 == 1) { Serial.println("# ABORT：电机过压(E:1) → 断电。"); motorPowerOff(); return; }

    M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝：测试中

    // 1) 电流能力扫频：交替方向短脉冲，看各档实际能拉到多大电流、飞轮转多快
    const float levels[] = {200, 400, 600, 800, 1000, 1200};
    float maxCurOverall = 0, maxSpdOverall = 0;
    for (unsigned i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        float sign = (i % 2 == 0) ? +1.0f : -1.0f;  // 交替，减少机体净推
        float pc = 0, ps = 0;
        if (!benchBurst(sign * levels[i], 250, "BENCH_I", &pc, &ps)) return;
        if (pc > maxCurOverall) maxCurOverall = pc;
        if (ps > maxSpdOverall) maxSpdOverall = ps;
        Serial.printf("# 电流档 %+.0fmA: 实际峰值电流=%.0fmA 峰值转速=%.0frpm\n", sign * levels[i], pc, ps);
        if (!benchSettle(700, "BENCH_S")) return;  // 飞轮滑行 + 机体回稳
    }

    // 2) 最大转速：满电流较长脉冲测平台转速
    float pc = 0, ps = 0;
    if (!benchBurst(+MOTOR_MAX_MA, 800, "BENCH_V", &pc, &ps)) return;
    if (pc > maxCurOverall) maxCurOverall = pc;
    if (ps > maxSpdOverall) maxSpdOverall = ps;
    Serial.printf("# 最大转速测试 @+1200mA: 峰值转速=%.0frpm 峰值电流=%.0fmA\n", ps, pc);

    motorStop();
    motorPowerOff();
    Serial.printf("# === 12V 台架结论: 最大实际电流≈%.0fmA, 最大飞轮转速≈%.0frpm（对比 4.5V 时 ~85mA/~600rpm）===\n",
                  maxCurOverall, maxSpdOverall);
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿
    Serial.println("# 全部测试完成，电机已断电。");
}
