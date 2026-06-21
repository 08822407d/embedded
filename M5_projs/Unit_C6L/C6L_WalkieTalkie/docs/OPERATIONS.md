# OPERATIONS — 编译 / 烧录 / 串口 / 测试手册

> 命令以 `platformio.ini` 为准(env = `m5stack-unitc6l`)。**烧录是操作红线**:未经用户明确指示别执行(见 CLAUDE.md)。

## 设备探测(⚠️ 先探后用 · 必读)
**主机可能同时连了多块 M5 板、也可能一块没连;串口名不固定(可能在 Windows)。任何烧录 / 读串口前先探测,别写死端口名、别假定连的就是 C6(FACTS F‑8~F‑11)。**

**第 1 步 · 枚举端口(跨平台、机器可读)**
```
pio device list --json-output
```
按 `hwid` 里的 `VID:PID` 分类,**不看端口名**:
- `303A:*` → Espressif **原生 USB** 芯片(C6/S3/C3/H2 同此 VID;Unit-C6L 属此类,F‑8)。
- `10C4:*`(CP210x)/ `1A86:*`(CH34x)/ `0403:*`(FTDI)→ 走**外置 USB-UART 桥**的板子 → Unit-C6L 不是这种(F‑4)→ 基本可排除。

按候选(VID `303A`)数量决定:
- **0 个** → 无可用板(或板未进入 USB-CDC)→ **不烧录 / 不盲连**,告知用户。
- **1 个** → 多半是一块 Unit-C6L;若主机可能还接了别的 Espressif 原生 USB 板(如 S3 系列),型号关键时做第 2 步。
- **2 个** → 与"两块 C6"吻合,通信测试用这两个口。

**第 2 步 · 确认芯片确为 ESP32-C6**(仅 VID:PID 不足以和 S3/C3 区分,必要时做)
```
# esptool 随 espressif 平台安装;读型号 / MAC 为只读,但会把板子复位进下载模式
esptool.py --port <PORT> chip_id     # 看 "Detecting chip type... ESP32-C6"
esptool.py --port <PORT> read_mac    # 取唯一 MAC,用来区分两块同型号 C6
```
(确切调用按本机 PlatformIO 版本核对,例如 `pio pkg exec -- esptool.py ...`。)

**区分两块同型号 C6**:`pio device list` 的 `hwid` 里 **`SER=` 直接就是芯片 MAC**(2026-06-21 实测有值),据此即可区分,**通常无需 esptool**;也可用 `/dev/serial/by-id/`(按该 SER)或 `by-path/`(按物理 USB 口,线不挪就稳定)。esptool `read_mac` 仅作交叉验证。

> 兜底:`pio run -t upload` 调用的 esptool 本就会自动检测芯片,烧错到非 C6 会报错 —— 但那已复位了板子,**别拿它当探测手段**,探测用上面的只读步骤。

## 编译
```
pio run                       # 默认 env = m5stack-unitc6l
pio run -e m5stack-unitc6l    # 显式指定
```
- 首次构建会从 pioarduino `develop` zip 拉平台,**耗时较长**;若报找不到 `riscv32-esp-elf-g++`,由 `scripts/fix_toolchain_path.py` 自动前置 PATH 处理(FACTS F‑5)。若仍失败,检查 `toolchain-riscv32-esp` 包是否已安装。
- **⛔ 已知坑(2026-06-21,FACTS F‑12)**:平台要求 **pio Core ≥ 6.1.19**;6.1.18 会报 `IncompatiblePlatform`、平台装完即卸、编译失败。`pio upgrade` 在本机被 PEP 668 挡下;就地升级用 `python3 -m pip install --user -U platformio --break-system-packages`,再 `pio run`。

## 烧录(⛔ 红线:需用户在场明确指示)
```
pio run -t upload                         # 单板;upload_speed=1500000
pio run -t upload --upload-port <PORT>    # 多板:用『设备探测』拿到的端口显式指定,别靠自动选口
```
- 串口为原生 USB-CDC(FACTS F‑4);**端口名不固定,先探测,别写死**(F‑10)。

## 串口监视
```
pio device monitor -b 115200              # 单板可自动选口
pio device monitor -p <PORT> -b 115200    # 多板:显式指定探测到的端口
```
- **抓一次性启动日志**(如 `setup()` 里只打印一次的串):C6 走 USB-Serial-JTAG,端口稳定、`python3` 自带 pyserial(3.5)。可开口后**脉冲 RTS 复位**再读几秒:`s=serial.Serial('/dev/ttyACMx',115200); s.setRTS(True); time.sleep(0.2); s.setRTS(False)` 然后 `s.read(...)`(2026-06-21 实测可复位并抓到完整 boot 日志,F‑13)。`CORE_DEBUG_LEVEL=5` 时启动会打印整板自检,信息量很大。
- 启动应见骨架日志 `Unit C6L skeleton boot`(改代码前的基线)。

## 测试
- 暂无;`test/` 仅 README 占位。后续单测 / 集成测试方案待定。

## 备注
- 本地约定:命令尽量分两步跑、不用管道 `A | grep`(避免权限弹窗)。
- 耗 token 且可程序化的批量工作优先写本地脚本处理,模型只读结论。
