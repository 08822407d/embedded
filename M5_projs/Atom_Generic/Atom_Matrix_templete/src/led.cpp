#include <M5Atom.h>

#include "led.h"

bool g_lightColumn = false;

void lightLine(uint8_t index, CRGB color) {
    if (index > 4) return;
    M5.dis.clear();
    for (uint8_t k = 0; k < 5; k++) {
        if (g_lightColumn)
            M5.dis.drawpix(index, k, color);  // 第 index 列（x=index 固定，y 扫描）
        else
            M5.dis.drawpix(k, index, color);  // 第 index 行（y=index 固定，x 扫描）
    }
}
