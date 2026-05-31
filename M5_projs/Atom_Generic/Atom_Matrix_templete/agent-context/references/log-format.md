# 紧凑机器日志格式（高频精密分析用，2026-05-31）

> `sampleAndLog()` 每帧输出一行，**为离线数学分析设计、不为人读**。≈83Hz（`TICK_MS=8` + 采样~4ms ⇒ 帧间隔 ~12ms）。

## 数据行格式（10 列，定点整数）
```
phase,iter,t_ms,pf_cdeg,gy_cdps,cmd_ma,cur_ma,spd_rpm,lat_cdeg,err
```
| 列 | 含义 | 单位/换算 |
|----|------|-----------|
| `phase` | **代码位置标识**(字符串) | 看出执行哪段代码：SID_DRV/SID_FREE(辨识) · CUSH_KICK/CUSH_DAMP(起跳/缓冲) · BAL_KICK/BALANCE · TQ(探扭矩) · MON(监视) · WAIT/CHECK(prepAtRest) 等 |
| `iter` | **该 phase 内循环帧号**(0 起，phase 变即重置) | 看出循环了多少帧/进度 |
| `t_ms` | millis 时间戳 | ms（算 dt / 一阶二阶微分） |
| `pf_cdeg` | 融合 pitch θ ×100 | 厘度（÷100=°） |
| `gy_cdps` | 机体角速度 θ̇ ×100（已扣零偏） | ×100 °/s |
| `cmd_ma` | 命令电流（=施加扭矩输入） | mA |
| `cur_ma` | 实际电流读数 | mA |
| `spd_rpm` | 飞轮转速 | rpm |
| `lat_cdeg` | 侧向(出平面)角 ×100 | 厘度 |
| `err` | 电机错误码 | 1过压/2堵转/4越界 |

## 注释行
以 `#` 开头：阶段标记(如 `# == 辨识档1: 驱动电流 -280mA ==`)、判定(`# === 1s窗判定…`)、表头、ABORT 等。**非数据，解析时跳过**。

## 用途
- 估 θ̈：对 `gy_cdps`(θ̇) 按 `t_ms` 求一阶差分（或对 pf 求二阶）。
- 辨识：`I_b·θ̈ = Kt·cmd_ma + mgl·sin(θ)` → 用 (θ̈, cmd, θ) 回归估 Kt/I_b、mgl/I_b。
- 诊断：`phase`+`iter`+波形定位"哪一步出问题"（如施加 `cmd`<0 朝平衡但 `pf` 反而增大 ⇒ 初始扭矩方向判反）。
- 注：之前 17 列浮点格式(含 ax/ay/az/gx/gz/roll/accel-pitch/vin)已弃用，换此紧凑格式提采样率。
