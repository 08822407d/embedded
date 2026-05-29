# 上下文索引

> 一行一条，新增文件请在此登记。格式：`- [标题](相对路径) — 一句话钩子`

## 综述
- [系统模型与标定](system-model.md) — 单轴反作用轮倒立摆的简化模型 + IMU 姿态角实测标定
- [术语表](glossary.md) — 屏动平衡系统相关概念命名

## requirements/
- [001 屏动平衡系统](requirements/001-screen-balance-system.md) — 反作用轮倒立摆项目诉求与分步目标

## decisions/
- [001 IMU 姿态角读取方式与采样](decisions/001-imu-readout.md) — getAccelData+atan2、50ms、调试等级降为1

## protocols/
<!-- 例: - [串口命令协议](protocols/serial-command-protocol.md) — 反作用轮电机命令格式（待补） -->

## sessions/
- [2026-05-29 第1步 IMU 读取与标定](sessions/2026-05-29-step1-imu.md) — 环境/串口桥/存储系统/IMU标定
