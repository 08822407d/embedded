# ESP32 Arduino I²C(Wire) 频率坑（备查）

> 适用于本平台所有 I²C 设备（IMU + RollerCAN）。记录两个频率相关 bug 及在本项目版本的状态。
> 保留来源链接以便**后续版本复核**（可能进一步修复或回归）。整理日期 2026-05-29。

## 本项目版本
- arduino-esp32 **3.2.0** / esp-idf **5.4.1**，使用 IDF5 新 `i2c_master` 驱动。

## 坑 1：100 kHz 事务间 ~10ms 大延迟
- **现象**：I²C 跑在 **正好 100 kHz** 时，相邻事务之间出现 ~7–10ms 间隙（疑与 FreeRTOS tick 耦合）。
- **边界**：**≥125 kHz** 消失；100–125 kHz 之间部分频率仍异常。
- **状态**：esp-idf issue 标记 Done（内部已解决），但**未注明具体修复版本** → 复核需实测。
- 来源：https://github.com/espressif/esp-idf/issues/8770 ，论坛 https://esp32.com/viewtopic.php?t=15999

## 坑 2：>142857 Hz（含 400kHz）断言崩溃
- **现象**：频率 >142857 Hz 时 HAL `assert(scl_high > 13)` 失败 → 崩溃。
- **影响版本**：IDF **5.1** 引入的回归（2023-10）。
- **修复**：arduino-esp32 **3.0.0**（PR #8777）。我们 3.2.0 > 3.0.0 → **已修复**。
- 来源：https://github.com/espressif/arduino-esp32/issues/8772

## 对本项目的处置（结论）
- **统一用 400 kHz**（RollerCAN 即此）：绕开坑 1（非 100kHz），坑 2 已在 3.2.0 修复。
- **任何 I²C 不要设成正好 100 kHz**；要慢用 ≥125 kHz。
- **实测佐证**：3.2.0 上 MPU6886 经 I²C 稳定读取（串口 `i2cWriteReadNonStop` 无异常延迟）。
- **复核时机**：升级 arduino-esp32 / esp-idf 时，回看上面两个 issue，确认状态有无变化。

## 关联
- 电机 I²C 用法：[rollercan-i2c.md](rollercan-i2c.md)
