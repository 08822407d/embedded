#include <Arduino.h>
#include <math.h>

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
static const float SPEED_LIMIT_RPM = 2000.0f;  // 飞轮转速封顶，超则保底
static const float LATERAL_DANGER_DEG = 15.0f; // 侧向(垂直可控轴)倾角阈值：电机无 authority，超过=不可恢复
static const float LPF_ALPHA          = 0.6f;  // 加速度 pitch 低通系数（抑制飞轮振动伪象）

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
static float g_lastSpeed   = 0;   // 最近一次读到的飞轮转速（看门狗用）
static float g_lastLateral = 0;   // 最近一次侧向(out-of-plane)倾角（看门狗用）
static float g_lpfPitch    = 0;   // 低通后的 pitch（看门狗/判向用，抗振动伪象）

// 采一帧：读原始6轴 + 换算姿态 + 电机命令/实际电流/转速/错误，打时间戳 CSV 输出。
// 返回 pitch（°）。同时更新 g_lastSpeed。
static float sampleAndLog(const char *phase) {
    float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
    imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
    double pitch = 0, roll = 0;
    imuComputeAttitude(ax, ay, az, &pitch, &roll);
    // 侧向(out-of-plane)倾角：正常运行时 ~0；机体往垂直可控轴方向倾倒时增大。
    float lat = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.29578f;
    g_lastLateral = lat;
    // 低通后的 pitch：抑制飞轮振动造成的尖刺，作看门狗/判向的可靠信号
    g_lpfPitch = LPF_ALPHA * g_lpfPitch + (1.0f - LPF_ALPHA) * (float)pitch;

    float   cmd = motorCommandedmA();
    float   cur = motorCurrentmA();
    float   spd = motorSpeedRPM();
    uint8_t err = motorErrorCode();
    g_lastSpeed = spd;

    // CSV: t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err
    //   pitch=加速度原始角(动态下有振动伪象)  pf=低通后角(看门狗/判向用)
    Serial.printf("%lu,%s,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f,%u\n",
                  (unsigned long)millis(), phase, ax, ay, az, gx, gy, gz,
                  pitch, g_lpfPitch, roll, lat, cmd, cur, spd, err);
    return g_lpfPitch;
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

int dirTestRestoreCurrentSign() {
    return g_restoreSign;
}

void dirTestRun() {
    // 遥测 CSV 表头（# 开头的行均为说明，非数据）
    Serial.println("# t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err");
    Serial.println("# 方向辨识：请确认系统处于静止态 B，3 秒后开始...");
    // 用首帧加速度角播种低通滤波器
    {
        float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
        imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
        double p = 0, r = 0;
        imuComputeAttitude(ax, ay, az, &p, &r);
        g_lpfPitch = (float)p;
    }
    for (int i = 0; i < 3000 / (int)TICK_MS; i++) { delay(TICK_MS); sampleAndLog("WAIT"); }

    // 前置检查：必须从静止态 B 附近开始
    float p0 = sampleAndLog("CHECK");
    Serial.printf("# 起始 pitch=%.2f（预期 ~%.1f）\n", p0, REST_B_DEG);
    if (fabsf(p0 - REST_B_DEG) > REST_BAND_DEG) {
        Serial.println("# ABORT：不在静止态 B，不进行探测。");
        motorPowerOff();
        return;
    }

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

    motorStop();
    Serial.println("# 方向辨识完成，电机已软停。");
}
