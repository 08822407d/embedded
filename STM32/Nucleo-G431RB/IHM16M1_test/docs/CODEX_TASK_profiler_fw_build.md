# Codex 任务书 — 编译 Motor Profiler 固件(GM16020-06_profile)+ 可选授权烧录

> 给 Codex:读完即可自主执行。**Phase B(编译)完全自主、离线、不碰硬件;Phase C(烧录)属红线,须逐次 HITL 授权。**
> 项目根(Windows)= `C:\Users\cheyh\Desktop\embedded\STM32\Nucleo-G431RB\IHM16M1_test`(monorepo `embedded` 内)。
> 真相以 `docs/` 为准:`FACTS.md`、`DECISIONS.md` #003、`OPERATIONS.md` §0、`SYNC_WORKFLOW.md`、标定规程 `MOTOR_PROFILER_SOP.md`、Windows 工具链基线 `HW_SMOKE_TEST_WIN.md`。**动手前先读。**

## 背景(为什么有这个工程)
路线 #003:用 MC Workbench 生成的 **Motor Profiler 固件**去无感标定电机 GM16020-06(4 对极/12V/高KV,详见 F-24),测出 Rs/Ls/Ke 等参数。工程已由 MC Workbench(MCSDK **6.4.2**)成功生成到 `GM16020-06_profile/`。**本任务只负责把它编成固件**(Phase B),并可在授权后烧录(Phase C)。真正的标定(MotorPilot GUI)是用户操作,不在本任务。

## 目标
1. **Phase B(主,自主)**:用 STM32CubeIDE **headless** 构建 `GM16020-06_profile`(Debug 配置),产出 profiler 固件 `.elf`(并尽量出 `.hex/.bin`),留磁盘,**不烧录**。
2. **Phase C(可选,红线,HITL 授权后)**:母线断电下把该固件烧到板子;**不运行 MotorPilot、不驱动电机**(标定是用户 GUI 步骤)。
3. 处理 `.gitignore`(生成大树/构建产物忽略、保留种子)、写构建报告、更新 docs、commit+push。

## ⛔ 红线(硬件安全,最高优先)
1. **Phase B 纯离线编译,不连板、不烧录、不驱动电机。**
2. **Phase C 烧录属红线**:每次烧录前经 §-1 HITL 让用户确认「① 已授权 ② 电机母线已断电/仅 USB 供电 ③ 目标板 = NUCLEO-G431RB SN `002A00403234510E33353533`」。烧录不驱动电机(电机只有在用户于 MotorPilot 点 Start profile 且母线通电时才转),但仍须逐次确认。
3. **绝不**运行 MotorPilot / Motor Profiler 去驱动电机、**绝不**给电机母线上电、**绝不** mass erase / 改 option bytes / RDP。
4. 认准 SN 再烧,别烧错板(同机可能有 Espressif 板)。
5. 不确定就停,走 §-1 HITL/ESC,别赌。

## §-1 交互协议(HITL + ESC)
- **HITL**(须用户手动/授权):固定输出并停等——
```
==== 需要你手动操作 [HITL-n] ====
目的:... / 请你做:... / 做完回复:"HITL-n 完成"(或反馈结果)
在你回复前我不继续。
====
```
本任务必有的 HITL:**Phase C 每次烧录前的三确认**(授权+母线断+目标板 SN)。
- **ESC**(超出你职责的判断/反复失败):停止并把球交回 CC——
```
==== 需 CC 决策 [ESC-n] ====
背景/卡点:... / 我已尝试:...(附日志) / 需要 CC 定的:...
====
```

## §0 权限预检(先把会弹窗的只读动作各探一个)
- CubeIDE:`stm32cubeidec.exe --help`(或 version)确认可执行
- 工具链:CubeCLT `arm-none-eabi-gcc --version`、`arm-none-eabi-objcopy --version`
- 只读设备(Phase C 才需要,先不做):留到 HITL 时
- 文件写:项目内建 workspace 临时目录、docs
先列出你判断会弹窗的动作,逐项探通再进 §3。**烧录不放预检,走 HITL。**

## §1 已确认事实(直接用)
- **待编译工程**:`GM16020-06_profile/`。Eclipse 工程根 = `GM16020-06_profile/STM32CubeIDE/`(含 `.project`/`.cproject`);**工程名 = `GM16020-06_profile`**;构建配置 = **Debug** / Release,**用 Debug**。
- 链接脚本:`GM16020-06_profile/STM32CubeIDE/STM32G431RBTX_FLASH.ld`。
- **Motor Profiler 库**:`.cproject` 链接 `libMP/libmp-G4-IAR_ARMv7-M.a`(位于 `GM16020-06_profile/MCSDK_v6.4.2-Full/MotorControl/libMP/`)。**注**:AN Table 1 明确 STM32CubeIDE(GCC)就用这个 `-G4-IAR_ARMv7-M.a`(命名带 IAR 但 ST 提供为 CubeIDE 可链接格式),**这是正确的,别去换成别的 .a**。
- 工具:**STM32CubeIDE 1.19.0** headless = `C:\Program Files\STMicroelectronics\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeidec.exe`(已确认存在);CubeCLT 1.18.0(gcc/objcopy/`STM32_Programmer_CLI`,路径见 F-12)。
- 板/SN:NUCLEO-G431RB,SN `002A00403234510E33353533`,Device ID `0x468`(F-1/F-3/F-4);VCP 按 SN 现场扫(别写死 COM)。
- MCSDK 已升 **6.4.2**(6.4.1 已卸);电机 GM16020-06 见 F-24;三电阻/2ADC/STO PLL+Hall 配置已在 .ioc 核实无误。
- 生成时的 `PDSC version not supported` 是**无关软件包的良性噪音**,与本工程无关,**别当错误去追**。

## §2 你(codex)自己查
- **STM32CubeIDE headless 构建**的确切命令(`--launcher.suppressErrors -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data <干净临时workspace> -import "<...>/GM16020-06_profile/STM32CubeIDE" -build "GM16020-06_profile/Debug"`),以及如何指定/确认输出目录(通常 `STM32CubeIDE/Debug/GM16020-06_profile.elf`)。
- 如何从 `.elf` 生成 `.hex/.bin`(CubeIDE 后置步骤,或 `arm-none-eabi-objcopy` / `STM32_Programmer_CLI`)。
- Phase C 只读连接与烧录 `.elf` 的 `STM32_Programmer_CLI` 命令(HOTPLUG,按 SN)。

## §3 执行阶段
0. §0 权限预检。
1. **Phase B 构建**:用干净的**临时 workspace 目录**(别用工程目录本身),import `GM16020-06_profile/STM32CubeIDE` → build Debug → 得 `.elf`,objcopy/转出 `.hex/.bin`。记录命令、输出路径、size(text/data/bss)。**不烧录。**
2. **.gitignore**:为该生成工程加忽略规则(见 §5),避免把数 MB 的 HAL/中间件/`libMP/*.a`/构建产物入库;保留种子(`.ioc`/`.stwb6`/`.wbdef`/`.settings`)。
3. **Phase C(仅在用户要求且授权时)**:§-1 HITL 三确认 → 母线断电下 `STM32_Programmer_CLI` 烧 `.elf`(按 SN)→ verify。**烧完即止,不开 MotorPilot、不驱动电机**,提示用户「固件已上板,可开 MotorPilot 做标定(见 SOP §5)」。
4. 写报告 + 落盘 + commit+push。

## §4 排查矩阵(各步没达预期怎么办;依次尝试,全无效→ESC 并附日志)
| 步骤 | 预期 | 依次尝试 | 全无效→ |
|---|---|---|---|
| 定位/调用 headless | 能启动构建 | 确认 `stm32cubeidec.exe`(带 `c` 的 console 版,不是 `stm32cubeide.exe`)→ 校正 `-application ...headlessbuild` 语法 → 用**全新空 workspace**(避免陈旧 workspace 冲突) | ESC,附完整命令+stderr |
| import 工程 | 工程被识别 | `-import` 指向**含 `.project` 的 `STM32CubeIDE/` 子目录**(不是 `GM16020-06_profile/` 顶层)→ 必要时先 `-importAll` 顶层 → 确认 `.project` 未损坏 | ESC,附 headless 日志 |
| 链接 libMP | 无 undefined ref | 确认 `.cproject` 链接的 `libmp-G4-IAR_ARMv7-M.a` **存在**于 `MCSDK_v6.4.2-Full/MotorControl/libMP/` → 确认相对路径(`../` 链接资源)在你选的 workspace 下能解析;若 workspace 拷贝破坏了相对链接,**改用 in-place / 原地 import** 保持目录结构 | ESC,附链接错误 |
| HAL/CMSIS/include 缺失 | 编过 | 确认 `Drivers/` 存在、`.cproject` include 路径解析;确认 G4 FW(V1.6.2)驱动已随工程拷入(MCSDK 通常拷贝而非引用) | ESC,附缺失文件名 |
| 链接脚本找不到 | 链接成功 | 确认 `STM32CubeIDE/STM32G431RBTX_FLASH.ld` 存在;`.cproject` 里是 `${workspace_loc:/${ProjName}/STM32G431RBTX_FLASH.ld}` | ESC |
| build 完成但无 .elf | 有 .elf | 查 `STM32CubeIDE/Debug/`;核对 build 配置名(Debug);读 headless console 尾部真正错误 | ESC,附 console 尾 |
| headless 太不稳 | 拿到 .elf | **退路**:headless import 后 `Debug/` 会生成 makefile → 用 `make -C GM16020-06_profile/STM32CubeIDE/Debug` + CubeIDE/CubeCLT 的 `arm-none-eabi-gcc`(把其 bin 加 PATH)编 | ESC,说明两法都试过 |
| Phase C 烧录失败 | 写入+verify | 先只读连通(HOTPLUG 读 IDCODE)验证链路 → 查线/SN → 确认母线断 | HITL 报烧录失败+完整命令/报错 |

## §5 设计自由 + 留痕 + gitignore
- **不限定你只走 headless**:workspace 放哪、Debug/Release、出 hex 的方式、退路用 make——你可自择更优,**但满足红线**;并在报告里**完整留痕**(选了什么/试过什么/为何/结果),便于 CC 复盘。
- **.gitignore 建议**(自行定稿并说明):忽略 `GM16020-06_profile/` 下的**构建产物**(`**/Debug/`、`**/Release/`、`*.elf/*.hex/*.bin/*.map/*.list`)与**体量大的生成物/库**(HAL `Drivers/`、`Middlewares/`、`MCSDK_v6.4.2-Full/`,尤其 `libMP/*.a`);**保留种子** `*.ioc`、`*.stwb6`、`*.wbdef`、`*.settings`,以便必要时经 MC Workbench(GUI,Windows 限定)重生成。理由:该 profiler 固件是一次性诊断,重生成靠 MC Workbench GUI,不必把数 MB 生成树入库。

## §6 产出与落盘
- 新增 `docs/PROFILER_FW_BUILD.md`:headless 构建命令、输出路径/size、排查经过与留痕、`.hex` 生成法、**Phase C 烧录命令**、以及给用户的"固件上板后如何开 MotorPilot 标定"一句指引(引 `MOTOR_PROFILER_SOP.md` §5)。
- 新事实(MCSDK 6.4.2、profiler 工程已生成并编译通过、固件 size、`.cproject`/链接脚本/libMP 位置)→ 追加 `FACTS.md`(接续 F 编号,带来源+日期,**不改旧条目**);把 F-12 的 MCSDK 6.4.1 更新为 6.4.2(注明升级)。
- 更新 `STATUS.md`。
- **收尾 = commit + push**(SYNC_WORKFLOW)。

## §7 纪律 / 边界
- 红线见顶部;Phase B 只编译,Phase C 烧录逐次 HITL,**绝不驱动电机/不跑 MotorPilot**。
- 改动限本项目目录;端口/SN 现场探测别写死;Windows 路径含空格加引号;别乱设 `core.autocrlf`。
- 反复失败宁可 ESC 交回 CC,也不硬试、不越红线。
