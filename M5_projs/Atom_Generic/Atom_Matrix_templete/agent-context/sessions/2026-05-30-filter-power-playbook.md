# 2026-05-30 — 滤波加固 / 速度模式 / 供电诊断与 12V 升级 / 起摆 playbook

> 本篇是 2026-05-30 这一整天的总交接（最新入口）。细节散见 decisions/003、004，protocols/rollercan-i2c，references/playbook。
> 接 [2026-05-30 互补滤波上线](2026-05-30-dir-test-with-filter.md)。

## 本次进展（时间顺序）
1. **IMU 互补滤波 + 实测验证**：pitch 陀螺(gy)主导互补滤波(α=0.98)取代原 accel 低通；伪象抑制 ~93%，方向辨识首次完整跑完（[003](../decisions/003-imu-filtering.md)）。
2. **陀螺标定加固**：`imuCalibrateGyro` 加丢弃首帧 + 运动门(被晃动则重标，≤5 次) + 返回 bool；点阵橙/绿/红状态灯（main 开了 5×5 显示）。
3. **lateral 也上陀螺融合**：发现强力矩下 accel 侧向角也有伪象（曾假报 22°），加 gx 互补融合后稳在 -1.8°；侧向危险阈按用户改 **42°**。
4. **速度模式实测：无改善**：飞轮仍顶 ~580rpm、机体 pf 仅 ±1.5° 抖动不脱阱；证实 **~600rpm 是 4.5V 硬墙、与模式无关**；高转速 gy ±80°/s 混叠会顶出融合角假尖峰（[004](../decisions/004-motor-power-and-mode.md)）。
5. **供电诊断**：加 `motorVoltage()`/CSV vin 列。命令 1200mA 实际仅 ~85mA、Vin≈4.5V 不掉压 → 定性为**低电压受限**。查证 m5-docs：RollerCAN **XT30 支持 6-16V**、Grove 仅 5V；16V vs 5V 力矩 0.065 vs 0.021 N·m（≈3×）。
6. **用户接入 ~12V 到 XT30**（Grove 5V **未断**，两路并存）。问过断不断 5V：内部有 DC-DC 降压，15V 不会倒灌 Grove 5V 脚；保守做法是断 Grove 5V 只留 GND/SDA/SCL，但用户先不断、实测并存无异常。
7. **12V 台架能力实测**（不做起跳，机体受硬线约束，只看电机读数）：Vin=12.65V err=0；**最大飞轮转速 ≈1900rpm（4.5V 时 600，≈3×）**；空载峰值电流 ~240mA（空载被 back-EMF 限流，带载可到规格 ~900mA）。authority ≈3×，足够起跳。
8. **起摆 playbook**：调研 Cubli/RWP 专著/论文/实战项目，整理成 [references/reaction-wheel-pendulum-playbook.md](../references/reaction-wheel-pendulum-playbook.md)（含★问题→优先排查表），作为后续遇问题的优先判断路径。

## 当前状态
- **能跑**。板上固件 = `motorBenchTest`（main.cpp，电流模式，12V 台架测试）。电机**测试后已断电空闲**。
- 代码模块：`imu.*`(pitch+lateral 双互补滤波、零偏标定加固)、`motor.*`(电流+速度两种 init、Vin/温读)、`dir_test.*`(含 dirTestRun/breakawayTest/speedModeTest/poweredBreakawayTest/motorBenchTest 多个测试入口 + 共用 prepAtRestB/sampleAndLog/各看门狗)、`led.*`(未用)、`main.cpp`(装配)。
- 工具：`tools/capture_run.py`（DTR/RTS 复位 ESP32 后从 boot 抓 CSV，最稳的采集方式）。
- 供电：XT30 接 ~12V + Grove 5V 并存，实测正常（Vin 12.65V，无过压/错误）。

## 未完成 / 待办（按 playbook 流程）
- [ ] **走线**：XT30 供电线太硬，会约束/干扰机体摆动 → 看机体运动的测试(起跳/起摆/平衡)做不准，需先解决（柔性细线/理线/滑环）。
- [ ] **抗混叠**：开 MPU6886 片上 DLPF 或提采样率（现 50Hz），压高转速 gy 混叠。
- [ ] **摩擦辨识** + **线性化建模**（拉格朗日→状态空间）。
- [ ] **平衡 LQR/状态反馈骨架**（θ, θ̇, φ̇；或先串级：外环角→内环飞轮速度，对参数不敏感）。
- [ ] **能量法起摆** + **交接迟滞** + **退饱和**。

## 下一步
走线解决后，回到**电流模式起跳**（`poweredBreakawayTest` 已就绪：极小电流起步、脱阱即停、遇险只断电、Vin 守护）实测 12V 下能否真脱阱；并行可先起"摩擦辨识/平衡 LQR 骨架"（不依赖机体运动的部分）。遇问题先查 playbook 第 5 节排查表。

## 备注 / 坑
- **飞轮转速饱和 = 头号敌人**（playbook）；平衡阶段要做退饱和。
- **BLDC 正反不对称**（无主动刹车）经典坑，电流模式+主动刹车要盯。
- **平衡点≠0°**：质心偏置体现在实测平衡角，用 A(-31)/平衡(0)/B(+30) 标定值作 setpoint。
- 真力矩下**单次硬冲量可能一个采样周期就甩过棱边**，无人值守跑起跳建议有人看护。
- 采集：`python3 tools/capture_run.py <out> <秒>`；`pkill -f serial_bridge.py` 会误杀自身 shell，用 pgrep+comm 过滤。
- I²C：勿用 100k/400k（危险频率）；平衡高频回路要减每周期 I²C 操作、勿运行中切电机模式。
