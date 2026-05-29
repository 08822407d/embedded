# 001 — IMU 姿态角读取方式与采样

- 日期：2026-05-29
- 状态：已采纳

## 背景
第 1 步需把 MPU6886 读数换算成姿态角，确定可动轴与平衡点。

## 决策
- 用 `M5.IMU.getAccelData()` 取加速度，**自行 atan2 换算** pitch/roll，不用库的 `getAttitude()`。
- 采样间隔 **50ms（20Hz）**，沿用原工程。
- `platformio.ini` 的 `CORE_DEBUG_LEVEL` 由 **5 改为 1**。

## 理由
- 库 `getAttitude()` 用 `asin(-accX)`/`atan(accY/accZ)`，量程受限（±90°、无象限）；
  `atan2` 给完整量程、象限正确，且换算透明可控。
- 50ms 既满足实时性，又与既有工程一致。
- `CORE_DEBUG_LEVEL=5` 会让每次 I2C 读打印一行 `[V]` verbose 日志，淹没姿态数据；
  降到 1（仅错误）后串口输出干净。

## 影响 / 代价
- 当前 pitch 为**纯加速度计静态角**，动态下受运动加速度干扰，后续闭环控制需加陀螺仪融合。
- 烧录本工程固件**覆盖了板上原有的电机/平衡控制固件**（无源码，不可找回）；用户已授权从头开始。

## 相关
- 模型与标定：[../system-model.md](../system-model.md)
- 监控工具：`tools/serial_bridge.py`（详见 [会话记录](../sessions/2026-05-29-step1-imu.md)）
