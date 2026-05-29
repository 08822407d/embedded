// led —— 正面 5x5 LED 点阵控制模块
#pragma once

#include <M5Atom.h>  // 需要 CRGB 类型

// 【临时 / 暂未接入】行列方向开关：false=点亮“行”，true=点亮“列”。
// 最终要让点亮的线与系统底棱平行（与“静止态↔平衡态”旋转平面垂直的方向）。
// 框架打印已按此方向对齐，但代码侧暂无法确定落在行还是列，待现场可观察时固定。
extern bool g_lightColumn;

// 点亮第 index 条线（0..4），先清屏只显示该线；行/列由 g_lightColumn 决定。
// 注意：使用前需在 M5.begin(...) 第三参数启用 LED 点阵显示。
void lightLine(uint8_t index, CRGB color);
