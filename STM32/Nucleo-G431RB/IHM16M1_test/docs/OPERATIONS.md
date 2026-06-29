# OPERATIONS — 编译 / 烧录 / 读串口 / 调试 / 设备探测

> 怎么把代码跑到板子上、怎么观察。**工具链未定前,本文件只给"设备探测"通用流程 + 候选命令骨架;定了工具链再补实命令。**
> ⚠️ 烧录 / 上电机电源是**操作红线**(见 [`CLAUDE.md`](../CLAUDE.md)),未经本会话明确授权别做。

## 0. 设备探测(动手前必做,别写死端口/探针)
主机可能接多块 ST-Link / 板子,或一块没连;**USB 口 / 端口号每次插拔会变**。**烧录或读串口前必现场扫描**:确认目标在线且确为 G431RB,并按固定 SN 解析当前端口。
> 原则:**按固定标识匹配**(ST-Link SN、USB VID:PID —— 见 [`FACTS.md`](FACTS.md) F-3/F-4),**别看端口名、别记上次的 `ttyACMx` 数字、别靠人眼认型号**。

- **第 1 步 · 确认目标板在线且为 G431**:`STM32_Programmer_CLI -l` —— 在线会直接列出 `Board: NUCLEO-G431RB` + `ST-Link SN`(应 = FACTS F-3 的 `002A...3533`)+ `Device ID 0x468`(FACTS F-4)。**这一步即可确认身份,无需连 SWD**。
- **第 2 步 · 解析 VCP 串口(现场,别写死)**:`ls -l /dev/serial/by-id/` —— 找含本板 ST-Link SN 的 `...STLINK-V3_<SN>-if02` 链接,它指向**当前**的 `/dev/ttyACMx`。⚠️ 裸 `ttyACMx` 数字每次可能变,**永远按 by-id(SN)解析**。
- ⚠️ **同机干扰**:本机常并存其它板(如 Espressif ESP32,VID `303a`,会占一个 `/dev/ttyACM`)。务必按 **ST-Link SN / VID:PID `0483:374e`** 锁定 G431,**别拿错口、别碰非目标板**。

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
