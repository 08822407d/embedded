# STATUS — 当前状态(恢复锚点)

> 这是新会话的第一落点。**只记"现在"**;历史交给 `git log`,别在这里堆。
> 每段工作收尾必须更新本文件并 `git commit`。

- **更新时间**:2026-06-25 00:22 +08:00(本机时间;任务从 2026-06-24 开始,跨午夜收尾)
- **当前阶段**:RNode 移植到 Unit-C6L 已完成,USB-CDC 路径已在 Reticulum 下双机验收通过。C6L 作为 LoRa/RNode modem 可用;Grove UART 环境已实现并编译通过,待接线后实测。
- **联机快照(2026-06-24/25,易变)**:探测到 2 个 `303A:1001` ESP32-C6 设备:`58:8C:81:50:04:38`@`/dev/ttyACM0`、`58:8C:81:50:06:E4`@`/dev/ttyACM1`;两块均已烧录 USB-CDC RNode 固件并通过 `rnodeconf`/`rnstatus`/Reticulum 传输测试。

## 进行中
- **当前主线已交付**:`RNode_Firmware/` 内新增 C6L RNode 固件目标,默认 USB-CDC;`m5stack-unitc6l-rnode-uart` 为 Grove UART 编译目标。
- **验收记录**见 `docs/RNODE_C6L_PORT.md`:移植点、Reticulum 配置、复测命令、双向文件校验、RSSI/SNR 结果和已知现象都已落盘。
- **未提交**:本轮没有自动 git commit。当前工作区还有前序 docs 脏改和未跟踪 RNode 子目录;提交前需决定 `RNode_Firmware/` 作为 vendored source 还是 submodule/patch 流。

## 已完成
- **RNode 移植 PASS(2026-06-24/25)**:`RNode_Firmware/platformio.ini` 新增 `m5stack-unitc6l-rnode` 与 `m5stack-unitc6l-rnode-uart`;C6L 板定义加入 SX1262 引脚、DIO2 RF switch、DIO1 IRQ、PI4IOE5V6408 控 NRST/ANT_SW/LNA、自动 EEPROM provision、USB-CDC/Grove UART host 串口路径。
- **编译 PASS(2026-06-25 00:xx +08:00)**:`pio run -e m5stack-unitc6l-rnode` 成功(RAM 11.9% / Flash 29.0%);`pio run -e m5stack-unitc6l-rnode-uart` 成功(RAM 11.9% / Flash 30.1%)。
- **烧录与 Reticulum 双机验收 PASS**:两块 C6L 均烧录 USB-CDC RNode 固件;`rnodeconf` 识别为 `RNode 820 - 960 MHz (03:a6:45)`/SX1262;`rnstatus` 两端 `Up`;自定义 proof probe A->B/B->A 成功(SNR 约 12.5dB);`rncp` A->B 768B、B->A 640B 二进制传输 `cmp`/sha256 一致,覆盖 >255B 分片重组可靠传输。
- **主机工具**:`rns` 已通过 `python3 -m pip install --user --break-system-packages -U rns` 安装到用户环境,版本 1.3.5;新增 `tools/reticulum_packet_probe.py` 用于 Reticulum packet/proof echo。
- 持久化与接手协议落盘(DECISIONS #000)。
- 已核实的硬件 / 构建事实入账:FACTS F‑1~F‑7(目标板、平台、库、USB-CDC、工具链修复脚本、骨架现状、git 未入库)。

## 下一步(路线 A)
- [x] `RNode_Firmware/` 入库形态**已定 = patch 流**(DECISIONS #004):clone 被 gitignore,提交 `patches/rnode_c6l.patch` + `tools/setup_rnode_firmware.sh` + 再移植指南。重建:`bash tools/setup_rnode_firmware.sh`。
- [ ] Grove UART 接线后烧录 `m5stack-unitc6l-rnode-uart`,用同一套 Reticulum/rncp/proof probe 复测。
- [ ] 查一次性启动 `Radio state mismatch` 后自动恢复的原因;当前不阻塞使用。
- [ ] (可选)降功率 / 拉远测真实 RSSI 与距离;近距 RSSI 0dBm 属饱和读数,不要拿来标定链路预算。

## 卡点 / 待澄清
- Grove UART 路径尚未接线实测;USB-CDC 路径已通过。
- `rnodeconf` 签名 warning 属本地自定义固件预期现象,不是通信故障。

## 交给 Codex 的状态
- **✅ codex RNode 移植交付(2026-06-24/25)**:`RNode_Firmware/` 完成 C6L RNode 固件适配并烧录两块板;Reticulum USB-CDC 双机验收通过;详见 `docs/RNODE_C6L_PORT.md`。
- **✅ codex 已交付(2026-06-21)**:`src/main.cpp`(LoRa ping-pong,按 MAC 分角色:`...0438`=PINGER / `...06E4`=PONGER,MAC_A 硬编码这对板)+ `platformio.ini` 加 `RadioLib@>=7.3.0`。`pio run` 通过(RAM 14.3% / **Flash 89.3%**,高是 M5Unified/M5GFX 字体所致,够用但余量小,后续做音频需关注)。**未烧录**。
- **✅ codex 改进交付(2026-06-21)**:有界运行 + `radio.sleep()` 关射频兜底(DECISIONS #001)、`while(!Serial)` 等串口再发以对齐日志、新增主机 `tools/lora_capture.py`(按 MAC 探口 + 置 DTR + 可选 RTS 复位对齐 + 双口时间戳记录 + 末尾汇总)。`pio run` 通过(Flash 89.4%)。
- **✅ LoRa 通信测试 PASS(2026-06-21)**:首版 100% 丢包 → 定位为缺 DIO1 中断收包 + `setDio2AsRfSwitch(true)`(F‑21);修复后复测 **28/28、双向 0% 丢包、SNR 5~9dB**。日志 `lora_logs/*_211539.txt`。RSSI 近距饱和(~0dBm),测距离需降功率/拉远。
- 注(2026-06-21 历史):当时 codex 沙箱故障(bwrap 挡了文件系统读写),降级为只给方案,修复由 CC 手改 main.cpp 应用。2026-06-24/25 本轮已可正常读写、编译、烧录和测试。

## 开放问题
- (已解决)`RNode_Firmware/` 入库 = patch 流(DECISIONS #004);重建脚本 `tools/setup_rnode_firmware.sh`。
- Grove UART 路径尚未接线实测。
- 初次 RNodeInterface 打开偶发 `Radio state mismatch` 后自动恢复,后续可继续查。
- 图片等大载荷走 LoRa 耗时数分钟~数十分钟(REFERENCE §5);用户已接受非实时,但需按典型载荷设预期、选 SF/BW。
