# CLAUDE.md

本文件在每次新会话启动时自动加载，用于把 Agent 引导到仓库内的持久化上下文。
**这是跨会话 / 跨机器续接工作的入口** —— 不要依赖 Claude Code 自带的会话 resume
（它绑定项目路径，仓库一旦移动/改名/克隆就会丢失上下文）。

## 新会话开局必读（按顺序）

1. `agent-context/README.md` —— 了解持久化存储系统的用途与约定。
2. `agent-context/INDEX.md` —— 总索引，快速定位所有上下文文件。
3. `agent-context/sessions/` 下**最新一篇**交接笔记 —— 了解上次停在哪、下一步做什么。

## 工作中要做的事

- 产生了不适合写进代码、但需长期留存的信息（原始需求、关键决策、硬件协议、
  会话进展）时，写入 `agent-context/` 对应子目录，并在 `INDEX.md` 登记。
- 每段会话结束前，在 `agent-context/sessions/` 留一份交接笔记（用 `_template.md`）。

## 项目速览

- 硬件：M5Stack ATOM Matrix（ESP32 + MPU6886 IMU + 5×5 RGB 点阵）。
- 构建：PlatformIO，env `m5stack-atom`，framework arduino。
- 串口：开发板为 `/dev/ttyUSB0`（FTDI, "M5stack"），监控 115200。
  监控用 `tools/serial_bridge.py`（单进程独占串口，并发收发；`pio device monitor`
  在无 TTY 环境不可用）。
- 目标系统：反作用轮 / 动量平衡（"屏动平衡"）。详见 `agent-context/`。
