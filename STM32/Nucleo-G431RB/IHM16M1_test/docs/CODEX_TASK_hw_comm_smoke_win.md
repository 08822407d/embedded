# Codex 任务书(Windows 版)— G431RB + IHM16M1 硬件/通信/工具链冒烟测试(只读 · 不烧录 · 不转电机)

> 给 Codex:读完即可自主执行,**但本任务含"人在环(HITL)"交互点,遇到须我手动操作处必须停下等我**(见 §-1)。
> 本文件是 [`CODEX_TASK_hw_comm_smoke.md`](CODEX_TASK_hw_comm_smoke.md)(Linux 版)的 **Windows 适配版**,**新建、不改原版**。Linux 版已在 Linux 机全 PASS(结果见 [`HW_SMOKE_TEST.md`](HW_SMOKE_TEST.md))。
> 项目根(**本机 Windows**)= `C:\Users\cheyh\Desktop\embedded\STM32\Nucleo-G431RB\IHM16M1_test`(在 monorepo `embedded` 内)。
> 真相以仓库 `docs/` 为准:硬件事实 `docs/FACTS.md`、设备探测 `docs/OPERATIONS.md` §0、路线 `docs/DECISIONS.md` #003、双机同步 `docs/SYNC_WORKFLOW.md` / #004。**动手前先读它们。**

## 为什么有 Windows 版(背景)
- 本项目**双机协作**(DECISIONS #004 / SYNC_WORKFLOW):**Linux** 管仓库/编译/烧录;**Windows** 跑 **X-CUBE-MCSDK**(Motor Profiler 标定 + MC Workbench 生成 FOC),因为 **MCSDK 只有 Windows 版**。
- 在 Windows 机上正式做 MCSDK 之前,先建立 **Windows 侧基线**:确认①这台 Windows 能和**同一块板**只读通信、②Windows 侧有可用工具链能离线编译、③盘点 Windows 已装的 ST 工具(含 MCSDK 是否就位)。
- **硬件完全没变**(用户确认):同一套 NUCLEO-G431RB + X-NUCLEO-IHM16M1 + 同一台电机,未做任何改动。故**板子身份事实可直接复用**(SN / Device ID,见 §1),**只有主机工具链与端口是 Windows 特有、需现场发现**。

## ⛔ 红线(最高优先,违反即停)
当前**电机可能带电、板子可能无人值守**。本任务**全程只读 + 离线编译**:
- **绝对禁止**:烧录/下载(任何 write/download/program)、mass erase、擦写 flash/option bytes、复位后运行用户程序(run/start/-go)、任何可能输出 PWM / 驱动电机的动作。
- **绝对禁止**:运行 **Motor Profiler**、用 **MC Workbench 生成/下载工程**、给电机扩展板上电机电源 —— 这些是后续**用户主导**的独立步骤,**不在本任务内**。
- **只允许**:① ST-Link **只读**连接(读 IDCODE/flash size/option bytes,不改任何字节);② 在磁盘**生成+编译**最小工程(产物留磁盘,**不下载到板**);③ 只读枚举串口/USB;④ **只读盘点**已装 ST 工具(查安装路径/版本,不安装、不启动会驱动硬件的功能)。
- 连 SWD 用**不运行用户代码**的方式(hot-plug 或 connect-under-reset 均可,但**不要 `-rst` 后启动、不要 start application**)。
- 任何一步若**必须写/烧/转电机/装软件**才能继续 → **停,走 §-1 交互点问我或写进报告等我**。

## §-1 交互协议(HITL · 本 Windows 版新增,务必遵守)
Linux 版靠"§0 一次性批准所有权限弹窗"就能全自主跑完;**Windows 版不同**:有些事只能我(人)动手——插拔 USB、装软件、在设备管理器认端口、点 GUI 授权等。**遇到这类步骤,你不能替我做、也不许假设我已做好,必须停下等我**。

**触发停等的情形(非穷举):**
- 需要我**物理插拔/换口**开发板 USB(比如板子现在插在 Linux 机上,要挪到这台 Windows)。
- 需要我**安装/更新**某工具(CubeProgrammer / CubeCLT / CubeMX / MCSDK),或首次启动某 GUI 要**点许可/登录 st.com**。
- 自动枚举认不出 COM 口,需要我**打开设备管理器**按 SN 帮你确认。
- 任何会弹 **Windows UAC / 安全提示 / 驱动安装** 的动作。
- 你不确定某步是否越红线。

**停等时,固定输出这样一块(编号递增 HITL-1、HITL-2……),然后停止,等我回复:**
```
==== 需要你手动操作 [HITL-n] ====
目的:<为什么需要这步>
请你做:
  1) <具体动作,写清在哪点/插哪个口/装什么版本>
  2) ...
做完请回复:"HITL-n 完成"(或告诉我实际结果/报错/截图内容)
在你回复前我会停在这里,不继续、不猜测。
====
```
**纪律:**① 一次尽量把**当前阶段需要我做的手动动作合并**成一个 HITL,别把我当自动机反复零碎打断(呼应仓库"少打断"原则);② 我回复后,按我反馈的**实际情况**继续,别按理想假设走;③ 若我反馈"没装/做不了/出错",**记录并转下一可行项或停下问**,不硬试、不越红线。

## §0 权限预检(可自动、只读的先批量探通)
把**能自动、纯只读**的探针各跑一个最小实例,让我一次性批准(这批不算 HITL,是常规权限弹窗):
- 设备:`STM32_Programmer_CLI.exe -l`(先解析可执行路径,见 §2)
- 串口枚举:PowerShell 列 COM 口并尝试按 SN 匹配(见 §2)
- 工具链版本:若装了 CubeCLT/CubeIDE,`arm-none-eabi-gcc --version`、`make --version`
- 确认各 ST 工具可执行文件**存在**(不启动 GUI)
先**列出你判断会弹窗的动作清单**,逐项探通;**凡属"须我物理动手/装软件/点 GUI"的,不要塞进这里,走 §-1 的 HITL。**

## §1 已验证事实(硬件未变,直接复用;详见 docs/FACTS.md)
- 板 = NUCLEO-G431RB,MCU STM32G431RB,**Device ID `0x468`**。
- 调试器 = **STLINK-V3**,**本板 SN = `002A00403234510E33353533`** —— 用它确认连的就是本板(**同一块板,SN 不变**)。
- 硬件为 ST 官方套件 **P-NUCLEO-IHM03**(IHM16M1 + G431RB)组合,**无改动**。
- ⚠️ **注意与 Linux 版的区别**:下列在 Linux 是"已知事实",在 **Windows 上一律未知、须你现场发现并记入 FACTS(新 F 编号)**:
  - **ST 工具安装路径/版本**(CubeProgrammer / CubeCLT / CubeMX / **MCSDK**)——Windows 多在 `C:\Program Files\STMicroelectronics\...`,但**别写死,实测**。
  - **构建工具链**:Windows 上是否装了 CubeCLT(带 `arm-none-eabi-gcc` + `make`)?若没有,能否用 CubeIDE 内置 GCC 或其它方式离线编译?——**这是本任务要查清的关键未知**。
  - **端口**:Windows 是 `COM*`,不是 `/dev/ttyACM*`;按**同一 SN**在系统里认(设备管理器/PowerShell)。**每次现场扫,别写死 COM 号**。
  - 可能仍并存 **Espressif 板**(VID `303a`)占一个 COM,**别拿错**。

## §2 你(codex)自己查(别让 CC 代查)
- **定位 Windows 版 `STM32_Programmer_CLI.exe`**(CubeProgrammer 或 CubeCLT 内)及其**只读**读芯片信息命令(读 IDCODE/flash size/option bytes,`-c port=SWD mode=HOTPLUG sn=<SN>` + `-r32/-r16/-ob displ`,**不写不擦不 run**)。
- **PowerShell 按 SN 枚举 COM 口**的方法(Win32_PnPEntity / Get-PnpDevice / `Win32_SerialPort`,在设备实例 ID 里匹配 SN `002A...3533`;`STM32_Programmer_CLI -l` 的输出通常也直接给出对应 COM)。给出"by-id 等效"的可复现命令。
- **Windows 构建方案**:优先用与 Linux 版一致的 **CubeMX headless 生成 Makefile 工程 + `make` + `arm-none-eabi-gcc`**;若 Windows 未装 CubeCLT/make,则查明本机可行的离线编译途径(CubeIDE 内置工具链 / MinGW make 等),并把结论记入 docs。
- **Windows 版 CubeMX 命令行/headless** 为 NUCLEO-G431RB 生成最小工程(默认时钟 + 板载 LED **LD2=PA5** GPIO 输出,可选 USART2 作 VCP),输出 Makefile 工程;注意 Windows 路径含空格要正确加引号。

## §3 执行阶段(按序;遇手动步走 §-1 HITL)
0. **§0 权限预检**(批量只读探针)。
1. **HITL-1:确认板子插在这台 Windows 上**。板子先前可能插在 Linux 机;请我把 USB 线接到本 Windows 机、并告知就绪(顺带首次插入可能触发驱动安装)。**收到我"完成"再继续。**
2. **工具盘点(只读)**:发现并记录本机已装 ST 工具(CubeProgrammer/CubeCLT/CubeMX/**MCSDK**)的**路径 + 版本**。
   - 若做后续步骤**必需**的工具缺失(如无 CubeProgrammer 则无法只读 SWD、无任何 GCC/make 则无法编译)→ **发 HITL** 请我安装或告诉我缺什么,等我反馈;**你不自行安装**。
   - **MCSDK 只盘点是否存在与版本,绝不启动 Motor Profiler / 不生成 FOC 工程。**
3. **设备探测**:`STM32_Programmer_CLI.exe -l` → 确认 Board=NUCLEO-G431RB、SN=§1、Device ID=0x468;按 SN 解析当前 **COM 口**(只记录,不打开收发)。若认不出 COM → **HITL** 请我在设备管理器按 SN 帮认。
4. **ST-Link↔MCU 通信验证(只读)**:连 SWD 读 IDCODE/flash size/option bytes。读到即代表调试链路 OK。**不擦不写不 run。**
5. **工具链编译验证**:用 §2 查明的 Windows 构建方案生成最小 G431 工程(LED GPIO + 可选 USART2)→ 编译 → 产出 `.elf`/`.bin`/`.hex` **留磁盘**。**不烧录。**
   - CubeMX 首次启动若要点许可/更新/登录 → **HITL** 请我处理。
6. 写报告(§5)。

## §4 测试目标(测什么;怎么测你设计)
1. **交互层**:所有须人手动的步骤都经 §-1 HITL 停等,我确认后才推进(过程留痕在报告里)。
2. **探针层**:从 Windows 侧,ST-Link 在线、SN=§1、Device ID=0x468。
3. **调试通信层**:Windows 侧 SWD 只读连接成功读到 IDCODE/flash/option bytes(证明这台 Windows 能和同一块板通信)。
4. **工具链层**:最小 G431 工程用 **Windows 本机工具链编译通过**,产出 `.elf`。
5. **串口枚举**:Windows 侧按 SN 认到本板 VCP 的 **COM 口**(只确认,不收发),并与 Espressif 区分。
6. **环境盘点**:记录 Windows 侧 ST 工具路径/版本(**含 MCSDK 是否就位**),为下一步 MCSDK 工作铺垫。
7. 任一项失败/缺件 → 记录完整现象(命令+输出+报错),经 HITL 问我或停下,不硬试、不越红线。

## §5 产出
- 新增 **`docs/HW_SMOKE_TEST_WIN.md`**:测试 procedure + 每项实测结果(命令+输出摘要)+ **HITL 交互记录**(每个 HITL 的请求与我的反馈)。**不覆盖 Linux 版 `HW_SMOKE_TEST.md`。**
- 新发现的 Windows 事实(工具路径/版本、构建方案、COM 认法)→ 追加到 `docs/FACTS.md`,**用接续现有最大号的新 F 编号**,带来源+日期(今天),**不改动已有条目**。
- 最小工程放 `hw_smoke_win/`(与 Linux 的 `hw_smoke/` 分开,避免两 OS 生成物互踩),`.gitignore` 掉构建产物;CubeMX 脚本与工具链路径记入 docs(可复现)。
- 更新 `docs/STATUS.md`("现在到哪/下一步"反映 Windows 基线结果)。
- **收尾 = commit + push**(SYNC_WORKFLOW 纪律:本地 commit 不算保存,须 push 到 origin 另一台机才看得到)。

## §6 纪律/边界
- 红线见顶部:**只读 + 离线编译,不烧/不擦/不 run/不转电机/不装软件/不跑 MCSDK。**
- **HITL 铁律**:须人动手处必停等我,不替我做、不假设已做(§-1)。
- 改动限本项目目录,别碰 monorepo 其它项目。
- 端口/COM/工具路径**现场探测**,别写死;Windows 路径含空格记得加引号。
- 跨 OS:别乱设 `core.autocrlf` 致全文件翻动(`.gitattributes` 已统一 LF)。
- 不确定就停下问,别为"跑通"越界。
