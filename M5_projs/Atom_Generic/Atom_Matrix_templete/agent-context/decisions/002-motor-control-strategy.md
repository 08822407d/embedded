# 002 — 电机控制方式：电流（力矩）模式

- 日期：2026-05-29
- 状态：已采纳（编码前的方案确认，尚未实现）

## 背景
平衡三棱柱 / 莱洛三角属**反作用轮倒立摆**（Cubli、reaction-wheel pendulum 同族）。
需确定电机用哪种控制：角度(位置) / 速度 / 电流。

## 决策
**主控量 = 电流（力矩）模式 `ROLLER_MODE_CURRENT` + `setCurrent()`。**

## 理由
- 执行机构是飞轮，控制权威是**反作用力矩**，而 BLDC 力矩 T = Kt·I ⇒ 本质控制**电流**。
- 控制器（PID / LQR 状态反馈）由状态 `[机体角 pitch, 角速度 pitch_rate, 飞轮转速]`
  算出期望力矩，以电流设定值下发。
- **不用位置控制**：飞轮自身角度无意义；机体姿态角由 IMU(pitch) 测量（非电机）。
- **不用速度控制作主回路**：速度模式仅用于**外环动量管理/去饱和**，防飞轮转速饱和。

## 需读取的电机状态（库均提供，见 [protocols/rollercan-i2c.md](../protocols/rollercan-i2c.md)）
- 飞轮转速 `getSpeedReadback()÷100`（状态反馈 + 去饱和，核心）
- 实际电流 `getCurrentReadback()÷100`（力矩跟踪/诊断）
- 错误码 `getErrorCode()`（1过压/2堵转/4越界，安全）
- 输出状态 `getOutputStatus()`、系统状态 `getSysStatus()`、模式 `getMotorMode()`
- 健康：`getVin()÷100`、`getTemp()`

## 初始化顺序（电流模式）
1. `begin(&Wire, 0x64, 26, 32, <freq>)`（**freq 勿用 400000/100000**，危险频率，暂定 ~200000 待实测，见 [protocols/esp32-i2c-frequency-caveat.md](../protocols/esp32-i2c-frequency-caveat.md)）
2. `setOutput(0)`（改模式前先关输出）
3. `setMode(ROLLER_MODE_CURRENT)`
4. `setCurrent(0)`（初始零力矩，防使能跳变）
5. 查 `getErrorCode()`；若堵转锁定先解除
6. `setOutput(1)` 使能
7. 主循环：控制器算力矩 → `setCurrent(mA×100)`；周期读转速/错误做去饱和与保护

## 影响 / 代价
- 电流下发前需按 **±1200mA（raw ±120000）** 饱和裁剪。
- 需额外做**飞轮去饱和**逻辑，否则转速会单调累积至饱和而失去控制力矩。
