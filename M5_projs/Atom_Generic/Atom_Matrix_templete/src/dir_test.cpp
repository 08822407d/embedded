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
static float    g_lastGyRate  = 0;   // 最近一次 pitch 轴角速度(gy 扣零偏, °/s)——起摆刹车判停用
static float    g_accelPitch   = 0;   // 最近一帧**加速度计原始** pitch（°，无积分漂移，静止态判别用）
static float    g_accelLat     = 0;   // 最近一帧**加速度计原始**侧向角（°，无积分漂移）
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
    g_accelPitch = (float)pitch;  // 加速度原始姿态（无漂移，供静止态窗口判别）
    g_accelLat   = lat;
    g_lastLateral = (float)imuFusionStepLateral(lat, gx - g_gxBias, dt);
    // 互补滤波：pitch 轴陀螺(gy 扣零偏)主导动态、加速度慢校正。
    //   动态下信陀螺（飞轮线加速度污染加速度角；陀螺仅高频振动且零均值，积分≈0）。
    g_lastGyRate = gy - g_gyBias;  // pitch 轴角速度(°/s)，刹车判停用
    g_fusedPitch = (float)imuFusionStepPitch(pitch, g_lastGyRate, dt);

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

// 速度模式硬危险（decision 006·A）：**硬断电只看飞轮"管不了"的两件事**——
//   ① 出平面/横向倾倒(|lat|>阈，飞轮无 authority、不可控)  ② 飞轮超速(runaway)。
//   **面内(pitch)越外棱不在此硬断电**：交给"消能/动量接住"预防、万一翻越交面内击杀自救。
//   返回 true=已断电(应中止)。
static bool speedDanger(float pitch) {
    // decision 006(用户简化)：**超出平衡 40° 即视为翻越/不可恢复 → 立即断电，只读监视、不再自救**。
    //   外加横向倾倒、飞轮超速两道硬保底。任一触发 → setSpeed0 + 断输出。
    bool overPit = fabsf(pitch)         > 40.0f;               // 超平衡 40° → 翻越外棱、不可恢复
    bool lateral = fabsf(g_lastLateral) > LATERAL_DANGER_DEG;  // 出平面倾倒(飞轮不可控)
    bool overspd = fabsf(g_lastSpeed)   > SPEED_LIMIT_RPM;     // 飞轮 runaway
    if (!overPit && !lateral && !overspd) return false;
    Serial.printf("# !! 硬危险(%s pitch=%.1f lat=%.1f spd=%.0f) → 断电、仅监视(不自救)\n",
                  overPit ? "超平衡40°" : (lateral ? "横向倾倒" : "飞轮超速"), pitch, g_lastLateral, g_lastSpeed);
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

// 共用开局：打表头 → 标定陀螺零偏(点阵橙) → 播种互补滤波 → 静置 3s → 识别静止态。
//   返回 +1=在静止态 B(+30°)，-1=在静止态 A(-31°)，0=校准失败/不在 A/B（已 ABORT/断电，调用方直接 return）。
static int prepAtRest() {
    Serial.println("# t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err,vin");
    Serial.println("# 请确认系统处于静止态 A 或 B，3 秒后开始...");
    M5.dis.fillpix(CRGB(0x30, 0x18, 0x00));  // 橙：正在校准
    if (!imuCalibrateGyro(200)) {
        M5.dis.fillpix(CRGB(0x40, 0x00, 0x00));  // 红：校准失败
        Serial.println("# ABORT：陀螺标定多次检测到运动，未取得静止窗 → 不驱动电机。");
        motorPowerOff();
        return 0;
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
    if (fabsf(p0 - REST_B_DEG) < REST_BAND_DEG) {
        Serial.printf("# 起始 pitch=%.2f → 静止态 B(+%.0f°)\n", p0, REST_B_DEG);
        return +1;
    }
    if (fabsf(p0 - REST_A_DEG) < REST_BAND_DEG) {
        Serial.printf("# 起始 pitch=%.2f → 静止态 A(%.0f°)\n", p0, REST_A_DEG);
        return -1;
    }
    Serial.printf("# ABORT：不在静止态 A/B（pitch=%.2f）。\n", p0);
    motorPowerOff();
    return 0;
}

void dirTestRun() {
    if (prepAtRest() != +1) {  // 方向辨识固定从 B 起
        Serial.println("# （方向辨识需从静止态 B 开始）");
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
    if (prepAtRest() != +1) { motorPowerOff(); return; }  // 速度模式测试固定从 B 起
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

// 危险消抖：角度类危险需**连续 DANGER_NEED 次**越界才确认，滤掉飞轮振动造成的单帧假尖峰；
//   过压立即确认（硬故障）。配合片上 DLPF 双保险。
static int       g_dangerStreak = 0;
static const int DANGER_NEED    = 3;  // 连续 3 帧(=60ms)越界才算真危险

// 上电(XT30 高压)专用危险处理：真力矩下**只断电、不主动反向恢复**（避免无人值守时反向甩动失控）。
//   含消抖；过压(err=1)立即断电。返回 true=已断电应中止。
static bool poweredDangerStop(float pitch) {
    if (g_lastErr == 1) {  // 过压：硬故障，立即
        Serial.println("# !! 过压(E:1) → 立即断电");
        motorStop(); motorPowerOff(); g_faulted = true; return true;
    }
    DangerKind dk = checkDanger(pitch);
    if (dk == DK_NONE) { g_dangerStreak = 0; return false; }
    if (++g_dangerStreak < DANGER_NEED) return false;  // 还没连够 → 疑似振动尖峰，先不动
    Serial.printf("# !! 危险确认(kind=%d 连%d帧 pitch=%.1f lat=%.1f spd=%.0f) → 断电\n",
                  (int)dk, g_dangerStreak, pitch, g_lastLateral, g_lastSpeed);
    motorStop();
    motorPowerOff();
    g_faulted = true;
    return true;
}

// 上电谨慎起跳表征（电流模式，XT30 接 12-15V 后用）：
//   **自动识别在 A 还是 B**，朝平衡方向施力（B→负电流、A→正电流）；从极小 50mA 起步、步进 50、
//   短脉冲、脱阱即停、自复位；遇险/过压只断电；含上电 Vin/错误自检。真力矩下小冲量找阈值，降低过冲翻越风险。
//   advance(朝平衡前进) = st·(restAngle − pitch)，st=+1@B / −1@A；脱阱后机体落到另一态，可连续测。
void poweredBreakawayTest() {
    int st = prepAtRest();
    if (st == 0) return;                                  // 校准失败/不在 A/B（已断电）
    const float restAngle = (st == +1) ? REST_B_DEG : REST_A_DEG;
    const int   toward    = -st;                          // B(+1)→-1电流; A(-1)→+1电流，均朝平衡

    float   vin  = motorVoltage();
    uint8_t err0 = motorErrorCode();
    Serial.printf("# 上电自检: Vin=%.2fV err=%u（起始态=%s, 朝平衡电流符号=%+d）\n",
                  vin, err0, (st == +1) ? "B" : "A", toward);
    if (err0 == 1) { Serial.println("# ABORT: 电机过压(E:1) → 断电"); motorPowerOff(); return; }
    if (vin < 6.0f) Serial.println("# 注意: Vin<6V，XT30 高压似乎未接入；仍按低功率谨慎进行。");

    const float    PB_START  = 50.0f;         // 真力矩下从极小起步
    const float    PB_STEP   = 50.0f;
    const uint32_t PB_PULSE  = 150;           // 更短脉冲，限单次冲量
    const uint32_t PB_SETTLE = 1200;
    const float    PB_BRK    = 8.0f;          // 朝平衡前进超此=脱阱（早切，限过冲）

    Serial.println("# === 上电起跳表征(谨慎)：朝平衡 50→1200mA，脱阱即停，遇险断电 ===");
    M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝
    float thr = 0;

    for (float mA = PB_START; mA <= MOTOR_MAX_MA + 0.1f; mA += PB_STEP) {
        float pPre = sampleAndLog("PB_PRE");
        if (fabsf(pPre - restAngle) > REST_BAND_DEG * 2) {  // 没回到起始态先等自复位
            uint32_t t = millis();
            while (millis() - t < PB_SETTLE) {
                delay(TICK_MS);
                float p = sampleAndLog("PB_PRE");
                if (poweredDangerStop(p)) return;
            }
            pPre = sampleAndLog("PB_PRE");
        }

        float maxAdv = 0, maxSpd = 0, maxCur = 0;
        bool  broke  = false;
        motorSetCurrentmA((float)toward * mA);
        uint32_t t0 = millis();
        while (millis() - t0 < PB_PULSE) {
            delay(TICK_MS);
            float p   = sampleAndLog("PB");
            float adv = (float)st * (restAngle - p);  // 朝平衡前进（>0）
            if (adv > maxAdv) maxAdv = adv;
            if (fabsf(g_lastSpeed) > maxSpd) maxSpd = fabsf(g_lastSpeed);
            if (fabsf(g_lastCur)   > maxCur) maxCur = fabsf(g_lastCur);
            if (poweredDangerStop(p)) return;
            if (maxAdv > PB_BRK) { broke = true; break; }  // 脱阱→立即停推
        }
        motorStop();

        uint32_t t1 = millis();
        while (millis() - t1 < PB_SETTLE) {
            delay(TICK_MS);
            float p   = sampleAndLog("PB_SET");
            float adv = (float)st * (restAngle - p);
            if (adv > maxAdv) maxAdv = adv;
            if (poweredDangerStop(p)) return;
        }

        Serial.printf("# 起跳 %+.0fmA: 朝平衡前进=%.2f° 峰值转速=%.0frpm 峰值电流=%.0fmA Vin=%.2f\n",
                      (float)toward * mA, maxAdv, maxSpd, maxCur, motorVoltage());
        if (broke || maxAdv > PB_BRK) {
            thr = mA;
            Serial.printf("# => 脱阱！起跳阈值 ≈ %.0fmA（前进 %.1f°）。停止。\n", mA, maxAdv);
            break;
        }
    }

    motorStop();
    motorPowerOff();
    if (thr == 0) Serial.printf("# 升至 %.0fmA 仍未脱阱。\n", MOTOR_MAX_MA);
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿
    Serial.println("# 全部测试完成，电机已断电。");
}

// ===== 起跳策略（可更换）=====
SwingUpStrategy g_swingUpStrategy = SWINGUP_CUBLI;  // 默认 Cubli
void swingUpSetStrategy(SwingUpStrategy s) { g_swingUpStrategy = s; }

// 表征结果：使机体朝平衡起跳所需的**飞轮"速度变化方向"**（+1/-1），0=未测出。
int g_swingUpDir = 0;
int swingUpDirection() { return g_swingUpDir; }
static const char *swingUpName(SwingUpStrategy s) {
    switch (s) { case SWINGUP_IMPULSE: return "突然启动单次冲量";
                 case SWINGUP_PUMP:    return "震荡式蓄能(秋千)";
                 case SWINGUP_SPEED:   return "速度模式起跳+动量接住缓落";
                 default:              return "Cubli蓄能急刹"; }
}

// ---- 策略1：Cubli 蓄能急刹（默认）----
// 教训(run_cubli2)：满蓄能1600rpm+全力起跳 → 机体以1261°/s越平衡、冲过对面+38.5°**翻倒**。
//   ⇒ 起跳能量必须**远小**：蓄能转速**逐级加大**找最温和越平衡，越平衡即停加能并**立即反向急刹防翻**。
//   方向：driveSign=-st(起跳)、spinSign=+st(反向蓄能/越平衡后急刹)。落点精准本质需平衡控制器接住。
static void swingUpCubli(int st) {
    const float startRest = (st == +1) ? REST_B_DEG : REST_A_DEG;
    const int   driveSign = -st;
    const int   spinSign  = +st;

    const float    SPIN_MA   = 140.0f;        // 蓄能电流（<挣脱200，机体留在起始态）
    const float    HOLD_TOL  = 4.0f;
    const float    KICK_MA   = MOTOR_MAX_MA;  // 起跳全力（越平衡瞬间即停，速度由蓄能量决定）
    const uint32_t SPIN_TO   = 6000;
    const uint32_t KICK_TO   = 1000;
    const uint32_t MANAGE_TO = 2500;
    const float    GY_STOP   = 12.0f;
    // 蓄能转速从低到高（无蓄能的单次自旋只到平衡前 9°，故只需补一点点）：找最温和的越平衡
    const float    spinTargets[] = {200, 300, 400, 550, 700, 900};

    bool crossedOnce = false;
    for (unsigned i = 0; i < sizeof(spinTargets) / sizeof(spinTargets[0]) && !crossedOnce; i++) {
        float spinTgt = spinTargets[i];

        // 回到起始态（自复位）
        float pPre = sampleAndLog("CU_PRE");
        if (fabsf(pPre - startRest) > REST_BAND_DEG * 2) {
            uint32_t t = millis();
            while (millis() - t < 1500) { delay(TICK_MS); float p = sampleAndLog("CU_PRE"); if (poweredDangerStop(p)) return; }
            pPre = sampleAndLog("CU_PRE");
            if (fabsf(pPre - startRest) > REST_BAND_DEG * 2) { Serial.printf("# 未回到起始态(%.1f) 跳过档%u\n", pPre, i); continue; }
        }

        // 蓄能（反向，小电流，机体留在起始态）
        Serial.printf("# [档%u] 蓄能 %+.0fmA → %.0frpm\n", i, spinSign * SPIN_MA, spinTgt);
        M5.dis.fillpix(CRGB(0x20, 0x10, 0x00));  // 橙
        motorSetCurrentmA((float)spinSign * SPIN_MA);
        uint32_t t0 = millis();
        while (millis() - t0 < SPIN_TO) {
            delay(TICK_MS);
            float p = sampleAndLog("CU_SPIN");
            if (poweredDangerStop(p)) return;
            if (fabsf(p - startRest) > HOLD_TOL) { motorStop(); motorPowerOff(); Serial.printf("# 蓄能机体离位(%.1f)中止\n", p); return; }
            if (fabsf(g_lastSpeed) >= spinTgt) break;
        }

        // 起跳：全力正向，越平衡即停加能
        Serial.printf("# [档%u] 起跳 %+.0fmA（飞轮%.0frpm）\n", i, driveSign * KICK_MA, g_lastSpeed);
        M5.dis.fillpix(CRGB(0x30, 0x00, 0x00));  // 红
        motorSetCurrentmA((float)driveSign * KICK_MA);
        bool  crossed = false;
        float closest = startRest;
        uint32_t t1 = millis();
        while (millis() - t1 < KICK_TO) {
            delay(TICK_MS);
            float p = sampleAndLog("CU_KICK");
            if ((float)st * (startRest - p) > (float)st * (startRest - closest)) closest = p;
            if (poweredDangerStop(p)) return;
            if ((float)st * p < 0) { crossed = true; break; }        // 越过平衡
            if ((float)driveSign * g_lastGyRate <= 0) break;         // 顶点（未越过）
        }
        motorStop();  // 立即停加能

        if (!crossed) {
            // 未越过 → 回落侧减速防过冲 + 自复位，再加大蓄能
            Serial.printf("# [档%u] 未越平衡（最高 %.1f）。回落侧减速后加大蓄能。\n", i, closest);
            M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));
            motorSetCurrentmA((float)driveSign * 600.0f);  // 中等减速回落，防冲过起始态外棱
            uint32_t t2 = millis();
            while (millis() - t2 < MANAGE_TO) {
                delay(TICK_MS); float p = sampleAndLog("CU_CATCH");
                if (poweredDangerStop(p)) return;
                if (fabsf(p - startRest) < REST_BAND_DEG && fabsf(g_lastGyRate) < GY_STOP) break;
            }
            motorStop();
            continue;  // 加大蓄能
        }

        // 越过平衡 → **立即全力反向急刹**（防翻优先；上次实测立即急刹能拦住、coast 则翻倒）
        crossedOnce = true;
        Serial.printf("# ★[档%u] 越过平衡(closest=%.1f, 蓄能%.0frpm)！立即反向急刹防冲过对面/翻倒。\n", i, closest, spinTgt);
        M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝
        motorSetCurrentmA((float)spinSign * KICK_MA);
        float farPeak = 0;
        uint32_t t2 = millis();
        while (millis() - t2 < MANAGE_TO) {
            delay(TICK_MS);
            float p = sampleAndLog("CU_FAR");
            if ((float)st * p < (float)st * farPeak) farPeak = p;
            if (poweredDangerStop(p)) return;
            if ((float)driveSign * g_lastGyRate <= GY_STOP) break;  // 摆停
        }
        motorStop();
        Serial.printf("# 冲对面峰值 pitch=%.1f\n", farPeak);
    }

    // 落定观察
    uint32_t ts = millis();
    float pe = 0;
    while (millis() - ts < 1500) { delay(TICK_MS); float p = sampleAndLog("CU_SETTLE"); pe = p; if (poweredDangerStop(p)) return; }
    motorStop();
    Serial.printf("# Cubli起跳结束：%s，最终落于 %.1f。\n", crossedOnce ? "已安全越过平衡" : "未越过平衡", pe);
}

// ---- 策略2：突然启动电机单次冲量（早期方案）----
// 直接全幅施加驱动电流（无蓄能），逐级加大单次冲量找最小越平衡值；越平衡立即反向急刹、未越则回落侧减速。
//   受单次飞轮动量限制（实测单次只到平衡前 ~9°），多半越不过——保留作对比/弱机动用。
static void swingUpImpulse(int st) {
    const float startRest = (st == +1) ? REST_B_DEG : REST_A_DEG;
    const int   driveSign = -st;
    const int   spinSign  = +st;
    const float    SU_START = 300.0f, SU_STEP = 100.0f, SU_MAX = 900.0f;
    const uint32_t SU_DRIVE_TO = 700, MANAGE_TO = 2000;
    const float    GY_STOP = 12.0f;

    Serial.println("# 策略[突然启动单次冲量]：逐级加大单次驱动电流，找最小越平衡冲量。");
    bool crossedOnce = false;
    for (float mA = SU_START; mA <= SU_MAX + 0.1f && !crossedOnce; mA += SU_STEP) {
        float pPre = sampleAndLog("IM_PRE");
        if (fabsf(pPre - startRest) > REST_BAND_DEG * 2) {
            uint32_t t = millis();
            while (millis() - t < 1500) { delay(TICK_MS); float p = sampleAndLog("IM_PRE"); if (poweredDangerStop(p)) return; }
            pPre = sampleAndLog("IM_PRE");
            if (fabsf(pPre - startRest) > REST_BAND_DEG * 2) continue;
        }
        Serial.printf("# 单次冲量 %+.0fmA …\n", driveSign * mA);
        M5.dis.fillpix(CRGB(0x30, 0x00, 0x00));
        motorSetCurrentmA((float)driveSign * mA);
        bool  crossed = false;
        float closest = startRest;
        uint32_t t0 = millis();
        while (millis() - t0 < SU_DRIVE_TO) {
            delay(TICK_MS);
            float p = sampleAndLog("IM_DRIVE");
            if ((float)st * (startRest - p) > (float)st * (startRest - closest)) closest = p;
            if (poweredDangerStop(p)) return;
            if ((float)st * p < 0) { crossed = true; break; }
            if ((float)driveSign * g_lastGyRate <= 0) break;
        }
        motorStop();
        if (!crossed) {
            Serial.printf("# %+.0fmA 未越平衡(最高 %.1f)，回落侧减速后加大。\n", driveSign * mA, closest);
            motorSetCurrentmA((float)driveSign * 600.0f);
            uint32_t t2 = millis();
            while (millis() - t2 < MANAGE_TO) { delay(TICK_MS); float p = sampleAndLog("IM_CATCH"); if (poweredDangerStop(p)) return; if (fabsf(p - startRest) < REST_BAND_DEG && fabsf(g_lastGyRate) < GY_STOP) break; }
            motorStop();
            continue;
        }
        crossedOnce = true;
        Serial.printf("# ★越平衡(closest=%.1f)！立即反向急刹防翻。\n", closest);
        M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));
        motorSetCurrentmA((float)spinSign * MOTOR_MAX_MA);
        float farPeak = 0;
        uint32_t t2 = millis();
        while (millis() - t2 < MANAGE_TO) { delay(TICK_MS); float p = sampleAndLog("IM_FAR"); if ((float)st * p < (float)st * farPeak) farPeak = p; if (poweredDangerStop(p)) return; if ((float)driveSign * g_lastGyRate <= GY_STOP) break; }
        motorStop();
        Serial.printf("# 冲对面峰值 %.1f\n", farPeak);
    }
    uint32_t ts = millis(); float pe = 0;
    while (millis() - ts < 1500) { delay(TICK_MS); float p = sampleAndLog("IM_SET"); pe = p; if (poweredDangerStop(p)) return; }
    motorStop();
    Serial.printf("# 单次冲量起跳结束：%s，最终落于 %.1f。\n", crossedOnce ? "已越平衡" : "未越平衡(单次动量受限)", pe);
}

// ---- 策略3：莱洛三角震荡式蓄能（秋千）—— 当前硬件不适用，留空 ----
// 思路（仅备注，未实现）：与机体自然摆动**同相位逐周泵能**，幅度逐渐增长直到越平衡（像荡秋千）。
//   本套件**不适用**：几何强不对称（外棱距静止态仅 3-4°、平衡 30°+），振荡会先撞外棱再到平衡 → 不安全。
//   若将来换更对称/飞轮惯量更大的机构，再在此实现。
static void swingUpPump(int st) {
    (void)st;
    Serial.println("# 策略[震荡式蓄能/秋千]：当前硬件不适用（几何强不对称、外棱过近），未实现、留空。");
}

static bool recoverGuard();  // 前向声明（定义在文件后部）

// ===== 消能 / 缓落（统一封装，decision 006·C）=====
// 把机体角速度收向 0、收停到目标静止态——用对**机体角速度的反向阻尼**实现，**side-agnostic**：
//     目标飞轮转速 = 当前飞轮转速 − K_DAMP · 机体角速度(带符号)
//   它永远"刹机体的当前运动"：未越平衡的回落=同向刹、越平衡后=反向刹，自动正确（无需分侧）。
//   机体越慢 → 目标越接近当前转速(hold) → 自然软收敛。收停到位后**斜坡退飞轮转速去饱和**。
// 复用场景：探测每轮回落消能 / 正式起跳落地 / 平衡维持失败回落 / 翻倒自救收尾。
// 返回：0=已收停到目标静止态; 1=硬危险已断电(调用方中止); 2=超时未收停。
enum DampResult { DAMP_SETTLED = 0, DAMP_DANGER = 1, DAMP_TIMEOUT = 2 };
static int cancelEnergyToRest(float targetRestDeg, uint32_t timeoutMs, const char *phase) {
    const float    K_DAMP   = 4.0f;                    // rpm per (°/s) 阻尼增益，估算初值待实测
    const float    GY_STOP  = 12.0f;                   // 机体角速度认定"摆停"阈(°/s)
    const float    W_CLAMP  = SPEED_LIMIT_RPM * 0.9f;  // 飞轮目标转速限幅(<硬超速)
    const float    LAT_BAIL = 18.0f;                   // 机体出平面超此 → 立即停消能（见下）
    const uint32_t DESAT_MS = 1500;                    // 去饱和斜坡时长
    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMs) {
        delay(TICK_MS);
        float p = sampleAndLog(phase);
        if (speedDanger(p)) return DAMP_DANGER;
        // 关键(run_006c 教训)：机体一旦出平面，本阻尼器按"面内陀螺率"驱动飞轮会**越追越歪、把飞轮泵到高速**
        //   (实测泵到 ~868rpm)，反而加剧震荡。出平面飞轮无 authority → 立即停飞轮、断电、交收尾自救。
        if (fabsf(g_lastLateral) > LAT_BAIL) {
            Serial.printf("# 消能中机体出平面(lat=%.1f>%.0f) → 停飞轮、断电交收尾(防越追越歪)。\n", g_lastLateral, LAT_BAIL);
            motorSetSpeedRPM(0); motorPowerOff(); g_faulted = true; return DAMP_DANGER;
        }
        float target = g_lastSpeed - K_DAMP * g_lastGyRate;   // 反向阻尼：刹机体当前运动
        if (target >  W_CLAMP) target =  W_CLAMP;
        if (target < -W_CLAMP) target = -W_CLAMP;
        motorSetSpeedRPM(target);
        if (fabsf(p - targetRestDeg) < REST_BAND_DEG && fabsf(g_lastGyRate) < GY_STOP) {
            // 收停到目标静止态 → 斜坡退飞轮去饱和（稳定势阱吸收小反力矩）
            float w0 = g_lastSpeed;
            uint32_t t1 = millis();
            while (millis() - t1 < DESAT_MS) {
                delay(TICK_MS);
                float pp = sampleAndLog(phase);
                if (speedDanger(pp)) return DAMP_DANGER;
                float frac = 1.0f - (float)(millis() - t1) / (float)DESAT_MS;
                if (frac < 0) frac = 0;
                motorSetSpeedRPM(w0 * frac);
            }
            motorSetSpeedRPM(0);
            return DAMP_SETTLED;
        }
    }
    motorSetSpeedRPM(0);
    return DAMP_TIMEOUT;
}

// 确认"已稳定回到静止态"（decision 006·B 加严）：要求 pitch 入带 且 **横向(出平面)≈0** 且 角速度小，
//   **连续保持 holdMs(≥0.5s)** 才算稳定回位——单帧入带不算，防震荡未停/未察觉的侧向失稳就开下一轮。
//   capMs 内未达成返回 false。speedDanger 危险即断电返回 false。
static bool confirmSettledAtRest(float targetRest, uint32_t holdMs, uint32_t capMs, const char *phase) {
    const float LAT_OK = 6.0f;   // 横向(出平面)需接近 0（防侧倒未察觉）
    const float GY_OK  = 8.0f;   // 角速度需小
    uint32_t okStart = 0, t0 = millis();
    while (millis() - t0 < capMs) {
        delay(TICK_MS);
        float p = sampleAndLog(phase);
        if (speedDanger(p)) return false;
        bool ok = fabsf(p - targetRest) < REST_BAND_DEG && fabsf(g_lastLateral) < LAT_OK && fabsf(g_lastGyRate) < GY_OK;
        if (ok) { if (okStart == 0) okStart = millis(); if (millis() - okStart >= holdMs) return true; }
        else okStart = 0;   // 任何一帧违例 → 稳定计时重置
    }
    return false;
}

// 收尾 + **一次性**条件自救（decision 006·D；按用户实测：导向第三/四面后若可救，磕一次即回）：
//   起跳/消能不论怎么退出(危险断电/越平衡落地/探测耗尽)都走这里。步骤：
//   1) **先等机体震荡/翻滚停下、稳定到第三/四面**（断电后飞轮滑行 + 机体落定，约 2.5s）。
//   2) 读新鲜稳定姿态判可救性：
//      · |pitch|>40°(已翻越到第三/四面) 且 |lat|<10°(已落回平面内、飞轮有 authority) → **单次击杀**磕回最近静止态。
//      · |lat|≥10°(仍横向倾倒/三维) 或 |pitch|≤40°(没翻越/已在静止态) → 不救、断电收尾。
static void finalizeOrRescueSpeed() {
    // 1) 等震荡停下、机体落定（断电下飞轮滑行；只读不驱动）
    Serial.println("# [D] 收尾：等机体震荡/翻滚停下、落定…");
    M5.dis.fillpix(CRGB(0x10, 0x10, 0x10));
    uint32_t t0 = millis();
    while (millis() - t0 < 2500) { delay(TICK_MS); sampleAndLog("FIN_SETL"); }

    float p0   = g_accelPitch;    // 用 settle 末帧的新鲜读数（勿再额外 imuReadAttitude，曾在此挂死）
    float lat0 = g_lastLateral;
    const float ROLLED_DEG = 40.0f, PERP_GIVEUP = 10.0f;
    Serial.printf("# [D] 收尾姿态: 面内pitch=%.1f° 垂直lat=%.1f°（翻越阈%.0f 可救阈%.0f）\n", p0, lat0, ROLLED_DEG, PERP_GIVEUP);

    if (fabsf(p0) <= ROLLED_DEG) { Serial.println("# 未翻越(在范围内/静止态) → 断电收尾。"); motorPowerOff(); return; }
    if (fabsf(lat0) >= PERP_GIVEUP) { Serial.printf("# 仍横向倾倒(lat=%.1f≥%.0f，三维) → 不救、断电。\n", fabsf(lat0), PERP_GIVEUP); motorPowerOff(); return; }

    // 2) 可救：翻越到第三/四面 且 已落回平面内 → **单次**击杀磕回最近静止态
    int   kickSign = (p0 > 0) ? -1 : +1;                 // 朝**减小 |pitch|**(回最近静止态)方向
    float tRest    = (p0 > 0) ? REST_B_DEG : REST_A_DEG; // 第三/四面在正侧→回B，负侧→回A
    Serial.printf("# 翻越到第三/四面(pitch=%.1f) 但已落回平面(lat=%.1f<%.0f) → **单次**击杀 %+.0frpm 磕回 %s。\n",
                  p0, fabsf(lat0), PERP_GIVEUP, kickSign * 2600.0f, (p0 > 0) ? "B" : "A");
    motorReenable();   // 自救前重新使能输出（**不重 begin**，避免双 I²C 挂死）
    // 单次但**蓄能式**击杀(实测单一速度冲量太弱、磕不动第三面)：先反向把飞轮转起来(机体顶在第三面不动)，
    //   再**急反转**给大冲量——这才是之前验证有效(84°→B)的力度。
    const float    SPIN_RPM = 1600.0f;   // 反向蓄能目标转速
    const float    KICK_RPM = 2800.0f;   // 急反转大冲量
    const uint32_t SPIN_TO  = 1500, KICK_TO = 800;
    M5.dis.fillpix(CRGB(0x20, 0x10, 0x00));  // 橙：蓄能
    motorSetSpeedRPM((float)(-kickSign) * SPIN_RPM);   // 反向蓄能(机体顶第三面不动)
    uint32_t ts = millis();
    while (millis() - ts < SPIN_TO) { delay(TICK_MS); sampleAndLog("FIN_SPIN"); if (recoverGuard()) return; if (fabsf(g_lastSpeed) >= SPIN_RPM) break; }
    M5.dis.fillpix(CRGB(0x30, 0x00, 0x00));  // 红：急反转起跳
    motorSetSpeedRPM((float)kickSign * KICK_RPM);
    uint32_t tk = millis();
    while (millis() - tk < KICK_TO) { delay(TICK_MS); sampleAndLog("FIN_KICK"); if (recoverGuard()) return; }
    // 击回后用统一消能收停到最近静止态（出平面会自动 bail）
    if (cancelEnergyToRest(tRest, 2500, "FIN_DAMP") == DAMP_DANGER) { Serial.println("# 自救中出平面/危险 → 断电。"); return; }
    motorStop(); motorPowerOff();
    float pe = g_accelPitch;
    if (fabsf(pe) < 34.0f && fabsf(g_lastLateral) < 15.0f) { Serial.printf("# ★单次击杀自救成功 pitch=%.1f\n", pe); M5.dis.fillpix(CRGB(0x00, 0x28, 0x00)); }
    else { Serial.printf("# 单次击杀后 pitch=%.1f（未完全归位），停、不再追加。\n", pe); M5.dis.fillpix(CRGB(0x28, 0x18, 0x00)); }
}

// ---- 策略4：全速度模式 起跳（离散分轮探测 + 一次性起跳 + 统一消能落地 + 兜底）----
// 锁定流程见 decisions/006。四阶段：
//   [0] 速度驱动/供电自检（命令小目标转速看飞轮响应）。
//   [探测·分轮] 多轮，每轮**一次性**施加**一个固定**目标转速、轮间**逐步增大**（**非轮内加速**）；
//       每轮前确认已停稳回到起始静止态（防同相位泵能/震荡累积）；
//       未越平衡 → `cancelEnergyToRest` 回落消能（同向刹，统一函数自动判向）、收停后再加档。
//   [起跳落地] 某轮一次性档**越平衡** → `cancelEnergyToRest` 反向消能/动量接住，收停到对面静止态
//       （该档即"起跳所需最小控制量"，记录可复用为正式一次性起跳参数）。
//   [D] 终点兜底：翻越→面内击杀自救 / 横向→断电；否则正常断电。
// 方向：driveSign=-st（约定：负转速=朝平衡，B 侧实测、A 侧对称）。因硬危险只判横向(006·A)、外棱仅 3-4°，
//   **不做"朝外主动方向探测"**（风险高），靠约定 + 统一消能 + [D] 兜底护住。参数保守，待实测逐 run 调。
static void swingUpSpeed(int st) {
    const float startRest = (st == +1) ? REST_B_DEG : REST_A_DEG;
    const float oppRest   = (st == +1) ? REST_A_DEG : REST_B_DEG;
    const int   driveSign = -st;   // 朝平衡(|pitch|减小)的飞轮转向（负转速=朝平衡）

    const float    ROUND_START = 200.0f;   // 首轮固定目标转速(rpm)，小
    const float    ROUND_MAX   = 2000.0f;  // 目标转速硬天花板（安全上限）
    const float    ADV_INC_CAP = 8.0f;     // 每轮最多再追加的"朝平衡前进量"(°)——防贪心
    const float    ADV_FRAC    = 0.45f;    // 想吃掉剩余 gap 的比例（稳步逼近）
    const float    DT_MIN = 40.0f, DT_MAX = 200.0f;  // 轮间 ΔT 限幅(rpm)
    const uint32_t ROUND_TO    = 1500;     // 单轮观察时长（一次性冲量后看越平衡/顶点回落）
    const uint32_t DAMP_TO     = 3000;     // 消能收停超时
    const uint32_t SETTLE_MS   = 1500;
    const int      CROSS_NEED  = 3;        // 越平衡去抖帧数（防噪声假越界）
    const float    barrier     = fabsf(startRest);  // 静止态→平衡 的角距(越过即起跳)

    // ---- [0] 速度驱动/供电自检 ----
    Serial.println("# [0] 速度模式驱动自检：命令 +150rpm 看飞轮响应…");
    motorSetSpeedRPM(150); delay(300);
    float wTest = motorSpeedRPM(); motorSetSpeedRPM(0);
    Serial.printf("# 飞轮响应 %.0frpm\n", wTest);
    if (fabsf(wTest) < 30.0f) { Serial.println("# 飞轮无响应(供电/驱动异常) → 断电"); motorPowerOff(); return; }
    if (!settleSpeedSafe(SETTLE_MS, "SP0_SET")) return;

    // ---- [探测·分轮·模型自适应] 一次性冲量 → 读峰值前进 → adv-vs-T 局部拟合稳步加档 ----
    // 简易模型：一次性目标转速 T 给机体一记冲量，机体朝平衡峰值前进 adv(°)。多轮拟合 adv-vs-T 斜率，
    //   据此把"想再前进 X°"换算成"加多少 ΔT"；每轮**只追加有限前进量**(≤ADV_INC_CAP)，**不贪心一次到位**，
    //   稳步逼近越平衡——避免某轮 T 过大、动量过猛冲过外棱翻倒不可复。(torque 在速度模式不可直接算，
    //   故用 输入T→输出adv 的经验拟合隐式代表 torque 效果。)
    Serial.printf("# [探测] 离散分轮(模型自适应)：首档%.0frpm，每轮一次性冲量、读峰值前进、按拟合稳步加档(每轮≤%.0f°)。\n",
                  ROUND_START, ADV_INC_CAP);
    bool  crossed  = false;
    float crossTgt = 0;
    float T = ROUND_START, prevT = 0, prevAdv = 0;
    bool  haveSlope = false;
    int   roundIdx = 0;
    while (T <= ROUND_MAX && !crossed) {
        roundIdx++;
        // 轮前：必须"已稳定回位 ≥0.5s"(pitch 入带 且 横向≈0 且 角速度小)才开下一轮（防震荡累积/未察觉侧倒）
        if (!confirmSettledAtRest(startRest, 500, 4000, "SP_SETL")) {
            if (cancelEnergyToRest(startRest, DAMP_TO, "SP_PRESET") == DAMP_DANGER) return;
            if (!confirmSettledAtRest(startRest, 500, 4000, "SP_SETL2")) {
                Serial.println("# 未能在限时内稳定回位(疑横向失稳) → 终止探测、转兜底。");
                break;
            }
        }

        // 一次性施加固定目标转速（不在轮内加速）
        Serial.printf("# [轮%d 档%.0frpm] 一次性冲量 → %+.0frpm…\n", roundIdx, T, driveSign * T);
        M5.dis.fillpix(CRGB(0x30, 0x00, 0x00));  // 红
        motorSetSpeedRPM((float)driveSign * T);
        int   crossStreak = 0;
        float closest = startRest;
        bool  peaked  = false;
        uint32_t t0 = millis();
        while (millis() - t0 < ROUND_TO) {
            delay(TICK_MS);
            float p = sampleAndLog("SP_RND");
            if (speedDanger(p)) return;
            if ((float)st * (startRest - p) > (float)st * (startRest - closest)) closest = p;  // 记最朝平衡
            if ((float)st * p < 0) { if (++crossStreak >= CROSS_NEED) { crossed = true; crossTgt = T; break; } }
            else crossStreak = 0;
            if ((float)driveSign * g_lastGyRate <= 0 && fabsf(p - startRest) > 3.0f) { peaked = true; break; }  // 顶点未越
        }
        float peakAdv = (float)st * (startRest - closest);   // 本轮朝平衡峰值前进(°)

        if (crossed) {
            Serial.printf("# ★[轮%d 档%.0frpm] 越平衡(去抖%d帧)！= 最小起跳量。→ 反向消能/动量接住落地。\n", roundIdx, T, CROSS_NEED);
            break;
        }
        // 异常：本轮朝外棱(远离平衡)动 → 疑方向反/失稳，停止贪进
        if (peakAdv < -2.0f) {
            Serial.printf("# ⚠[轮%d 档%.0frpm] 机体朝外棱动(前进%.1f°<0) → 疑方向反/失稳，终止探测转兜底。\n", roundIdx, T, peakAdv);
            cancelEnergyToRest(startRest, DAMP_TO, "SP_ABORT");
            break;
        }
        // 未越 → 同向回落消能
        Serial.printf("# [轮%d 档%.0frpm] 未越(峰值前进%.1f°/%.0f°, %s)。回落消能。\n",
                      roundIdx, T, peakAdv, barrier, peaked ? "顶点回落" : "观察超时");
        M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝
        if (cancelEnergyToRest(startRest, DAMP_TO, "SP_DAMP") == DAMP_DANGER) return;

        // —— 模型自适应：据 adv-vs-T 斜率，换算"再前进 desiredInc°"所需 ΔT，稳步加档 ——
        float remaining  = barrier - peakAdv;
        float desiredInc = remaining * ADV_FRAC;
        if (desiredInc > ADV_INC_CAP) desiredInc = ADV_INC_CAP;       // 不贪心：每轮最多追加 ADV_INC_CAP°
        float slope;                                                 // °前进 per rpm
        if (haveSlope && T > prevT + 1.0f) slope = (peakAdv - prevAdv) / (T - prevT);
        else                               slope = peakAdv / T;       // 初轮：从原点(T=0,adv=0)割线估
        if (slope < 0.002f) slope = 0.002f;                          // 防 0/负(未脱阱)→给较大 ΔT(后续被 DT_MAX 钳)
        float dT = desiredInc / slope;
        if (dT < DT_MIN) dT = DT_MIN;
        if (dT > DT_MAX) dT = DT_MAX;
        Serial.printf("# 模型: 前进%.1f° 剩余%.1f° 斜率%.4f°/rpm → 目标再进%.1f° → ΔT=%.0frpm（下档%.0f）\n",
                      peakAdv, remaining, slope, desiredInc, dT, T + dT);
        prevT = T; prevAdv = peakAdv; haveSlope = true;
        T += dT;
    }

    // ---- [起跳落地] 越平衡 → 反向消能/动量接住 收停到对面静止态 ----
    if (crossed) {
        M5.dis.fillpix(CRGB(0x00, 0x20, 0x10));  // 青
        int dr = cancelEnergyToRest(oppRest, DAMP_TO, "SP_LAND");
        if (dr == DAMP_DANGER) return;
        if (dr == DAMP_SETTLED) Serial.printf("# ★落地成功：收停到对面静止态(目标%.0f°)。最小起跳量≈%.0frpm。\n", oppRest, crossTgt);
        else                    Serial.println("# 落地消能超时 → 转兜底检查。");
    } else {
        Serial.printf("# 升至顶档 %.0frpm 仍未越平衡。安全收尾（下次上调 ROUND_MAX）。\n", ROUND_MAX);
    }

    // [D] 收尾/自救由分派器在 swingUpSpeed 返回后统一调用 finalizeOrRescueSpeed()
    //   （这样**所有退出路径**——危险断电/越平衡落地/探测耗尽——都会走收尾自救）。
}

// ===== 起跳分派器：识别 A/B + Vin 自检 → 按 g_swingUpStrategy 调对应策略 =====
void swingUpTest() {
    // 策略决定电机模式（运行中不可切，decision 002）：SPEED 策略整段跑速度模式，其余跑电流模式。
    //   prepAtRest 仅读 IMU 不驱动电机，模式可在其后再确认/切换。
    bool wantSpeed = (g_swingUpStrategy == SWINGUP_SPEED);
    if (wantSpeed && motorMode() != MOTOR_MODE_SPD) {
        Serial.println("# 速度模式策略 → 重新初始化电机为速度模式。");
        if (!motorInitSpeed(MOTOR_MAX_MA)) { Serial.println("# 速度模式初始化失败 → 断电"); motorPowerOff(); return; }
    } else if (!wantSpeed && motorMode() != MOTOR_MODE_CUR) {
        Serial.println("# 电流模式策略 → 重新初始化电机为电流模式。");
        if (!motorInit()) { Serial.println("# 电流模式初始化失败 → 断电"); motorPowerOff(); return; }
    }

    int st = prepAtRest();
    if (st == 0 && wantSpeed) {
        // 起始就不在 A/B：若是可救的第三/四面 → **单次**自救磕回，再重判（避免开机即卡在第三面）。
        Serial.println("# 起始不在 A/B：尝试单次自救（第三/四面且 lat 小才救），成功后重判 A/B…");
        finalizeOrRescueSpeed();   // 自带可救判据；结束电机断电
        motorReenable();           // 自救后重新使能（不重 begin，避免双 I²C 挂死）
        st = prepAtRest();
    }
    if (st == 0) { motorPowerOff(); return; }
    float   vin  = motorVoltage();
    uint8_t err0 = motorErrorCode();
    Serial.printf("# 起跳自检: Vin=%.2fV err=%u 起始=%s 策略=[%s] 模式=%s\n",
                  vin, err0, (st == +1) ? "B" : "A", swingUpName(g_swingUpStrategy),
                  motorMode() == MOTOR_MODE_SPD ? "速度" : "电流");
    if (err0 == 1) { Serial.println("# ABORT: 过压 → 断电"); motorPowerOff(); return; }
    if (vin < 6.0f) { Serial.println("# ABORT: Vin<6V，未接 12V，不试。"); motorPowerOff(); return; }

    if (g_swingUpStrategy == SWINGUP_SPEED) {
        swingUpSpeed(st);          // [0]自检→分轮探测→越平衡落地（各危险路径会提前返回、已断电）
        finalizeOrRescueSpeed();   // [D] 统一收尾：等震荡停 + 第三/四面可救则**单次**击杀自救
        return;
    }

    switch (g_swingUpStrategy) {
        case SWINGUP_IMPULSE: swingUpImpulse(st); break;
        case SWINGUP_PUMP:    swingUpPump(st);    break;
        case SWINGUP_CUBLI:
        default:              swingUpCubli(st);   break;
    }
    motorStop();
    motorPowerOff();
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿
}

// ===== 开机方向表征（decision 006 简化目标）：只判定+记录起跳转向，然后终止 =====
// 目标(用户)：开机后只探测"飞轮**速度该往哪个方向变化(加速)**才能把机体推向平衡"，记录该方向 → **终止**
//   (断电 + 仅持续监视)。不再做完整起跳/落地/自救。
// 物理(用户校正)：机体运动方向由**反作用力矩**决定；力矩由**飞轮角加速度 = 速度变化 dω/dt 的方向**决定，
//   **不是飞轮当前转速方向**。故控制量 = "速度变化方向"。从静止给飞轮一个目标转速 = 一记"速度变化"冲量，
//   其符号即速度变化方向(加速到目标期间产生反作用力矩；到达目标后 dω/dt→0、力矩消失)。看机体朝平衡
//   (|pitch|↓)还是朝外棱动，定出正确的速度变化方向并记录。
// 安全：全程 speedDanger（超平衡 40°/横向/超速 → 立即断电、仅监视、不自救）。需 motorInitSpeed 后调用。
void probeSwingUpDirection() {
    if (motorMode() != MOTOR_MODE_SPD && !motorInitSpeed(MOTOR_MAX_MA)) { motorPowerOff(); return; }
    int st = prepAtRest();
    if (st == 0) { Serial.println("# 不在静止态 A/B → 断电、仅监视（不驱动）。"); motorPowerOff(); return; }
    float vin = motorVoltage();
    if (motorErrorCode() == 1 || vin < 6.0f) { Serial.printf("# Vin=%.2f/过压异常 → 断电。\n", vin); motorPowerOff(); return; }

    const float    startRest = (st == +1) ? REST_B_DEG : REST_A_DEG;
    const int      guess     = -st;       // 约定速度变化方向(负=朝平衡，B侧实测、A侧对称)，先试它
    const float    PROBE_RPM = 350.0f;    // 小幅"速度变化"冲量(静止→±PROBE_RPM)，足以看出方向、不致翻
    const uint32_t PROBE_TO  = 900;

    Serial.printf("# [方向表征] 起始=%s。给一记小速度变化冲量(目标 %+.0frpm = 速度变化方向 %+d)看机体朝哪动…\n",
                  (st == +1) ? "B" : "A", guess * PROBE_RPM, guess);
    M5.dis.fillpix(CRGB(0x30, 0x00, 0x00));  // 红
    motorSetSpeedRPM((float)guess * PROBE_RPM);
    float closest = startRest, farthest = startRest;
    uint32_t t0 = millis();
    while (millis() - t0 < PROBE_TO) {
        delay(TICK_MS);
        float p = sampleAndLog("DIRPROBE");
        if (speedDanger(p)) return;   // 超平衡40°/横向/超速 → 断电+监视
        if ((float)st * (startRest - p) > (float)st * (startRest - closest))  closest  = p;  // 最朝平衡
        if ((float)st * (p - startRest) > (float)st * (farthest - startRest)) farthest = p;  // 最朝外棱
    }
    motorStop();
    float advBal  = (float)st * (startRest - closest);    // 朝平衡前进(>0 好)
    float advEdge = (float)st * (farthest - startRest);   // 朝外棱(>0 表示这个变化方向推反了)

    if      (advBal  > advEdge && advBal  > 1.5f) g_swingUpDir = guess;    // 约定方向正确
    else if (advEdge > advBal  && advEdge > 1.5f) g_swingUpDir = -guess;   // 推反了 → 取反
    else                                          g_swingUpDir = 0;        // 不明确

    settleSpeedSafe(1500, "DIR_SET");   // 小幅静置自复位（gravity 拉回；危险即断电）
    motorStop(); motorPowerOff();

    if (g_swingUpDir != 0) {
        Serial.printf("# ★方向表征完成并记录：朝平衡起跳的**飞轮速度变化方向 = %+d**（朝平衡%.1f° / 朝外%.1f°）。\n",
                      g_swingUpDir, advBal, advEdge);
        Serial.println("# 已终止：电机断电，仅持续监视读数。");
        M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));  // 绿
    } else {
        Serial.printf("# 方向不明确(朝平衡%.1f° 朝外%.1f°)，冲量偏小，需加大重试。终止、仅监视。\n", advBal, advEdge);
        M5.dis.fillpix(CRGB(0x28, 0x18, 0x00));  // 橙
    }
}

// ===== 启动自动表征序列：I²C自检 → 供电探测 → 识别A/B → 当前侧危险边界 =====
// [1] 两个 I²C 总线自检：IMU(MPU6886 在 Wire1 0x68)、电机(RollerCAN 在 Wire 0x64)。
static bool i2cSelfCheck() {
    Wire1.beginTransmission(0x68);              int imu = Wire1.endTransmission();
    Wire.beginTransmission(MOTOR_I2C_ADDR);     int mot = Wire.endTransmission();
    bool ok = (imu == 0) && (mot == 0);
    Serial.printf("# [1] I²C 自检: IMU(Wire1,0x68)=%s  电机(Wire,0x%02X)=%s → %s\n",
                  imu == 0 ? "OK" : "NG", MOTOR_I2C_ADDR, mot == 0 ? "OK" : "NG", ok ? "通过" : "失败");
    return ok;
}

// [4] 危险边界渐进探测：当前静止态 st，**朝已知外棱方向(电流符号=+st)**逐级加流，
//   渐进逼近"机体被推向外棱(不可恢复侧)"的临界。**速度钳 + 小偏移即停**(外棱仅 3-4°，绝不真翻)。
//   返回逼近到的边界电流(mA，带符号)；0=升到上限仍很稳。
float dangerBoundaryProbe(int st) {
    const float restAngle    = (st == +1) ? REST_B_DEG : REST_A_DEG;
    const float outerSign    = (float)st;     // +st = 朝外棱
    const float V_SAFE       = 60.0f;         // 朝外角速度钳(°/s)：超此=将失控→停
    const float SAFE_DEFLECT = 2.5f;          // 朝外偏移超此(°)=已逼近外棱→停
    const float    PB_START = 40.0f, PB_STEP = 25.0f;
    const uint32_t PB_PULSE = 120, PB_SETTLE = 1200;

    Serial.printf("# [4] %s侧危险边界探测：朝外棱(电流符号%+d)渐进加流，逼近即停。\n", (st == +1) ? "B" : "A", st);
    float boundary = 0;
    for (float mA = PB_START; mA <= MOTOR_MAX_MA + 0.1f; mA += PB_STEP) {
        float pPre = sampleAndLog("DB_PRE");
        if (fabsf(pPre - restAngle) > REST_BAND_DEG * 2) {  // 自复位
            uint32_t t = millis();
            while (millis() - t < PB_SETTLE) { delay(TICK_MS); float p = sampleAndLog("DB_PRE"); if (poweredDangerStop(p)) return boundary; }
            pPre = sampleAndLog("DB_PRE");
        }
        float maxOut = 0, maxVout = 0;
        bool  hit = false;
        motorSetCurrentmA(outerSign * mA);
        uint32_t t0 = millis();
        while (millis() - t0 < PB_PULSE) {
            delay(TICK_MS);
            float p    = sampleAndLog("DB");
            float outEx = outerSign * (p - restAngle);   // 朝外偏移(>0)
            float vout  = (float)st * g_lastGyRate;      // 朝外角速度
            if (outEx > maxOut)  maxOut = outEx;
            if (vout  > maxVout) maxVout = vout;
            if (poweredDangerStop(p)) return boundary;   // 硬保底(真到±34)
            if (vout > V_SAFE || outEx > SAFE_DEFLECT) { hit = true; break; }  // 逼近→停
        }
        motorStop();
        Serial.printf("# 朝外 %+.0fmA: 偏移峰值=%.2f° 朝外角速度峰值=%.0f°/s%s\n",
                      outerSign * mA, maxOut, maxVout, hit ? " ←逼近外棱" : "");
        uint32_t t1 = millis();
        while (millis() - t1 < PB_SETTLE) { delay(TICK_MS); float p = sampleAndLog("DB_SET"); if (poweredDangerStop(p)) return boundary; }
        if (hit) { boundary = outerSign * mA;
                   Serial.printf("# => %s侧危险边界 ≈ %+.0fmA（朝外棱，超此将失控翻倒）\n", (st == +1) ? "B" : "A", boundary);
                   break; }
    }
    if (boundary == 0) Serial.printf("# 升到 %.0fmA 仍未逼近外棱（边界更高/机体很稳）。\n", MOTOR_MAX_MA);
    return boundary;
}

// [4] 击杀式方向探测：用一个**小冲量**安全试出"电机往哪转才朝平衡"。
//   在预期朝平衡方向(-st)给小冲量；若机体反而朝**外棱(危险)**运动 → 转向相反(返 +st)。
//   小冲量(实测 ~250mA 朝外棱仅偏 ~0.6°，安全)，速度钳+逼近外棱即停。返回朝平衡电流符号(±1)，0=不明确/异常。
static int directionProbeKick(int st) {
    const float restAngle = (st == +1) ? REST_B_DEG : REST_A_DEG;
    const float V_SAFE  = 60.0f;     // 朝外角速度钳
    const float KICK_MA = 250.0f;    // 小冲量(朝外棱仅偏~0.6°，安全)
    const float DETECT  = 0.8f;      // 可辨偏移
    const uint32_t KICK_MS = 160, SETTLE = 1200;

    Serial.printf("# [4] 击杀式方向探测：先试 %+.0fmA(预期朝平衡)，看机体朝平衡还是朝外棱…\n", (float)(-st) * KICK_MA);
    float innerMax = 0, outerMax = 0;
    motorSetCurrentmA((float)(-st) * KICK_MA);
    uint32_t t0 = millis();
    while (millis() - t0 < KICK_MS) {
        delay(TICK_MS);
        float p = sampleAndLog("DIR");
        float inner = (float)st * (restAngle - p);   // 朝平衡(>0)
        float outer = (float)st * (p - restAngle);   // 朝外棱(>0)
        if (inner > innerMax) innerMax = inner;
        if (outer > outerMax) outerMax = outer;
        if (poweredDangerStop(p)) return 0;
        if (millis() - t0 > 40 && (float)st * g_lastGyRate > V_SAFE) {  // 跳过起振瞬态
            Serial.println("# 朝外棱速度超钳 → 安全停（最终判向以净偏移为准）。"); break;
        }
    }
    motorStop();
    uint32_t ts = millis();
    while (millis() - ts < SETTLE) { delay(TICK_MS); float p = sampleAndLog("DIR_SET"); if (poweredDangerStop(p)) return 0; }

    int tb;
    if (innerMax > outerMax + DETECT) { tb = -st; Serial.printf("# → 机体朝平衡(峰值%.1f°)，朝平衡电流符号=%+d ✓\n", innerMax, tb); }
    else if (outerMax > innerMax + DETECT) { tb = +st; Serial.printf("# → 机体朝外棱(峰值%.1f°)，转向相反！朝平衡电流符号=%+d\n", outerMax, tb); }
    else { tb = 0; Serial.printf("# → 不明确(inner=%.1f° outer=%.1f°)。\n", innerMax, outerMax); }
    return tb;
}

// 固定启动表征序列。需在 M5.begin/imuInit/motorInit 之后调用。
void startupSequence() {
    Serial.println("# ===== 启动自动表征序列 =====");
    if (!i2cSelfCheck()) { Serial.println("# [1] I²C 自检失败 → 停止。"); motorPowerOff(); return; }

    Serial.println("# [2] 供电探测（加大转速测顶速判 12V）…");
    if (!powerDetectTest()) { Serial.println("# 供电不足（仅5V）→ 不起跳/不探边界，停止。"); return; }

    Serial.println("# [3] 识别当前静止态 A/B …");
    int st = prepAtRest();
    if (st == 0) { Serial.println("# 不在 A/B（或翻倒）→ 需人工复位，停止。"); motorPowerOff(); return; }

    if (!motorInit()) { Serial.println("# 电机重新使能失败 → 停。"); motorPowerOff(); return; }  // 供电探测后重新使能输出

    int tb = directionProbeKick(st);                         // [4] 击杀式安全方向探测
    if (tb != -st) {
        Serial.printf("# 方向探测结果(%+d)与预期(%+d)不符或不明确 → 安全起见停止（勿在方向未确认下探边界/起跳）。\n", tb, -st);
        motorStop(); motorPowerOff(); return;
    }

    dangerBoundaryProbe(st);                                 // [5] 当前侧危险边界（朝已确认的外棱方向）

    Serial.println("# [6] 下一步（待『安全越平衡落地』ready 后启用）：起跳到对面静止态 → 落稳 → 探对面危险边界。");
    Serial.println("# 本轮先到此（前半段安全可测；起跳越平衡仍在解决受控落地，未自动执行以免翻倒）。");
    motorStop(); motorPowerOff();
    M5.dis.fillpix(CRGB(0x00, 0x28, 0x00));
}

// ===== 翻倒兜底恢复（条件化）=====
// 轴：面内角 pitch(可控轴，会滚过外棱翻越正常范围)；垂直角 lat(出平面，电机无 authority)。
// 触发：机体**面内角 |pitch| > 40°** = 已滚过外棱、翻越正常范围。读**垂直角 lat**判可救性：
//   · |lat| < 10°  → 机体仍在运动平面内、飞轮还能使劲 → 大动量击杀两方向各搏一次，尝试磕回 A/B。
//   · |lat| ≥ 10°  → 已三维翻滚、飞轮无 authority → 断电、不再补救。
//   |pitch| ≤ 40°(未翻越) → 本兜底不适用。放宽保底(只防过压/超速)。
static bool recoverGuard() {  // 自救用宽松保底
    if (g_lastErr == 1) { Serial.println("# 过压(E:1) → 停。"); motorStop(); motorPowerOff(); return true; }
    if (fabsf(g_lastSpeed) > SPEED_LIMIT_RPM) { Serial.println("# 飞轮超速 → 停。"); motorStop(); motorPowerOff(); return true; }
    return false;
}

void recoverFlipTest() {
    Serial.println("# t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err,vin");
    Serial.println("# 翻倒兜底恢复：面内角(pitch)>40°(翻越)且垂直角(lat)<10°(仍在平面)→击杀试救；垂直角≥10°→断电不救。");
    M5.dis.fillpix(CRGB(0x30, 0x18, 0x00));
    if (!imuCalibrateGyro(200)) { Serial.println("# 标定失败"); motorPowerOff(); return; }
    float lat0 = 0;
    {
        float bx = 0, by = 0, bz = 0; imuGyroBias(&bx, &by, &bz); g_gyBias = by; g_gxBias = bx;
        float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0; imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
        double p = 0, r = 0; imuComputeAttitude(ax, ay, az, &p, &r);
        lat0 = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.29578f;
        imuFusionInit(p, FUSION_ALPHA); imuFusionInitLateral(lat0, FUSION_ALPHA);
        g_fusedPitch = (float)p; g_lastLateral = lat0;
    }
    double p0 = 0, r0 = 0; imuReadAttitude(&p0, &r0);

    // 轴对应：p0=面内角(pitch，可控轴，会滚过外棱翻越正常范围)；lat0=垂直角(出平面，电机无 authority)。
    const float ROLLED_DEG  = 40.0f;  // |面内角|>此 = 已翻越正常范围(滚过外棱)
    const float PERP_GIVEUP = 10.0f;  // |垂直角|≥此 = 三维翻滚→放弃断电；<此 = 仍在平面内→可击杀试救
    Serial.printf("# 兜底检查: 面内角pitch=%.1f° 垂直角lat=%.1f°（翻越阈%.0f° 垂直救助阈%.0f°）\n",
                  p0, lat0, ROLLED_DEG, PERP_GIVEUP);

    if (fabsf(p0) <= ROLLED_DEG) {
        Serial.printf("# 面内角 %.0f°≤%.0f° → 未翻越正常范围，本兜底不适用。\n", fabsf(p0), ROLLED_DEG);
        motorPowerOff(); return;
    }
    if (fabsf(lat0) >= PERP_GIVEUP) {
        Serial.printf("# 已翻越 且 垂直角%.0f°≥%.0f°(三维翻滚) → 断电，不再补救。\n", fabsf(lat0), PERP_GIVEUP);
        motorPowerOff(); return;
    }
    Serial.printf("# 已翻越(面内%.0f°) 但 垂直角%.0f°<%.0f°(仍在平面内) → 击杀尝试磕回 A/B。\n", fabsf(p0), fabsf(lat0), PERP_GIVEUP);

    const float    SPIN_MA  = 140.0f;
    const float    KICK_MA  = MOTOR_MAX_MA;
    const uint32_t SPIN_TO  = 2500;
    const uint32_t KICK_TO  = 1500;
    const uint32_t SETTLE   = 1800;
    const int      dirs[2]  = {-1, +1};  // 先反向(-)再正向(+)

    bool recovered = false;
    for (int d = 0; d < 2 && !recovered; d++) {
        int kickSign = dirs[d];
        int spinSign = -kickSign;
        Serial.printf("# 自救尝试%d：蓄能 %+.0fmA → 全力起跳 %+.0fmA\n", d + 1, spinSign * SPIN_MA, kickSign * KICK_MA);

        M5.dis.fillpix(CRGB(0x20, 0x10, 0x00));  // 蓄能
        motorSetCurrentmA((float)spinSign * SPIN_MA);
        uint32_t t0 = millis();
        while (millis() - t0 < SPIN_TO) { delay(TICK_MS); sampleAndLog("RC_SPIN"); if (recoverGuard()) return; if (fabsf(g_lastSpeed) >= 1400) break; }

        M5.dis.fillpix(CRGB(0x30, 0x00, 0x00));  // 起跳
        motorSetCurrentmA((float)kickSign * KICK_MA);
        uint32_t t1 = millis();
        while (millis() - t1 < KICK_TO) { delay(TICK_MS); sampleAndLog("RC_KICK"); if (recoverGuard()) return; }
        motorStop();

        uint32_t ts = millis();
        while (millis() - ts < SETTLE) { delay(TICK_MS); sampleAndLog("RC_SET"); if (recoverGuard()) return; }

        double pe = 0, re = 0; imuReadAttitude(&pe, &re);   // 静态原始 pitch 判落点
        Serial.printf("# 尝试%d 落点: pitch=%.1f lat=%.1f\n", d + 1, pe, g_lastLateral);
        if (fabsf(pe) < 34.0f && fabsf(g_lastLateral) < 15.0f) recovered = true;  // 回到 A/B = 面内角进范围且垂直角小
    }

    motorStop();
    motorPowerOff();
    double pf = 0, rf = 0; imuReadAttitude(&pf, &rf);
    if (recovered) { Serial.printf("# ★翻倒自救成功！机体回到范围内 pitch=%.1f\n", pf); M5.dis.fillpix(CRGB(0x00, 0x28, 0x00)); }
    else           { Serial.printf("# 自救未成功，仍在范围外 pitch=%.1f，需人工复位。\n", pf); M5.dis.fillpix(CRGB(0x28, 0x00, 0x00)); }
}

// ===== 供电充足性探测（缓升小电流测飞轮顶速，区分 5V/12V，机体姿态无关）=====
bool powerDetectTest() {
    Serial.println("# t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err,vin");
    Serial.println("# 供电探测：缓升电流(<挣脱150mA)平稳转飞轮测顶速。5V≈600rpm vs 12V>1000rpm。");
    M5.dis.fillpix(CRGB(0x30, 0x18, 0x00));  // 橙：标定
    if (!imuCalibrateGyro(200)) {
        M5.dis.fillpix(CRGB(0x40, 0x00, 0x00));
        Serial.println("# ABORT：陀螺标定检测到运动 → 断电。");
        motorPowerOff();
        return false;
    }
    {  // 播种融合（任意姿态，仅用于机体异动监测）
        float bx = 0, by = 0, bz = 0;
        imuGyroBias(&bx, &by, &bz); g_gyBias = by; g_gxBias = bx;
        float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
        imuReadRaw(&ax, &ay, &az, &gx, &gy, &gz);
        double p = 0, r = 0; imuComputeAttitude(ax, ay, az, &p, &r);
        float lat0 = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.29578f;
        imuFusionInit(p, FUSION_ALPHA); imuFusionInitLateral(lat0, FUSION_ALPHA);
        g_fusedPitch = (float)p; g_lastLateral = lat0;
    }
    M5.dis.fillpix(CRGB(0x00, 0x10, 0x30));  // 蓝：探测中
    float startP = sampleAndLog("PWR_PRE");

    const float    RAMP_MAX = 150.0f;   // 缓升上限（<12V 挣脱 200mA，机体不动）
    const uint32_t RAMP_MS  = 2000;     // 2s 缓升
    const uint32_t HOLD_MS  = 4000;     // 4s 持稳让飞轮到顶速
    const float    MOVE_TOL = 6.0f;     // 机体偏离起始>此=异常(挣脱)，停
    const float    REF_5V   = 600.0f;   // 5V 实测顶速基准
    const float    THRESH   = 800.0f;   // 判据阈值（5V~600 与 12V>1000 之间）

    float    maxSpd = 0;
    bool     moved  = false;
    uint32_t t0 = millis();
    while (millis() - t0 < RAMP_MS + HOLD_MS) {
        uint32_t el = millis() - t0;
        float mA = (el < RAMP_MS) ? RAMP_MAX * (float)el / (float)RAMP_MS : RAMP_MAX;  // 缓升后保持
        motorSetCurrentmA(mA);
        delay(TICK_MS);
        float p = sampleAndLog("PWR");
        if (fabsf(g_lastSpeed) > maxSpd) maxSpd = fabsf(g_lastSpeed);
        if (g_lastErr == 1) { Serial.println("# 过压(E:1) → 停。"); break; }
        if (fabsf(p - startP) > MOVE_TOL) { moved = true; Serial.printf("# 机体异动(%.1f→%.1f) → 停。\n", startP, p); break; }
    }
    motorStop();
    motorPowerOff();

    float vin = motorVoltage();
    bool sufficient = (maxSpd > THRESH);
    Serial.printf("# === 供电探测结果: 飞轮顶速 %.0frpm, Vin=%.2fV（5V基准≈%.0f, 阈值%.0f）===\n",
                  maxSpd, vin, REF_5V, THRESH);
    if (sufficient) Serial.println("# => ✓ 供电充足：外部 12V 已生效（顶速远超 5V 水平）。");
    else            Serial.println("# => ✗ 供电不足：仅 5V 水平（外部 12V 未接/未生效）。");
    if (moved) Serial.println("# 注：机体中途移动，顶速可能偏低，结果仅供参考。");
    M5.dis.fillpix(sufficient ? CRGB(0x00, 0x28, 0x00) : CRGB(0x28, 0x10, 0x00));
    return sufficient;
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

// ===== 姿态状态监视（流程终点态：电机断电、持续打印全部传感器 + 窗口稳定判定）=====
// 设计（按用户澄清）：
//   · **逐帧原始打印保持全速率**（每 TICK_MS 一行，走 sampleAndLog 的**统一 CSV schema**），
//     贴近固件内部采样率、信息无损 → 日志可直接喂给工程内的离线计算。
//   · **窗口仅用于"状态判定"**：聚合 N 帧的**加速度计原始姿态**（无积分漂移），用均值+抖动定状态，
//     避免单帧噪声造成整体姿态误判。窗口不降低打印频率。
//   · 电机只读不驱动。判定基准：REST_B≈+30° / REST_A≈-31° / 平衡≈0°（不稳定，若稳定停此=第三面翻倒/标定异常）。
void attitudeReportTick() {
    static bool headerDone = false;
    if (!headerDone) {  // CSV 表头只打一次（与 sampleAndLog 同 schema，便于离线解析）
        Serial.println("# t_ms,phase,ax,ay,az,gx,gy,gz,pitch,pf,roll,lat,cmd_mA,cur_mA,spd_rpm,err,vin");
        headerDone = true;
    }
    const int N = 1000 / (int)TICK_MS;        // 1s 窗样本数（仅用于判定聚合）
    float pSum = 0, lSum = 0, pMin = 1e9f, pMax = -1e9f, lMin = 1e9f, lMax = -1e9f;
    for (int i = 0; i < N; i++) {
        delay(TICK_MS);
        sampleAndLog("MON");                  // 全速率逐帧打印统一 CSV（含原始6轴+加速度pitch+融合pf+lat+电机态）
        float p = g_accelPitch, l = g_accelLat;  // 判定用加速度原始姿态（无漂移）
        pSum += p; lSum += l;
        if (p < pMin) pMin = p; if (p > pMax) pMax = p;
        if (l < lMin) lMin = l; if (l > lMax) lMax = l;
    }
    float pAvg = pSum / N, lAvg = lSum / N, pJit = pMax - pMin, lJit = lMax - lMin;

    const char *state;
    if      (fabsf(pAvg - REST_B_DEG) < 6.0f) state = "静止态 B(+30°靠面)";
    else if (fabsf(pAvg - REST_A_DEG) < 6.0f) state = "静止态 A(-31°靠面)";
    else if (fabsf(pAvg) < 8.0f)              state = "平衡点附近(不稳定;若稳定停此=疑似第三面翻倒/标定异常)";
    else                                      state = "未知/翻倒(见数值)";
    bool steady = (pJit < 2.0f && lJit < 2.0f);
    Serial.printf("# === 1s窗判定(聚合%d帧, 加速度原始姿态): pitch=%.2f°(抖%.2f) lat=%.2f°(抖%.2f) → %s | %s ===\n",
                  N, pAvg, pJit, lAvg, lJit, state, steady ? "稳定" : "晃动中");
}
