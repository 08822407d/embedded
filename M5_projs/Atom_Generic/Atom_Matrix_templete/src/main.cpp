/*
 * 屏动平衡系统 —— 主程序（命令常驻架构）
 * 硬件: M5Stack ATOM Matrix (MPU6886 IMU + 5x5 点阵) + RollerCAN 反作用轮电机（电流/力矩模式）
 *
 * 架构（2026-05-31）：**固件常驻、串口命令分发**——烧一次，之后发字符命令触发流程，不必重编烧。
 *   loop 持续监视(83Hz 紧凑日志) + 收串口命令；命令跑完回监视。电机待命时输出断开。
 *   发命令：用 tools/serial_bridge.py（单进程独占串口、并发收发）→ `echo "r" > tools/serial.cmd`。
 *
 * 命令：r=急救(大力矩把卡第三面的机体磕回A/B)  i=系统辨识  b=探扭矩  c=起跳+缓冲  g=起跳→平衡  s=姿态  h=菜单
 */
#include <M5Atom.h>

#include "imu.h"
#include "motor.h"
#include "dir_test.h"

static void printMenu() {
    Serial.println("# === 命令待命(发 字符+换行) ===");
    Serial.println("#   r=急救(大力矩磕回A/B)  i=系统辨识  b=探扭矩  c=起跳+缓冲  s=姿态  h=菜单");
    Serial.println("#   ★g=起跳→平衡(蓄能急停荡向平衡+PID接管;从低蓄能起)  ]/[=起跳蓄能转速±40rpm");
    Serial.println("#   ★p=纯平衡保持(手扶到直立松手,验证PID;不起跳)");
    Serial.println("#   d=突然加速扫描(电流逐档↑)  k=蓄能急停扫描(蓄能转速逐档↑)  ← 采起跳扭矩特征曲线");
    Serial.println("#   ★m=方向确认(起跳/急救前必做：判定急刹使机体朝平衡还是朝外跳)  v=差分方向验证(两向对比抵消污染)");
    Serial.println("#   独立动作: 1=突然加速·起跳 2=突然加速·急救 3=蓄能急停·起跳 4=蓄能急停·急救 e=导向第三面");
    Serial.println("#   x=②急救参数探测(检测姿态→若不在第三面先导向→逐档急停急救磕回A/B)");
    Serial.println("#   loop 持续监视(紧凑日志)；命令跑完回监视。");
}

static void dispatchCommand(const String &cmd) {
    char c = cmd.length() ? cmd[0] : 'h';
    Serial.printf("# >>> CMD '%c'\n", c);
    // 驱动类命令：先重新使能电流输出（上个流程/待命时输出是断的；不重 begin，避免双 I²C 挂死）
    if (c == 'r' || c == 'i' || c == 'b' || c == 'c' || c == 'g' || c == 'd' || c == 'k' ||
        c == '1' || c == '2' || c == '3' || c == '4' || c == 'e' || c == 'x' || c == 'm' || c == 'v' || c == 'p' || c == 'n') motorReenable();
    switch (c) {
        case 'r': recoverFlipTest();       break;   // 急救：面内翻越(第三面)且横向小 → 蓄能+全力急刹磕回 A/B
        case 'i': sysIdExperiment();       break;   // 系统辨识激励
        case 'b': probeBreakawayTorque();  break;   // 探能起跳的扭矩 τ_break
        case 'c': swingUpCushionTest();    break;   // 起跳 + 回落缓冲
        case 'g': swingUpToBalance();      break;   // 起跳 → 平衡
        case 'd': directSweepTest();       break;   // 突然加速 扭矩特征扫描(电流逐档↑)
        case 'k': brakeSweepTest();        break;   // 蓄能急停 扭矩特征扫描(蓄能转速逐档↑)
        case 'm': confirmKickDirection();  break;   // ★方向确认(起跳/急救前必做)
        case 'v': diffDirectionVerify();   break;   // 差分方向验证(两向各一记，对比抵消共模污染)
        case '1': directSwingUpAction();   break;   // 突然加速·起跳（A/B→朝平衡）
        case '2': directRescueAction();    break;   // 突然加速·急救（第三面→回A/B）
        case '3': brakeSwingUpAction();    break;   // 蓄能急停·起跳（A/B→朝平衡）
        case '4': brakeRescueAction();     break;   // 蓄能急停·急救（第三面→回A/B）
        case 'e': driveToThirdFaceAction();break;   // 导向第三面（A/B→朝外踢到危险态）
        case 'x': recoverExperiment();     break;   // ②急救参数探测(确保第三面→逐档磕回A/B)
        case 'p': pureBalanceHold();       break;   // 纯平衡保持(手扶验证PID,不起跳)
        case 'n': directionProbe();        break;   // 方向净测(低速干净探,钉死作用量符号)
        case ']': swingUpSpinNudge(+1);    break;   // 起跳蓄能转速 +40rpm（g 起跳用）
        case '[': swingUpSpinNudge(-1);    break;   // 起跳蓄能转速 -40rpm
        case 's': { double p = 0, r = 0; imuReadAttitude(&p, &r); Serial.printf("# 姿态 pitch=%.1f roll=%.1f\n", p, r); break; }
        case 'h':
        default:  printMenu();             break;
    }
    motorPowerOff();   // 命令收尾：输出断开，回待命/监视
    Serial.println("# >>> done，回监视。");
}

void setup() {
    M5.begin(true, true, true);   // 串口 + 内部 I2C(IMU 在 Wire1) + 5×5 点阵
    M5.dis.setBrightness(20);
    imuInit();
    Serial.println();
    Serial.println("# [命令常驻] 固件烧一次，串口发命令触发流程，不必重烧。");
    if (!motorInit()) { Serial.println("ERR: 电机 I2C 初始化失败。"); }
    motorPowerOff();   // 待命：输出断开
    printMenu();
    M5.dis.fillpix(CRGB(0x00, 0x00, 0x20));  // 蓝：待命/监视
}

void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length()) dispatchCommand(line);
        return;
    }
    attitudeReportTick();   // 无命令 → 跑一个监视窗（紧凑日志）
}
