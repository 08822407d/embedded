# 上下文索引

> 一行一条，新增文件请在此登记。格式：`- [标题](相对路径) — 一句话钩子`

## 综述
- [系统模型与标定](system-model.md) — 单轴反作用轮倒立摆的简化模型 + IMU 姿态角实测标定
- [术语表](glossary.md) — 屏动平衡系统相关概念命名

## requirements/
- [001 屏动平衡系统](requirements/001-screen-balance-system.md) — 反作用轮倒立摆项目诉求与分步目标

## decisions/
- [001 IMU 姿态角读取方式与采样](decisions/001-imu-readout.md) — getAccelData+atan2、50ms、调试等级降为1
- [002 电机控制方式：电流(力矩)模式](decisions/002-motor-control-strategy.md) — 反作用轮用电流模式，状态读取项与初始化顺序

## protocols/
- [RollerCAN I²C 控制](protocols/rollercan-i2c.md) — 接线/地址0x64/官方库 UnitRollerI2C API/定标/帧协议
- [ESP32 I²C 频率坑](protocols/esp32-i2c-frequency-caveat.md) — 危险频率 100kHz 与 400kHz(M5 ATOM同款实测高延迟)均勿用，工作频率暂定~200kHz待实测

## sessions/
- [2026-05-29 第1步 IMU 读取与标定](sessions/2026-05-29-step1-imu.md) — 环境/串口桥/存储系统/IMU标定
- [2026-05-29 电机模块文档调研](sessions/2026-05-29-motor-module-research.md) — RollerCAN I²C/库API/控制方案确认(未编码)
