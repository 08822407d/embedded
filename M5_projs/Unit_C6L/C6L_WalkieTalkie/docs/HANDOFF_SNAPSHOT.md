# HANDOFF_SNAPSHOT — 跨环境交接单(2026-06-21)

> 用户次日可能在**其他开发环境**继续。本快照让另一台机器/全新会话正确接手。
> 接手顺序:`CLAUDE.md` → `docs/STATUS.md` → 本文件。历史细节见 `git log`。

## 1. 进度指针(现在到哪了)
- **持久化体系**已建(CLAUDE.md + docs/ + .claude/settings.json),并入 git。
- **LoRa 点对点通信测试 ✅ 实测通过**:两块 Unit-C6L(SX1262)双向 **0% 丢包**、SNR 5~9dB(FACTS F‑21)。
- 对讲机**正式需求尚未展开**(LoRa 既已验证,射频路线基本明朗)——下次从"需求讨论"接着谈,落 `docs/REQUIREMENTS.md`。
- 提交状态:本快照所在 commit 即最新进度(`git log -1`);提交后工作区应无本项目未提交改动。

## 2. 不随仓库走的东西(换机/换环境务必注意)
- **Claude Code 全局记忆 `~/.claude/.../MEMORY.md` 不随仓库迁移**(按绝对路径 `/home/cheyh/projs/embedded` 作键;换 home / 换 Windows 失效)。本项目要点已尽量内联进 `CLAUDE.md` + `docs/`,**以仓库为准**。其中跨会话仍适用的关键约定(新环境若无全局记忆,记得照做):
  - 回复用中文;工程严谨、不谎报、不臆造。
  - 操作红线:未经本会话明确指示**不编码/不烧录/不持续发射射频**。
  - 实现尽量交 codex 省 Claude 用量;CC 主做需求/设计/验收。
  - 烧录/读串口前先**设备探测**(别假定串口名、别假定连着的就是 C6)。
- **工具链(关键坑)**:平台要求 **pio Core ≥ 6.1.19**(F‑12)。新机若是 6.1.18 会报 `IncompatiblePlatform`。Ubuntu(PEP 668)就地升级:`python3 -m pip install --user -U platformio --break-system-packages`。`RadioLib`/`M5Unified` 由 `pio run` 按 `platformio.ini` 自动拉取;pioarduino 平台首次 `pio run` 自动安装(耗时)。
- **联机状态**(机器相关):本机两块 C6 MAC = `58:8C:81:50:04:38`(PINGER)/ `58:8C:81:50:06:E4`(PONGER);端口随机器变,新机按 `OPERATIONS.md`『设备探测』重测。⚠️ `src/main.cpp` 里 **MAC_A 硬编码**这对板,**换板需改**角色判定。
- **codex 沙箱**:本轮出现过 `bwrap` 挡文件系统的故障;新环境派 codex 前先跑 `codex:setup` 自检。

## 3. 跨 OS 注意(Ubuntu ↔ Windows)
- 串口名:Linux `/dev/ttyACM*` vs Windows `COM*`(代码/脚本不写死,靠探测)。
- `tools/lora_capture.py` 依赖 **pyserial**(本机系统 python3 自带 3.5;新机可能需 `pip install pyserial`);RTS 复位脉冲对齐方式跨平台通用。
- 工具链修复脚本 `scripts/fix_toolchain_path.py` 已处理 `os.name=='nt'`。

## 4. 复现当前测试(冒烟)
1. 两块 Unit-C6L 各接 **LoRa 频段天线**(⚠️ 别空载发射,F‑19a);USB 连主机。
2. `pio run` 编译通过。
3. **确认天线后**烧两块:`pio run -t upload --upload-port <PORT>`(端口先探测)。
4. `python3 tools/lora_capture.py --reset` → 预期双向 **0% 丢包**。
- **LoRa 关键复现要点(F‑21)**:SX1262 必须 `begin()` 后 `setDio2AsRfSwitch(true)` + 收包走 **DIO1 中断**(`setPacketReceivedAction`,非 `available()` 轮询),否则双向 0 通。测试 14dBm 近距 RSSI 会饱和(~0dBm),测距离需降功率/拉远。

## 5. 接手三步
1. 在新机 clone/pull 本仓库(`origin` = `github.com:08822407d/embedded`)。
2. 读 `CLAUDE.md` → `docs/STATUS.md` → 本快照。
3. `pio run` 编译通过即代表环境就绪,接着干(先确认 pio ≥6.1.19,见 §2)。
