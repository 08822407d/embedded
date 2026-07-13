# FOC_BRINGUP_PREP - GM16020-06 正式 Hall FOC 首次带电前离线审计

日期:2026-07-13

> 范围:只读 `GM16020-06_foc/` 生成配置/源码、MCSDK 6.4.2 本机板卡定义、MotorPilot 注册表/QML,并写 `docs/`。
> 本次未枚举/连接板卡,未调用 Programmer/ST-Link,未烧录/复位,未启动 MotorPilot/MC Workbench,未重生成,未修改生成树,未给母线上电或转电机。

## 0. 结论和现场闸门

1. **Ke 量化定论:接受生成的 `0.4`,不要仅为此重生成。** `0.449045... -> 0.4` 是 Workbench 输出宏/只读配置上报的 1 位小数量化,当前 Hall 主反馈 FOC 的已编译控制路径不读取该值。Workbench 的控制参数计算路径收到的是项目模型中的未量化 `BEmfConstant`,所以 `0.4` 字符串不会再反向污染 PI 计算。
2. **Hall 静态配置自洽。** F-23 的实测序列换算为 MCSDK 内部位序后,正好是生成状态机的 `POSITIVE` 序列;58 度相移和 120 度传感器布置均落入运行时 Hall handle。未记录当时手转是物理 CW 还是 CCW,所以“正速度对应机械哪一方向”仍须现场低速确认。
3. **保护链存在,但不能把它误当 0.5 A 硬件限流。** 过压/欠压/板级温度会进入安全任务并关 PWM;PA11 是 STSPIN830 active-low driver-protection 到 TIM1 BKIN2 的异步关断输入。项目的 `M1_OCP_TOPOLOGY=NONE`,没有 MCSDK 可设阈值的独立 OCP/BRK1;`IQMAX=0.5 A` 是速度 PI 输出/电流环参考上限,不是短路保险。
4. **默认上电不自动启动。** 三个桥臂 enable GPIO 先置低且有下拉,TIM1 automatic output 关闭,状态机初始 `IDLE`,应用层无 `MC_StartMotor1`;默认 500 rpm 只是缓冲参考。必须收到 MotorPilot/MCP Start 才进入启动状态机。
5. **现场前仍有一个 CC 决策闸。** 固定 12.6 V/1.9 A 电源若仍无可调限流和保险丝,后续不得把软件 `IQMAX=0.5 A` 当唯一保护。CC/用户应在“加限流/快熔保险丝后接受现有 0.5 A”与“回 Workbench 将 Iqmax/nominal current 降至 0.3 A 并重生成”中明确选一个。

## 1. 证据范围

关键静态证据:

- Ke/电机参数:`GM16020-06_foc.stwb6`, `GM16020-06_foc/Inc/pmsm_motor_parameters.h`, `Src/mc_configuration_registers.c`, `Src/sync_registers.c`。
- Workbench 生成器:`...STMCWB/assets/www/static/js_css/CmnStepProtectionsWbDef-t0GFRrKl.js` 与 `generateWbDef-Dp2NJj4N.js`。
- Hall:`Inc/pmsm_motor_parameters.h`, `Src/mc_config.c`, `Src/hall_speed_pos_fdbk.c`, `hall_probe_fw/src/main.c`。
- 保护/默认态:`GM16020-06_foc.ioc`, `Src/main.c`, `Src/mc_tasks.c`, `Src/mc_tasks_foc.c`, `Src/pwm_curr_fdbk.c`, `r3_2_g4xx_pwm_curr_fdbk.c`, IHM16M1 board JSON。
- MCP/MotorPilot:`Inc/register_interface.h`, `Src/sync_registers.c`, `Src/hf_registers.c`, MotorPilot `RegisterList/RegListSTMV2.json` 与 `GUI/MC_FOC_SDK.qml`。

以下只代表静态配置和源码闭包。相线实物顺序、机械 CW/CCW、PA11 真实触发、ADC 电流极性、实际 PI 稳定性仍须后续现场验证。

## A. Ke 量化定论

### A1. 传播闭包

| 环节 | 静态结果 | 对控制的意义 |
|---|---|---|
| 受控种子 | `BEmfConstant=0.4490450918674469` | 精确标定值仍保留,未丢失 |
| Workbench 序列化 | 生成器明确以 `w(a.BEmfConstant,1)` 输出 `MOTOR_VOLTAGE_CONSTANT` | 单独把 C 宏格式化为 1 位小数,所以得到 `0.4` |
| Workbench 控制参数计算 | 生成器另一路把 `BEmfConstant:a.BEmfConstant` 传入控制参数计算 | 使用内部未量化值,不是回读 `0.4` 宏 |
| 生成配置 | `.ioc/.wbdef` 有 `M1_MOTOR_VOLTAGE_CONSTANT=0.4`、`M1_MOTOR_RATED_FLUX=0.005499657` | 后者没有落入本工程运行时 C 控制对象 |
| 生成 C | `MOTOR_VOLTAGE_CONSTANT 0.4`; `M1_MotorConfig_reg.ratedFlux=0.4` | 宏在生成树无使用点;结构只供 MCP 配置上报 |
| MCP 写入 | `MC_REG_MOTOR_CONFIG` 的 Set 分支返回 `MCP_ERROR_RO_REG` | MotorPilot 不能在运行时改它 |
| 当前算法 | 主反馈=Hall;observer/feed-forward/flux weakening/MTPA/open-loop 均关闭 | 无 BEMF observer 或 Ke 依赖控制路径 |

### A2. 明确建议

- **本项目当前 Hall FOC 首测选择:接受 `0.4`,不重生成,也不手改 C。** 量化误差约 10.9%,但它没有进入当前运行时角度、电流环或速度环计算。
- Profiler 的 `0.385/0.62/0.449` 离散仍可能表现为**自动 PI 的模型/整定不确定性**,而不是 Hall 换相角错误。首次低速只观察速度环是否振荡、过冲、Iq 长时间饱和;不要把异常先归咎于 0.4 宏。
- 如果以后启用 STO/BEMF observer、前馈、磁链相关算法,再重新测 Ke 并在 **Workbench Motor 页的 B-EMF constant/用户电机资产**修正,重新生成后重新做本节闭包。若当时 Workbench 仍只输出 1 位小数,交回 CC/ST 支持,不要直接补丁生成文件。

## B. Hall 配置、方向与相序

### B1. 生成配置落地

- `HALL_SENSORS_PLACEMENT=DEGREES_120`, `HALL_PHASE_SHIFT=58`。
- `HALL_M1.SensorPlacement` 和 `HALL_M1.PhaseShift=58*65536/360` 直接进入 Hall handle;极对数为 4。
- PC6/PC7/PC8 分别为 H1/H2/H3,120 度分支不反相,内部状态按 `H3:H2:H1` 组装。
- Hall 算法用合法状态 `1..6`;`000/111` 会把传感器标为不可靠并最终产生 `MC_SPEED_FDBK`。

### B2. 实测序列与状态机

`hall_probe_fw` 的显示位序是 `H1,H2,H3`;MCSDK 内部整数位序是 `H3:H2:H1`。必须换算后再比较:

| F-23 显示 H1,H2,H3 | MCSDK STATE(H3:H2:H1) | 58 度配置下扇区中心 |
|---|---:|---:|
| `010` | 2 (`010`) | 268 deg |
| `011` | 6 (`110`) | 328 deg |
| `001` | 4 (`100`) | 28 deg |
| `101` | 5 (`101`) | 88 deg |
| `100` | 1 (`001`) | 148 deg |
| `110` | 3 (`011`) | 208 deg |
| `010` | 2 (`010`) | 268 deg |

因此 F-23 的 `010 -> 011 -> 001 -> 101 -> 100 -> 110 -> 010` 换算为 `2 -> 6 -> 4 -> 5 -> 1 -> 3 -> 2`,正是 `hall_speed_pos_fdbk.c` 的 `POSITIVE` 序列,每步电角度正向 60 度。结论:

- Hall 三通道顺序和 120 度选择与已测序列自洽。
- 该手转方向会被固件报告为正速度。
- 当时没有记录手转是机械 CW/CCW,所以用户期望的“正方向”仍可通过运行时正/负速度命令定义。
- 58 度由 Profiler 在当时的 U/V/W 与 H1/H2/H3 接线关系下标定;**只要之后未换相线/Hall 线**,静态证据支持保留。若接线变过,58 度不能沿用。

### B3. 首次现场症状与处理分支

| 现象 | 优先判定 | 处理,按成本从低到高 |
|---|---|---|
| 转动稳定、速度/Iq 正常,但机械方向不是用户想要的方向 | 只是方向约定不同 | Stop 后把 `SPEED_RAMP` 目标改为负值;这是运行时改法,不要动线 |
| 正速度命令时 Hall speed/机械 speed 为负,或转子一冲就反向 | 相/Hall 坐标关系或现场接线与标定时不同 | 立即 Stop/断母线;恢复 Profiler 时记录的 U/V/W、H1/H2/H3 接线。不能靠随机换线继续试 |
| 原地抽搐、尖叫、每 60 度猛跳、Iq 接近 0.5 A 而速度不起 | Hall 相移错误、相序错、电流极性错之一 | 立即断电并保存遥测;交 CC。先核接线,再决定重做 Hall profile/改 phase shift/重生成 |
| 方向正确但 Id 与 Iq 同量级、噪声大、发热快、转矩弱 | 58 度偏移或相电流/相序错误 | 不用 PI 掩盖;先重核 Hall placement/相序。`HALL_PHASE_SHIFT` 不是本工程 MCP 可写寄存器,修正须 WB 重生成 |
| `MC_SPEED_FDBK`,Hall angle 不动或只在少数值跳,可能见 000/111 | Hall 供电/接线/通道故障 | 母线断电后查 Hall 5V/GND/H1-3;stock MCP 不暴露原始三位 Hall state |

**不要把“交换两根线”当现场微调:**交换两相会反转电气相序;交换两根 Hall 会改变状态序列。两者都会使现有 58 度标定失效,必须断电操作、重新 Hall profiling、重新生成/编译。单纯常量角偏差用 Workbench Hall phase shift 修,不能用换线碰运气。

## C. 故障保护与安全态审计

| 项目 | 生成值/路径 | 审计结论 |
|---|---|---|
| 软件过压 | PA0 ADC1;15 V;`TURN_OFF_PWM` | 使能;安全任务报 `MC_OVER_VOLT` 并关 PWM。没有制动电阻/低侧刹车 |
| 软件欠压 | PA0 ADC1;7 V | 使能;报 `MC_UNDER_VOLT` 并关 PWM。USB-only 启动时出现/锁存 UV 是预期现象 |
| 板级温度 | PC4 ADC2;110 C,回差 10 C | 使能;报 `MC_OVER_TEMP` 并关 PWM。它保护功率板传感点,**不测电机绕组温度** |
| 速度模式电流上限 | `IQMAX_A=0.5`,速度 PI 输出限幅 +/-0.5 A | 有效的软件电流参考上限,但不是独立硬件 trip |
| MCSDK OCP | `M1_OCP_TOPOLOGY=NONE`;TIM1 BRK1 disabled | 没有 comparator/DAC OCP 阈值,MotorPilot 也无本工程可写 OCP threshold |
| Driver protection | IHM16M1 JSON=active low;PA11 -> TIM1 BKIN2,filter 4 | 硬件 Break2 异步触发 `PWMC_DP_Handler`,立即关 MOE/enable 并上报 `MC_DRV_PROTEC`。原因/电流阈值不由固件区分 |
| 默认 GPIO/PWM | PB13/14/15 初始化低+下拉;TIM1 automatic output disabled | 上电默认功率级关闭 |
| 自动启动 | 状态初始 `IDLE`;无 `MC_StartMotor1`;start/stop button与potentiometer关闭 | 不自动启动;MotorPilot Start/MCP start 是唯一正常入口 |
| 故障恢复 | `FAULT_NOW -> FAULT_OVER`,保持到显式 fault acknowledge | 不自动重启,符合首测需要 |
| HardFault | `TSK_HardwareFaultTask -> FOC_Clear -> PWMC_SwitchOffPWM` | 软件硬故障路径关 PWM |
| Stop/过压动作 | `SwitchOffPWM` 清 MOE 与三路 enable | 电机自由滑行,**PA11 Break 不是机械/能耗制动** |

### C1. 12.6 V 母线解释

- 正常 12.6 V 位于 7-15 V 软件窗口内,距 OV 约 2.4 V、距 UV 约 5.6 V。
- 按安全规则先在母线断开时烧录/启动后,UV fault 很可能已锁存。后续经单独授权接入母线后,先确认 `BUS_VOLTAGE` 约 12-13 V、当前 UV 已消失,再执行一次 fault acknowledge 回 `IDLE`;不要在 Vbus 仍异常时反复清 fault。
- 过压策略只是关 PWM,没有动态刹车。首测 100 rpm 风险低;以后高速急停/回生时必须重新评估 15 V 阈值和母线吸收能力。

## D. 保守首测参数集

### D1. 建议值与可改性

| 项目 | 本工程现值 | 建议首测 | 改动方式/持久性 |
|---|---:|---:|---|
| Control mode | Speed | **Speed,不切 Torque/Open Loop** | `CONTROL_MODE` 运行时可写,复位恢复生成值 |
| 首次目标速度 | 默认 500 rpm | **+100 rpm** | `SPEED_RAMP` 运行时可写;在 Start 前写入可覆盖缓冲的 500 rpm |
| 速度斜坡 | 未固定为首测值 | **3000-5000 ms** | `SPEED_RAMP=[rpm,duration_ms]` 运行时可写 |
| Iqmax/速度 PI 输出限幅 | 0.5 A | 有硬件兜底时接受 **0.5 A**;否则建议 **0.3 A** | stock MCP **不可改 Iqmax**;降到 0.3 须 Workbench 改 `M1_IQMAX/nominal current` 后重生成 |
| Id reference | 0 A | **0 A** | `CURRENT_REF/I_D_REF` 运行时可写,但首测保持默认 |
| Speed PI | Kp 2208/1024,Ki 21/16384 | **保持生成值** | Kp/Ki/divisor 运行时可写但不持久;异常先 Stop,交 CC 决定调参/回 WB 固化 |
| Iq/Id PI | Kp 2445/4096,Ki 2412/16384 | **保持生成值** | 运行时可写但不持久;首测不改 |
| OV/UV | 15/7 V | **保持** | 虽通用头定义了寄存器 ID,本工程 `sync_registers.c` 无 handler;须 WB 重生成 |
| 温度阈值 | 110 C/10 C | **保持,但不依赖它保护电机** | 须 WB 重生成 |
| Hall 120/58 | 120 deg/58 deg | **保持** | 本工程无 `HT_PHASE_SHIFT` handler;须 WB 重生成/重标定 |
| PWM | 30 kHz | **保持** | 须 WB 重生成;低电感纹波异常时再由 CC评估提高 |
| Ke C 宏 | 0.4 | **接受** | 不需要运行时改,不因首测重生成 |

`I_Q_REF`/`CURRENT_REF` 虽可写,但在 Speed mode 下会被速度 PI 生成的 Iq 参考覆盖,**不能充当 0.3 A 的速度模式电流上限**。Torque mode 虽可直接给小 Iq,但没有同等速度闭环约束,不作为这台高 KV 电机的首次方案。

### D2. 供现场 SOP 采用的顺序(本次未执行)

1. 经新的烧录 HITL 后,母线仍断开,先用 USB/MotorPilot 只读确认状态和 Hall angle/speed;手转只用于确认角度递增和方向符号。
2. 经单独母线上电授权及物理闸后接 12.6 V;先看 Vbus/温度/fault,处理预期的 UV 锁存并回到 `IDLE`。
3. 在按 Start **之前**写 `SPEED_RAMP=[+100,3000..5000]`;确认 Control mode=Speed。
4. 只点一次 Start。首段只验证能否平稳进入 RUN、速度符号、Iq/Id 和 fault;不要连续自动重试。
5. 任一红旗立即 Stop;若 Stop 后仍有驱动/异响,立即断母线。停止是自由滑行,不能期待电磁急刹。

## E. MotorPilot 遥测方案

MotorPilot 6.4.2 默认 `MC_FOC_SDK.qml` 中:

- 左侧 `Status` 框看状态/fault;`Control` 框选择 Speed 并执行 Start/Stop。
- `Speed Control`/`Speed Ramp` 设置目标与 duration。
- `Measures` 看 VBUS/TEMP/POWER 和 Speed/Id/Iq。
- 需要 Hall angle、Iq reference 等时,在 Data Log/Plot 的寄存器选择器加入下列注册名。生成工程 UART data log 最多 10 个信号。

### E1. 建议 Plot 8 项

| 注册名 | 正常判据 | 异常/立即停止判据 |
|---|---|---|
| `SPEED_REF` | 0 -> +100 rpm,按 3-5 s 上升 | 不是预设值;先不要 Start |
| `SPEED_MEAS` | 与 reference 同号并受控接近 | 反号;超过目标约 2 倍仍上升;剧烈来回翻转 |
| `HALL_SPEED` | 与 F-23 正向定义、`SPEED_MEAS` 同号 | 反号/突变/长期为 0。它与 SPEED_MEAS 同源,不是独立测速旁证 |
| `HALL_EL_ANGLE` | 按 60 度扇区连续前进并周期回绕;4 对极每机械圈回绕 4 次 | 不动、反复跨扇区抖、方向与速度不一致 |
| `I_Q_REF` | 启动短时上升,受约 0.5 A 限制,速度起来后回落 | 接近 0.5 A 持续而速度不升 |
| `I_Q_MEAS` | 跟随 Iq ref,允许 30 kHz/低电感带来的纹波 | 长时间顶限、符号乱跳、明显不跟随 |
| `I_D_MEAS` | 目标为 0,平均值应明显小于 Iq | 持续与 Iq 同量级或约 >0.1-0.2 A,伴随噪声/发热 |
| `BUS_VOLTAGE` | 稳在约 12-13 V | <7 V、接近/超过 15 V、明显塌陷/上冲 |

### E2. 常驻 Watch 3 项

- `STATUS`:预期 `IDLE -> ... -> RUN`;Stop 后回 IDLE。卡在 START、FAULT_NOW/FAULT_OVER 都不要重试。
- `FAULTS_FLAGS`:重点看 `MC_OVER_VOLT`, `MC_UNDER_VOLT`, `MC_OVER_TEMP`, `MC_START_UP`, `MC_SPEED_FDBK`, `MC_BREAK_IN`, `MC_DRV_PROTEC` 及 `_NOW` 位。PA11 正常报告应是 `MC_DRV_PROTEC`,不一定是 `MC_BREAK_IN`。
- `HEATS_TEMP`:起始应接近环境/板温且短时基本不变;它不是电机绕组温度。

stock MCP 没有原始 `H1/H2/H3` 或三位 `HallState` 注册表。若后续必须看原始位,需另行设计只读诊断/逻辑分析仪方案并走新任务,不能在现场临时改被忽略生成树。

## F. 风险台账

| 风险 | 触发条件/表现 | 缓解与响应 | 状态 |
|---|---|---|---|
| 无可调限流、无保险丝 | 短路/相序错时固定电源可供约 1.9 A | 首测前加限流或约 1-1.5 A 快熔;否则回 WB 降 Iqmax,并由 CC 决策 | **开放闸门** |
| `IQMAX=0.5 A` 不是硬件 trip | current loop 失效/驱动故障 | 不把软件限幅当保险;依赖 DP+物理断电 | 已确认边界 |
| DP 触发原因/阈值不可见 | PA11 active-low | 看 `MC_DRV_PROTEC`,断电查因;禁止盲目清 fault 重试 | 须现场验证 |
| 正方向机械定义未知 | F-23 未记 CW/CCW | 100 rpm 低速确认;若稳定但方向不合预期,改速度符号 | 待现场 |
| 相/Hall 接线可能与 Profiler 时不同 | 线被交换/重插 | 首测前核记录;变化则 58 度失效,须重标/重生成 | 待人工确认 |
| Hall 60 电角度低速量化 | 100 rpm 有 40 Hall edge/s(4 对极) | 允许一定低速转矩纹波;抽搐+Iq 饱和不是“正常量化” | 固有风险 |
| 低电感 0.19 mH/高 KV | 电流纹波、FOC duration、噪声 | 保持 30 kHz首测;观察 Iq/Id/`MC_FOC_DURATION`,异常交 CC评估 PWM/PI | 待现场 |
| Ke run 间离散 | 自动 PI 模型可能偏 | 0.4 宏无需修;低速观察速度环,必要时运行时调 PI后回 WB 固化 | 待现场 |
| 仅板级温度保护 | 微型电机绕组先热而 PC4 未到 110 C | 短时测试、手工感知/后续加电机温度策略;异味/升温立即断电 | 开放风险 |
| 15 V OV且无动态刹车 | 高速急停/回生抬母线 | 首测仅100 rpm;以后高速另做母线吸收/减速审计 | 后续风险 |
| GUI 可输入到 19806 rpm | 误输入/滑块误操作 | Start 前逐字确认 100 rpm和 duration;禁止直接沿用界面默认/500 rpm固件默认 | 人因风险 |
| USB-only 会锁存 UV | 母线断开启动固件 | 上母线后先验证 Vbus,只 ack 一次;UV仍在则不启动 | 预期行为 |
| 无原始 Hall state 遥测 | 只能看 angle/speed | 先用已有 F-23 旁证;需要原始位时另立只读任务 | 工具限制 |

## 7. 交回 CC 的决策项(ESC)

```text
==== 需 CC 决策 [ESC-1] ====
背景/卡点:正式工程 speed-mode Iqmax=0.5 A,stock MCP 无运行时 Iqmax setter;现有固定 12.6 V/1.9 A 电源记录为无可调限流、无保险丝。
我已查:0.5 A 已进入 speed PI 输出限幅;M1_OCP_TOPOLOGY=NONE;PA11/BKIN2 只有 combined driver-protection,阈值/原因不可由固件确认。
需要 CC 定的:首测前选择“加限流/快熔后接受0.5 A”或“用户回 WB 将 Iqmax/nominal current 改0.3 A并重生成+重编译”。Codex不改、不重生成。
====
```

```text
==== 需 CC 决策 [ESC-2] ====
背景/卡点:Hall 电气正向已静态确认,但 F-23 未记录该手转方向是机械 CW 还是 CCW;58 度仅在 Profiler 当时线序不变时有效。
我已查:F-23 显示序列换算后与 MCSDK POSITIVE 状态机完全一致;不存在静态 Hall 通道顺序冲突。
需要 CC 定的:现场 SOP 规定期望机械正方向,并在100 rpm首测确认;若线序曾改变,先恢复或重做 Hall profile,不要随机换线带电试。
====
```

## 8. 本任务停止点

离线准备已完成。没有发现必须在烧录前修补生成源码的错误;Ke `0.4` 不构成重生成理由。开放项均已上收为现场/CC 决策,本报告不授权烧录、连接 MotorPilot、接母线或启动电机。
