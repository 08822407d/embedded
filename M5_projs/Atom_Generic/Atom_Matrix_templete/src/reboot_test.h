// reboot_test —— 双路供电(USB-C + RollerCAN 12V 经 Grove 回灌)下的复位/重启诊断
//
// 目的：判断两路电源同时存在时，开发板能否正常复位，以及每次复位是
//       "热复位(板子全程有电)" 还是 "掉电冷启动"。
// 手段：RTC 内存计数器(掉电会丢、热复位会留) + esp_reset_reason() + 心跳 + 点阵在线指示。
#pragma once

void rebootTestSetup();  // 在 M5.begin 之后调用一次
void rebootTestLoop();   // 主循环里每秒一次
