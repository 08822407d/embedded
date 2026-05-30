# 上下文索引

> 一行一条，新增文件请在此登记。格式：`- [标题](相对路径) — 一句话钩子`

## 综述
- [系统模型与标定](system-model.md) — 单轴反作用轮倒立摆的简化模型 + IMU 姿态角实测标定
- [术语表](glossary.md) — 屏动平衡系统相关概念命名

## references/（外部成熟经验）
- [反作用轮倒立摆 Playbook + 问题排查路径](references/reaction-wheel-pendulum-playbook.md) — 理论/流程/数据处理/控制/★常见问题优先排查表；遇到问题先查这里

## requirements/
- [001 屏动平衡系统](requirements/001-screen-balance-system.md) — 反作用轮倒立摆项目诉求与分步目标

## decisions/
- [001 IMU 姿态角读取方式与采样](decisions/001-imu-readout.md) — getAccelData+atan2、50ms、调试等级降为1
- [002 电机控制方式：电流(力矩)模式](decisions/002-motor-control-strategy.md) — 反作用轮用电流模式，状态读取项与初始化顺序
- [003 IMU 姿态滤波：单轴 pitch 互补滤波](decisions/003-imu-filtering.md) — 飞轮转动时加速度角失真(实测+14°伪象)，改陀螺主导互补滤波(gy)，含零偏标定与延迟分析
- [004 电机供电与控制方式](decisions/004-motor-power-and-mode.md) — Grove 5V 供电不足致推不动；速度模式无改善(同~600rpm)；查证 XT30 接 12-16V 可得 3× 力矩

## protocols/
- [RollerCAN I²C 控制](protocols/rollercan-i2c.md) — 接线/地址0x64/官方库 UnitRollerI2C API/定标/帧协议
- [ESP32 I²C 频率坑](protocols/esp32-i2c-frequency-caveat.md) — 危险频率 100kHz 与 400kHz(M5 ATOM同款实测高延迟)均勿用，工作频率暂定~200kHz待实测

## sessions/
- [2026-05-29 第1步 IMU 读取与标定](sessions/2026-05-29-step1-imu.md) — 环境/串口桥/存储系统/IMU标定
- [2026-05-29 电机模块文档调研](sessions/2026-05-29-motor-module-research.md) — RollerCAN I²C/库API/控制方案确认(未编码)
- [2026-05-29 电机方向辨识进度(WIP)](sessions/2026-05-29-motor-dir-test.md) — 编码+实测；发现加速度动态不可信/需陀螺、需挣脱力矩；防翻保底(面内+侧向)
- [2026-05-30 互补滤波上线+方向辨识重测](sessions/2026-05-30-dir-test-with-filter.md) — 滤波器实测有效(伪象抑制93%/0误触发)；方向=负电流朝平衡；陀螺零偏会漂；高转速混叠；挣脱力矩不足需动量泵起摆
- [2026-05-30 滤波加固/速度模式/12V升级/playbook（总交接）](sessions/2026-05-30-filter-power-playbook.md) — ★当天总入口：校准加固、速度模式无改善、供电诊断、XT30接12V转速×3、起摆playbook、待办与下一步
