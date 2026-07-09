# MOTOR_PROFILER_SOP — GM16020-06 用 ST Motor Profiler 标定操作规程(用户驱动 + CC 旁站)

> 目的:用 MC Workbench 的 **Motor Profiler** 自动测出 GM16020-06 的电机参数(Rs / Ls / Ke / 最高转速 / 额定电流等),为后续生成霍尔 FOC 工程提供标定数据。
> 事实依据见 [`FACTS.md`](FACTS.md)(F-24 电机、F-25 电源、F-16~F-18 三电阻、F-20 霍尔脚)、路线 [`DECISIONS.md`](DECISIONS.md) #003。

## 0. 本步的性质(和之前几步不同,先读)
- **这一步会:① 烧录 ST 的 Motor Profiler 专用固件到 G431;② 真正驱动电机高速旋转。** 属项目**红线**动作,**每次执行前须用户在本会话明确授权**,且必须有人值守。
- **Motor Profiler 是 MC Workbench 的图形界面工具,靠人点按钮 + 实时盯电机**。**codex 无法驱动 GUI / 无法盯电机**,故本步分工:
  - **用户驱动**:在 MC Workbench GUI 里操作 + 观察电机与读数;
  - **CC(本会话)旁站**:解读读数、判断正常/异常、指导下一步、决定是否中止;
  - codex 仅可能做事后边角(读回生成的参数文件核对),不主导。
- 若 Motor Profiler 标不动本电机(高 KV/低电流/低电感,已知坑),转 **§7 Plan B 手工测参**(需仪表,可能要等到明天)。

## 1. ⛔ 本次电源的安全短板 + 补强(必读)
- **现有电源:固定 12.6V / 最大 1.9A,无可调限流**(用户 2026-07-08 确认)。电压对电机(额定 12V)和 IHM16M1(STSPIN830 4.5–45V)都 OK。
- **短板**:没有台面级可调限流 → **少了一层过流保护**。正常标定电流很小(电机空载才 0.16A),但一旦发生堵转 / FOC 失控 / 桥臂异常,电源可能把电流顶到其 ~1.9A 上限灌进微型绕组,持续下去可能过热损绕组。
- **保护分层(现状)**:
  1. **主保护 = Motor Profiler 里设的"最大电流"上限**(由固件/FOC 电流环强制,是标定期间的主限流)——**务必设低**(见 §4)。
  2. STSPIN830 自带过流保护 + MCSDK fault 阈值(次级)。
  3. 人工值守 + 立即断电(兜底)。
- **强烈建议加一层硬件兜底:在 12.6V 母线正极串一个 ~1–1.5A 快熔保险丝(fast-blow)。** 正常标定(峰值我们限到 ≤0.5–1A)不会熔,真出故障(堵转/短路把电流拉过 1.5A)会**立即熔断切电**,顶替缺失的可调限流。成本极低,强烈建议装。若无保险丝,则**更要**把 Motor Profiler 电流上限设到很低、并缩短每次运行时间、手指放在电源开关上。

## 2. 开工前物理检查闸(逐条确认,缺一不可)
标定会把电机拉到**几千甚至上万 RPM**:
1. **拆掉轴上的打印指针!** 高速必飞出,伤眼——它是留给后面**低速看角度**用的,标定阶段绝不能装。确认轴上空载、无任何附着物。
2. **电机机身牢固固定**(夹具/支架/粘固),防止高速跳动、位移;三相线与霍尔线理顺、不会被卷入。
3. **接线核对**:三相 U/V/W → IHM16M1 左侧绿色 3 位端子;霍尔 5 线 → J1(VCC/GND/H1→A+H1/H2→B+H2/H3→C+H3)。Motor Profiler 本身不用霍尔(它是无感标定),但先接好不碍事。
4. **电源接入**:12.6V 接 IHM16M1 母线输入,**正极串 §1 建议的保险丝**;先不通电。
5. **急停就位**:电源开关/插头触手可及,能一秒断电;**你全程在旁**。
6. **环境**:戴护目镜更稳妥;周围无人凑近旋转端。

## 3. 目标板与工具确认
- 控制板 = NUCLEO-G431RB(SN `002A00403234510E33353533`);功率板 = X-NUCLEO-IHM16M1(默认 FOC 三电阻,F-18);组合 = P-NUCLEO-IHM03 pack(F-7)。
- MC Workbench = `C:\Program Files (x86)\STMicroelectronics\MC_SDK_6.4.1\Utilities\PC_Software\STMCWB\STMCWB.exe`(F-12)。
- 板子用 USB 接本 Windows;Motor Profiler 会通过 ST-Link 烧它的标定固件(**会覆盖当前 hall_probe 固件,正常**)。

## 4. 标定输入参数(已知的先备好)
| 参数 | 值 | 说明 |
|---|---|---|
| 极对数 Pole pairs | **4** | F-23 实测,必填且必须对(填错则 Ke/转速整体偏倍数) |
| 母线电压 | **12.6V** | 实测供电 |
| 最大电流 Imax(标定) | **起步 ≤0.5A**(保守) | 电机空载 0.16A;先设很低,不够再逐步加。**这是主限流,务必低起步** |
| 最高转速上限 | 先设一个**远低于 18200 的保守值**(如 3000–5000 RPM)起步 | 先低速确认能转、参数合理,再看要不要放高;别一上来冲满速 |

> 注:Motor Profiler 通常要你填极对数、Imax、额定电压等再点开始;**具体字段/按钮以 MC Workbench 6.4.1 当场向导为准**,我会实时看着你填、逐项确认。

## 4.5 前置:必须先"生成→编译→烧录" Motor Profiler 固件(MCSDK 6.2+,无预编译二进制)
> 依据:本机官方 **AN Motor Profiler**(`C:\Program Files (x86)\STMicroelectronics\MC_SDK_6.4.1\Documentation\html\md_docs_2_a_n___motor___profiler.html`)。**6.2 起不再附带预编译 profiler 固件**——必须用 MC Workbench 生成带 Motor Profiler 特性的 FOC 工程 → 编译 → 烧板。板子跑上该固件后,MotorPilot profiler 才连得上(否则就是我们看到的 Board Disconnected / Start profile 灰 / 白框点不动)。
> 好消息:**X-NUCLEO-IHM16M1 在官方支持列表内,且其 .json 含 `resistorOffset` 参数 → 允许直接标定**(不含该参数会"怪声且不标定")。

**Phase A(MC Workbench GUI,用户+CC):生成 Profiler 工程**
1. MC Workbench → **New Project**(不是 Tools→Motor Profiler)。
2. 电机选 **低压 / 未知电机(low voltage motor)**;控制板 **NUCLEO-G431RB** + 功率板 **X-NUCLEO-IHM16M1**(或 P-NUCLEO-IHM03 pack)。
3. Current Sensing → **Three shunt resistors**(单电阻不支持 Profiler,与 F-17 一致)。
4. ADC → **3 shunts / 2 ADC**(⚠ 必须 2 个 ADC;1 ADC 会让进度条**卡在 7%** —— AN 已知问题;G431RB 支持 ADC1/ADC2)。
5. Application configuration → 勾选 **Motor Profiler** 特性。
6. 传感器配置(⚠ 实测有坑,已验证正确配法):**Motor Profiler 不兼容"Hall 作主传感器"**,报错 `Motor Profiler is not available for Hall Sensor. Only for STO PLL or High Sensitivity Observer`。正确配法 =
   - Motor 标签:**打开 Hall effect**(否则 Auxiliary 里选不了 Hall);
   - Speed Sensing Mode Selection:**Main = Observer + PLL (Sensorless / STO PLL)**(不是 Hall!开 Hall effect 后 UI 会默认把 Main 设成 Hall,需**手动改回 Observer+PLL**);
   - Auxiliary Sensor = **Hall**(此时才腾得出来);
   - 然后 Application Configuration 里 **Motor Profiler 才能勾上**。这样既无感标电机参数,又保留 Hall 辅助以便顺带标霍尔安放角。
7. (低电感电机)可适当**提高 PWM 频率**以降低相电流纹波(AN 提示;本电机低电感,可能需要)。
8. Save + Generate,工具链选 **STM32CubeIDE**(MCSDK 为其提供 Motor Profiler 预编译库 `libmp-G4-*`,最稳;codex 可 headless 构建 + `STM32_Programmer_CLI` 烧录,与 IDE 无关)。
   - ⚠ **CubeMX 版本**:MC Workbench 用它自己集成的 CubeMX(实测只提供 **6.14.1**),与我们独立安装的 6.18.0 无关。6.14.1 是**稳定版**(之前的生成 bug 是 6.17.0-RC5 特有),支持 G431、是 MCSDK 6.4.1 配套验证版本,**生成正常、直接用**。

**Phase B(可交 codex/脚本):编译**
- 用本机 Windows 工具链(CubeCLT GCC + CubeIDE make,复用 `hw_smoke_win`/`build_win.ps1` 思路)编译生成的工程 → 产出 profiler 固件 `.elf/.hex/.bin`,留磁盘。

**Phase C(⛔ 红线,逐次授权):烧录 profiler 固件**
- **母线断电**下,用 STM32CubeProgrammer/CubeIDE 把 profiler 固件烧到板子(SN `002A00403234510E33353533`)。**烧录本身不驱动电机,安全**;真正转电机在 Phase D。烧录会覆盖 hall_probe,正常。

**Phase D(GUI,用户+CC):MotorPilot 标定** → 见 §5(此时板子已跑 profiler FW,MotorPilot 能连上、显示 Control/Power Board、字段可填、Start profile 变亮)。

## 5. Motor Profiler 操作流程(Phase D;板子已跑 profiler FW 后)
> 我对 6.4.1 具体 UI 不逐像素打包票;**按向导走、每步把屏幕上看到的读给我**,我判断对错与下一步。大致流程:
1. **通电前**先启动 MC Workbench → 进入 **Motor Profiler**(起始页有入口)。
2. 选/确认硬件:控制板 NUCLEO-G431RB + 功率板 X-NUCLEO-IHM16M1(或直接 IHM03 pack)。
3. 填 §4 参数(极对数 4、Imax 0.5A 起、母线 12.6V、转速上限保守)。
4. **确认 §2 物理闸全绿 + 你已授权** → 给母线通电(12.6V)。
5. 点 **Start / Profile**:它会烧标定固件、无感拉起电机旋转、测 Rs/Ls/Ke 与机械参数。**全程盯电机与电流**。
6. 读结果:Rs、Ls、Ke(或 Kv/BEMF)、最高转速、额定电流、机械时间常数等 → 报给我核合理性。
7. 结果合理 → 保存 motor profile(供后续 MC Workbench 生成 FOC 工程用)。

## 6. 立即中止的红旗(见到任一条,马上断电)
- 异响、尖啸、剧烈抖动、电机跳动/移位;
- 电流**顶到设定上限并卡住**、或电源被拉到 1.9A 上限、电压明显下垂;
- 电机**发烫、焦味、冒烟**;
- 转速**失控上冲**或反复启停抽搐;
- MC Workbench 报 **fault / over-current / start-up failure** 且反复重试;
- 保险丝熔断(说明发生过流,查因再说,别急着换更大的保险丝硬冲)。
**中止后**:断电、记录当时屏幕读数/报错/现象,交给我判断(可能是 Imax 太低拉不起、或低电感标定坑、或接线/固定问题)。

## 6.5 AN 已知问题对照(我们这颗电机很可能撞上,预判)
来自官方 AN Motor Profiler「Known Problems」——**对高 KV/低电感微型电机尤其相关**:
- **进度条卡 7%**:current sensing 用了 1 个 ADC → 必须 **2 ADC**(见 §4.5 步 4)。
- **进度条卡 28%**:FW 找不到可行的启动速度爬坡 → **调 Max current / Max speed**;仍不行则转 **Open Loop** 手动调启动爬坡再回填。**本电机最可能卡这里**(低电感、高 KV 启动难)。
- **过压/欠压错误(标定前)**:把 UnderVoltage/OverVoltage threshold 设合理(母线 12.6V → 欠压阈值设远低、过压设远高,如 ~6V / ~20V);也可能是 1 ADC 造成。
- **怪声且不标定**:power board .json 缺 `resistorOffset` → IHM16M1 含此参数,应无此问题;若真遇到,AN 提供 "Offset Detection" 用已知 Rs 电机反算(我们暂无已知 Rs 电机,故走不了,只能靠 IHM16M1 自带值)。
- **Start profile 按钮变灰/复位后不可用**:disconnect → GUI/load GUI 重载 profiler.qml → 重新 connect。
- **低电感高纹波**:可在 WB 生成时**提高 PWM 频率**改善控制(§4.5 步 7)。

## 7. Plan B —— 手工测参(Motor Profiler 标不动时;需仪表,可能等明天)
若 §5 反复失败,改手工测下列参数,直接填进 MC Workbench(跳过自动标定):
- **Rs(相电阻)**:万用表量任意两相间电阻,**÷2** = 每相(星形)。微型电机可能几欧。
- **Ls(相电感)**:LCR 表量两相间电感,**÷2** = 每相。
- **Ke(反电动势常数)**:用电钻等带动电机到已知转速(转速可用我们的 hall_probe 链路读),量两相间反电动势(AC),换算 Ke;或查 KV(≈1517 RPM/V 是由 18200RPM@12V 粗推,手测更准)。
- 极对数 = 4(已知)。
> 用户当前**无仪表**,至少明天才可能有 → Plan B 今日不可用。**今日只走 Plan A**;A 若不成,等仪表再 B。

## 8. 标定后落盘
- 把测得的 Rs/Ls/Ke/额定电流/最高转速等 → 追加 [`FACTS.md`](FACTS.md)(新 F 编号,来源=Motor Profiler 或手工测,带日期)。
- 标定方式与结果、异常/重试经过 → 记本文件或新增 profiling 报告;更新 [`STATUS.md`](STATUS.md);commit + push。
- 标定完的 motor profile 文件路径记档,供下一步 MC Workbench 生成 FOC 工程用。

## 9. 一句话安全底线
**无可调限流是本次最大短板;用"Imax 低起步 + 串保险丝 + 低速起步 + 人工值守随时断电"四层兜住。任何一条红旗立即断电,别为把标定跑通而硬冲。**
