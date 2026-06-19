# OPERATIONS — 编译 / 烧录 / 调试运行手册

> 怎么把代码弄进设备、怎么观察它。发现新坑或新命令就往这里补。
> 工具链:PlatformIO(见根 `platformio.ini`)。
>
> ⛔ **安全红线**:开发板常无人值守 —— 未经用户明确指示**不得烧录、不得驱动电机**(机体可能翻到不可恢复姿态,无人能扶正)。详见根 `CLAUDE.md`「操作红线」。

## 环境
- 构建系统:**PlatformIO Core 6.1.18**(`/home/cheyh/.local/bin/pio`,亦可用 `platformio`)
- 目标板:env `m5stack-atom`,platform `espressif32`,framework `arduino`
- 串口波特率:监视 115200 / 上传 1500000
- 依赖库:`Wire`、`m5stack/M5Atom`、`fastled/FastLED`、`M5Unit-Roller`(github)

## 常用命令(在本项目目录下执行)
- 编译:`pio run`
- 烧录:`pio run -t upload`(指定串口:`pio run -t upload --upload-port /dev/ttyUSB0`)
- 读串口:`pio device monitor`(波特率取自 ini;或显式 `pio device monitor -b 115200`)
- 编译 + 烧录 + 监视:`pio run -t upload -t monitor`
- 清理:`pio run -t clean`
- 列出串口设备:`pio device list`

## 设备交互 / 调试工具
- （待建:若做串口命令桥、日志抓取脚本等,放 `tools/` 并在此登记用法)

## 已知坑
- （待填)
