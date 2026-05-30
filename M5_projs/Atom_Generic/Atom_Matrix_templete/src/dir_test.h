// dir_test —— 电机旋转方向辨识 + 防翻保底
//
// 目的：从静止态 B 出发，用渐增的小电流脉冲驱动飞轮，借 IMU(pitch) 观察机体
//       转动方向，辨识出"使 pitch 减小(朝平衡)"对应的电流符号。
// 安全：全程内嵌角度看门狗。一旦方向错误使机体冲过危险角(越过 A/B 外棱)，
//       先尝试全力反向恢复；**若限时内仍无法回到静止态 A/B，则直接断电**
//       （setOutput(0)），防止飞轮空转烧毁/耗电。
// 形态：一次性阻塞执行（当前顶层测试程序）。需在 imuInit/motorInit 之后调用。
#pragma once

// 运行一次方向辨识（含保底）。结果与过程打印到串口。
void dirTestRun();

// 速度模式 authority 测试：逐级提高目标转速，对比电流模式的扭矩/摆幅。
//   需在 imuInit/motorInitSpeed(速度模式) 之后调用。含速度模式专用断电保底。
void speedModeTest();

// 上电(XT30 12-15V)谨慎起跳表征：电流模式，仅朝平衡、极小电流起步、脱阱即停、遇险只断电。
//   需在 imuInit/motorInit(电流模式) 之后调用。含 Vin/过压自检。
void poweredBreakawayTest();

// 电机台架能力测试：测 12V 下最大实际电流/最大飞轮转速，**不做起跳**（机体受硬线约束时用）。
//   交替方向短脉冲，仅看电机读数；含 Vin/过压/超速/侧向看门狗。需在 motorInit(电流模式) 后调用。
void motorBenchTest();

// 辨识结果：使 pitch 减小(朝平衡)的电流符号，+1/-1；0=未知/未测出。
int dirTestRestoreCurrentSign();
