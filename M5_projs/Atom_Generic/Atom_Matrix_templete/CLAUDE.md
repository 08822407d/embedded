# CLAUDE.md

本文件在每次新会话启动时自动加载，用于把 Agent 引导到仓库内的持久化上下文。
**这是跨会话 / 跨机器续接工作的入口** —— 不要依赖 Claude Code 自带的会话 resume
（它绑定项目路径，仓库一旦移动/改名/克隆就会丢失上下文）。

## Claude Code 行为约束（必须遵守）

1. **持久化存储优先**：本项目用 `agent-context/`（随 Git 走，抗迁移、抗上下文压缩）做本地永久存储。
   - 每个新任务开局**必须先读**它（见下"新会话开局必读"）。
   - 一旦产生值得长期保留的成果（原始需求 / 关键决策 / 硬件协议 / 标定数据 / 踩过的坑），
     就**主动写入** `agent-context/` 对应子目录并更新 `INDEX.md`，不要只留在对话里等丢失。

2. **缺工具先问、不要硬解**：当某项工作需要某个工具（如解析 PDF、特定 CLI）才能高效完成，
   而本机没有该工具时，**先提示用户是否安装**，由用户决定。
   - **禁止**用二进制硬解、手工逆向等低效绕路方式强行处理——既容易出错，又大量浪费时间和 token。

3. **本地优化优先（省 token/算力）**：凡是**耗 token 且能程序化、适合本地跑**的工作（解析大段日志、
   计算/统计/拟合、批量数据处理、波形诊断等），**首先考虑写成本地脚本在本地完成**，模型只读**精简结论**，
   不要把大量原始数据塞进模型用算力硬解。已有：`tools/analyze.py`(实验日志分段+自动诊断)、
   `tools/fit_sysid.py`(系统辨识拟合)。新出现的此类需求，优先扩充/新建本地脚本。

## 新会话开局必读（按顺序）

1. `agent-context/README.md` —— 了解持久化存储系统的用途与约定。
2. `agent-context/INDEX.md` —— 总索引，快速定位所有上下文文件。
3. `agent-context/sessions/` 下**最新一篇**交接笔记 —— 了解上次停在哪、下一步做什么。

## 工作中要做的事

- 产生了不适合写进代码、但需长期留存的信息（原始需求、关键决策、硬件协议、
  会话进展）时，写入 `agent-context/` 对应子目录，并在 `INDEX.md` 登记。
- 每段会话结束前，在 `agent-context/sessions/` 留一份交接笔记（用 `_template.md`）。

## 项目速览

- 硬件：M5Stack ATOM Matrix（ESP32 + MPU6886 IMU + 5×5 RGB 点阵）+ RollerCAN 反作用轮电机。
- 构建：PlatformIO，env `m5stack-atom`，framework arduino。
- 串口：开发板为 `/dev/ttyUSB0`（FTDI, "M5stack"），监控 115200。
  监控用 `tools/serial_bridge.py`（单进程独占串口，并发收发；`pio device monitor`
  在无 TTY 环境不可用）。
- 电机：RollerCAN 经 I²C(地址 0x64，Grove SDA=26/SCL=32) 控制，用官方库 `M5Unit-Roller`
  的 `UnitRollerI2C`，**电流(力矩)模式**。详见 `agent-context/protocols/rollercan-i2c.md`、`decisions/002`。
- 目标系统：反作用轮倒立摆（"屏动平衡"，单轴，被控量 pitch，平衡点 0°）。详见 `agent-context/`。

## 代码组织约定（务必遵守）

- 按功能分模块，**不要把代码都堆进 `main.cpp`**；main 只做初始化与主循环装配。
- 现有模块：`src/imu.*`（IMU 姿态）、`src/led.*`（LED 点阵，临时）；后续电机 → `src/motor.*`。
- 模块间通过头文件接口交互。
