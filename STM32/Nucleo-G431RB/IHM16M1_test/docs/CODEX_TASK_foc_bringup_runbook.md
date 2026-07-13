# Codex 任务书 — 起草"首次闭环带电执行手册(runbook)",基于真实固件寄存器(纯文档 · 不烧录 · 不碰硬件)

> 给 Codex:读完即可自主执行。**这是纯离线文档任务**:只读生成工程源码 + 写 `docs/`;**绝不烧录、不连板、不上电、不转电机、不启动 MotorPilot/MC Workbench、不重生成、不改生成树。你只"写手册",绝不"执行手册"。**
> 手册涉及危险操作(烧录 + 给电机上电闭环),**安全闸与最终定稿由 CC 审核并负责**;你产出**标注 `DRAFT — 待 CC 安全审核`** 的草稿。
> 项目根 = `C:\Users\cheyh\Desktop\embedded\STM32\Nucleo-G431RB\IHM16M1_test`。真相以 `docs/` 为准:`FACTS.md`(F-28/F-36 等)、`FOC_BRINGUP_PREP.md`、`FOC_FW_BUILD.md`、`DECISIONS.md` #006(选 B,0.3A/100rpm)、`MOTOR_PROFILER_SOP.md`、`GITIGNORE_AND_REGEN.md`、`SYNC_WORKFLOW.md`、`OPERATIONS.md` §0。**动手前先读。**

## 背景与定位
- 正式霍尔 FOC 固件已生成 + 离线构建(0.3A Iqmax / 100rpm 默认目标,F-36);**未烧录、未带电**。项目停在"首次闭环带电"之前。
- 首次带电是**用户操作 + CC 旁站**的现场红线步(codex 干不了)。本任务是它的**离线前置产物**:把 `FOC_BRINGUP_PREP` 的分析,**落成一份精确到寄存器/操作步骤的执行手册草稿**,让现场少猜、可照做。
- 与 `FOC_BRINGUP_PREP` 的区别:那份是"审计/分析/风险";本份是"**照着做的操作 runbook**"——精确的监视寄存器清单、控制写入序列、启停/清障顺序、异常分支、急停判据。

## ⛔ 红线
- 纯离线:只读生成工程 `GM16020-06_foc/`、读 MCSDK 文档、写 `docs/`。
- **绝不**:烧录/写 flash、连板/枚举、启动 MotorPilot/Workbench 驱动功能、上电、转电机、重生成、改生成树。
- **不得在手册里替 CC 拍板安全决定**:凡涉及"是否上电/上到多少/何时放开限制"的判断,写成**选项 + 触发条件**,标注"须 CC/用户现场定",不自行下结论。
- 有疑或需硬件实测 → `ESC` 交回 CC。

## §0 权限预检
- 只读文件 + 写 `docs/`;如需查寄存器语义读本机 MCSDK 文档(`.../MC_SDK_6.4.2/Documentation/`,只读)。不做任何硬件动作。

## §1 已确认(直接用)
- 固件有效配置(F-36 / FOC_FW_BUILD / FOC_BRINGUP_PREP):Hall 主传感器、placement 58°/displacement 120°/TIM3 PC6-8;三电阻双 ADC;PWM TIM1 30kHz;**Iqmax=0.3A、nominal 0.3A、默认目标 100rpm**;OV15/UV7;DriverProtection PA11/BKIN2(硬关断,不可调);MCP over USART2 PA2/PA3 **1843200**;无自动 `MC_StartMotor1`;Ke 量化 0.4(不进控制)。
- 板/SN:NUCLEO-G431RB,SN `002A00403234510E33353533`;VCP 按 SN 现场扫(别写死 COM)。
- 供电 12.6V/1.9A **无可调限流、无保险丝**;0.3A 是软件限幅**不是短路保护**(DECISIONS #006-B);硬件保护=STSPIN830 DriverProtection + 用户 1 秒断电。
- 首次带电前:**指针保持拆除**,判方向靠慢动作/遥测。

## §2 你(codex)自己查(从真实生成代码抠精确项)
- **监视寄存器**:从 `Src/mc_configuration_registers.c` / MCP 寄存器定义 / MCSDK 文档,列出本固件里可读的关键量及其**寄存器名/ID**:Motor **STATE/STATUS**、**FAULT 标志位**(哪位=哪种故障:OV/UV/OC/OT/speed feedback/startup…)、**实测速度**、**转子/霍尔电角度**、**Iq/Id(实测 + 参考)**、**Vbus**、**温度**、Iqmax/限幅当前值。
- **控制写入项**:控制模式(Speed)、**速度斜坡/目标**(确认是 `SPEED_RAMP` 类命令还是写目标寄存器,给出**确切名/格式/单位**,例如"+100rpm、3000–5000ms")、**Start/Stop 命令**、**Fault Ack**。标注每项在 **MotorPilot 界面/寄存器**怎么下。
- **确认默认限幅**:0.3A/100rpm 在固件里的确切体现;运行时可下探到多低、上限受什么钳。

## §3 起草手册(`docs/FOC_BRINGUP_RUNBOOK.md`,标注 DRAFT)
按现场执行顺序组织,每步写"做什么 / 看什么 / 正常判据 / 异常与应对":
1. **前置安全闸**(照 MOTOR_PROFILER_SOP + FOC_BRINGUP_PREP):指针已拆、电机固定、母线可 1 秒断、人在场;母线电压范围;无保险丝的残余风险声明。
2. **设备探测**(OPERATIONS §0,只读确认 SN=本板、按 SN 认 COM)。
3. **HITL 授权 + 烧录**:给出**确切** `STM32_Programmer_CLI` 烧 `GM16020-06_foc.elf` 命令(HOTPLUG、认 SN、-v -rst);**标注"须 CC/用户逐次 HITL 授权 + 母线断电下执行;codex 不执行"**。
4. **MotorPilot 连接 + 监视布置**:连 COM(1843200),把 §2 的监视寄存器/图表摆出来。
5. **首次点动**:设 Speed 模式、速度斜坡 `+100rpm / 3–5s`,母线上电、Ack 残留故障,**Start**;
   - **正常判据**:平稳升到 ~100rpm、Iq 小而稳、无故障、方向为正(慢动作/角度确认);
   - **异常分支**:原地抽搐/不转/反转/Iq 饱和/报 fault → **立即 Stop + 断母线**;按 `FOC_BRINGUP_PREP` 的霍尔方向/相序修法分支(交换两相 or 两霍尔 or 改 placement/方向),写成"症状→动作"表。
6. **急停判据**:尖啸/剧抖/焦味/电机移位/Iq 顶格/反复 fault → 立即断电。
7. **首测后**:读实测空载转速/BEMF 与 Ke 预期对不对(Ke 存疑项)、是否需回 WB 调;后续放开电流/转速的**分级建议**(每级前提)。
8. 全篇醒目标注:**0.3A 是软件限幅非短路保护;无保险丝;每次放开限制前须 CC/用户重新评估**。

## §4 交付与落盘
- 新增 `docs/FOC_BRINGUP_RUNBOOK.md`(**顶部标 `DRAFT — 待 CC 安全审核,勿在未审核前执行`**)。
- 把 §2 抠到的**确切寄存器名/ID/格式**作为附录或表格入手册(有源码出处)。
- 更新 `STATUS.md`(反映"首次带电 runbook 草稿已出,待 CC 审核 + 用户授权带电")。
- 若发现固件里**与安全/正确性相关的真问题** → 醒目写 + `ESC` 交回 CC。
- **收尾 = commit + push**(SYNC_WORKFLOW)。

## §5 纪律
- 红线见顶部;你只写文档、不执行、不碰硬件、不替 CC 定安全。
- 结论要有源码/文档证据、完整留痕;改动限 `docs/`;Windows 路径含空格加引号。
- 任何"只能带电才能定"的项 → 标注留给现场,不臆断。
