# 2026-06-01（跨夜）— 回归电流模式 / 命令常驻架构 / 系统辨识(方案B)

接 [2026-05-31 启动序列/起跳/兜底](2026-05-31-startup-seq-swingup-recovery.md)。本段围绕"回归电流模式 + 命令工作流 + 系统辨识"。

## 本次进展
1. **方案回归电流(力矩)模式**（调研主流 SimpleFOC/Cubli：起跳+平衡全程力矩/电压模式、不切换、无人用速度控制；
   执行权威是扭矩）。核心活跃流程(探扭矩/起跳缓冲/起跳→平衡/balance)全转电流模式；速度模式旧函数搁置待清。
   - 物理模型统一写入 [system-model §6](../system-model.md)：`I_b·θ̈=m·g·l·sinθ−τ`；τ=电流(力矩)直接施加。
   - 决策记录 [decisions/006](../decisions/006-fixed-speedmode-pipeline.md) 顶部"控制模式回归电流"段。
2. **探"能起跳的扭矩" τ_break（电流模式）实测成功**：逐增电流朝平衡，**τ_break≈260mA**(100/180mA 推不动、260 挣脱)，方向 −1。
3. **起跳+回落缓冲(电流模式) run_cush1 成功**：电流脉冲起跳(朝平衡 11.2°) + 反角速度阻尼力矩缓冲(τ=θdown·K·θ̇)，
   机体平稳落回 B、朝外最远仅 3.1°(没翻)、lat 全程 ~−2°(没出平面)。缓冲有小振荡(K_DAMP=4 偏小，待整定)。
4. **平衡控制器 `src/balance.*`**（PID 主用/LQR 备用）：电流模式**直接输出力矩(mA)**；增益占位待整定。
5. **★命令常驻架构 + 串口急救**（`src/main.cpp`）：固件**烧一次**，串口发字符命令触发流程，**不必重编烧**：
   - 命令：`r`=急救(大力矩磕回A/B) `i`=系统辨识 `b`=探扭矩 `c`=起跳+缓冲 `g`=起跳→平衡 `s`=姿态 `h`=菜单。
   - 发命令工具：`tools/serial_bridge.py`(单进程独占串口、并发收发)→ `echo "i" > tools/serial.cmd`，日志见 `tools/serial.log`。
   - **急救 `r` 实测成功**：机体卡第三面 83° → 蓄能+全力 ±1200mA 急刹 → 磕回 B。**以后卡第三面发 `r` 即可，不用扶/不用重烧。**
6. **日志改高频紧凑机器格式**（[references/log-format.md](../references/log-format.md)）：10列定点整数+`phase`(代码位置)+`iter`(循环计数)，
   `TICK_MS 20→8` ⇒ **≈83Hz**(实测 t 间隔 12ms)。供离线精密分析。
7. **系统辨识 方案B（离线拟合，已定）**：MCU 跑 `sysIdExperiment` 只激励+记录，**我离线拟合 I_b/Kt/mgl**(不在 MCU 在线拟合，避免发散)。

## ⚠️ 待办（下次第一件事）
- **采温和辨识数据**：`sysIdExperiment` 已调温和(短脉冲80ms + 270/330/390mA + STOP_DEG=8，机体小幅离阱再摆回、不冲过平衡)。
  上次采集失败是**bridge 多实例争串口**(见坑)，不是固件问题。流程：① 确认 bridge 全清 → 启**单个** → 机体若卡第三面发 `r` 救回 →
  机体在 B 后发 `i` 采 3 档 SID_DRV/SID_FREE → ② 我离线拟合 **I_b/Kt/mgl** → 算 **K_DAMP(临界阻尼)/PID增益** → 写回
  `balance.cpp`、`swingUpCushionTest` → ③ 验证回落缓冲更顺、平衡能稳。
- 清理速度模式遗留死函数(stepwise/oneshot/measureSwingUpDir/cancel/finalize/swingUpSpeed)。

## 坑（务必注意）
- **serial_bridge 多实例**：每次启动前**务必 `pkill -9 -f serial_bridge.py` 并 pgrep 确认无残留**，否则多个实例争串口→命令不通/采不到数据。
  bridge 阻塞在串口 readline 是 **D(不可中断)状态**，SIGKILL 要等 readline 的 1s 超时才真死——杀后 `sleep 2` 再确认。
- `echo > serial.cmd`(FIFO) 若无 bridge reader 会**阻塞**；命令前确认 bridge 活。
- 机体仍**反复卡第三面**(USB/供电硬线张力+侧向不稳)；命令架构的 `r` 急救已能随时救回，但根治还需稳固机械装夹。
- 已采的旧辨识数据(23:31 的 −280mA 单档 12帧)太猛(挣脱后 0.14s θ̇→−160°/s、冲过平衡)，**欠定**(分不开 I_b/Kt/mgl)，弃用；用温和 3 档重采。

## 杂项
- 应用户要求把**本地显示器调暗**：`DISPLAY=:1 xrandr --output DP-0/DP-2 --brightness 0.3`(双屏)。无 backlight/ddcutil(台式机软件 gamma)。
- 采集仍可用 `tools/capture_run.py <out> <秒>`(复位从 boot 抓，单次)；命令工作流用 `serial_bridge.py`(常驻收发)。
