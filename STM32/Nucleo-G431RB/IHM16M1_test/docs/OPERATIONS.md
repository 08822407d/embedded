# OPERATIONS — 编译 / 烧录 / 读串口 / 调试 / 设备探测

> 怎么把代码跑到板子上、怎么观察。**工具链未定前,本文件只给"设备探测"通用流程 + 候选命令骨架;定了工具链再补实命令。**
> ⚠️ 烧录 / 上电机电源是**操作红线**(见 [`CLAUDE.md`](../CLAUDE.md)),未经本会话明确授权别做。

## 0. 设备探测(动手前必做,别写死端口/探针)
主机可能接多块 ST-Link / 板子,或一块没连。**烧录或读串口前先确认目标在线且确为 G431RB。**
> 原则:**按机器可读字段分类**(USB VID:PID、ST-Link 序列号),**别看端口名、别靠人眼认型号** —— 端口名(`ttyACM0` 等)不固定,型号靠猜易错。

- **找 ST-Link(USB)**:`lsusb` 看有无 STMicroelectronics ST-LINK(VID `0483`,常见 PID `374b`=ST-LINK/V2-1)。
- **探针信息**:若装了 STM32CubeProgrammer → `STM32_Programmer_CLI -l`;若用 OpenOCD → 起对应 cfg;若用 stlink 工具 → `st-info --probe`(看 chipid / 是否 G431)。
- **虚拟串口(板载 VCP)**:ST-LINK/V2-1 提供 USB-CDC,通常枚举为 `/dev/ttyACM*`。**名字不固定**:用 `ls -l /dev/ttyACM*` 列出,多块时按 `udevadm info /dev/ttyACMx` 的序列号区分,别假定 ttyACM0 就是目标。
- **多板 / 误连**:若出现非 G431 的板,**别碰**;只对确认为目标的设备操作。

## 1. 编译(待定)
_选定工具链后填,候选:_
- CubeIDE:工程内构建 / `cmake` + `make`
- PlatformIO:`pio run`(env 待定)
- Makefile(CubeMX 生成):`make`

## 2. 烧录(待定 · 红线)
_候选命令(确认授权 + 电机断电 / 空载后再用):_
- `STM32_Programmer_CLI -c port=SWD -w firmware.elf -rst`
- OpenOCD:`openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program firmware.elf verify reset exit"`
- PlatformIO:`pio run -t upload`

## 3. 读串口 / 调试
- 串口监视:`pio device monitor` 或 `picocom -b <baud> /dev/ttyACMx`(波特率待定,先探测端口)
- SWD 调试:OpenOCD + gdb,或 CubeIDE 调试器
- 本地耗 token 且可程序化的活(批量日志解析等)优先写脚本到 `scripts/` / `tools/`,模型只读结论。

## 4. 安全开机顺序(电机,待细化)
1. 确认接线、母线电压在范围、电机机械上安全(空载或固定)。
2. 先**不上电机电源**烧录 + 跑逻辑自检。
3. 占空从 0 / 小值起,旁边有人值守、可急停,再逐步放开。

## 5. 本地操作约定(减少权限弹窗)
- **git 用裸命令、分两步跑**:在项目目录直接 `git add .` → `git commit -F <消息文件>`;**别用 `git -C <路径>`、别用 `&&` 串多条、别用 heredoc** —— 这些写法会让命令偏离 `.claude/settings.json` 里 `allow` 的前缀模式(`git add *` / `git commit *`)而触发授权弹窗。(`git -C *` 形式虽已补进 `allow` 作双保险,但通配匹配路径不够稳,裸命令最可靠。)
- 命令尽量**分两步跑、不用管道 `A | grep`**(避免权限弹窗;呼应全局约定)。
- 耗 token 且可程序化的批量活,优先写脚本到 `scripts/` / `tools/`,模型只读结论。
