# CC 已安全审核(2026-07-13)· 执行仍须完成 §11 两次现场 HITL + 用户接受残余风险

# GM16020-06 Hall FOC 首次闭环带电执行手册

日期:2026-07-13

> **CC 审核意见(2026-07-13)**:已逐节安全审核,程序稳妥、无安全错误,批准作为**现场执行程序**。但"审核通过"≠"授权带电"——**执行授权仍是 §4.2 烧录 HITL 与 §6.2 母线上电 HITL 两道现场闸,外加用户接受"无保险丝/固定 1.9A 电源"的残余风险**(§0 选项 A/B 现场定)。
> 执行前必须仍然成立的三条硬前提(任一存疑即 `[ESC]`):① **Profiler 之后 U/V/W 与 H1/H2/H3 线序未变**(否则 58° 标定作废,须重标);② 母线可 1 秒物理断、有人值守;③ 烧前 ELF SHA-256 与 §1 一致。此外任何"放开电流/转速/改 Iqmax"仍须回本文 §10 分级 + 重新审核。

> **本文件只是现场执行草稿,不是烧录、上电或转电机授权。** 烧录与母线上电是两个独立红线步骤,每一步都必须由用户与 CC 在现场重新确认。Codex 本次只离线读取生成源码并编写本文,没有连接板卡、烧录、启动 MotorPilot/Workbench、给母线上电或转电机。

## 0. 必须先读的结论

> **0.3 A 是速度 PI 输出/电流参考的软件限幅,不是短路保护。现有 12.6 V/1.9 A 电源无可调限流,回路无保险丝。**
>
> PA11/TIM1_BKIN2 的 STSPIN830 DriverProtection 能异步关 PWM,但阈值/原因在本工程中不可调、不可区分;它不是制动器,也不能替代保险丝、限流电源或人工断电。任何放开速度/电流的动作,均须 CC/用户重新评估并现场决定。

本文使用以下现场标记:

- **`[GATE]`**:条件不全就不进入下一步。
- **`[HITL]`**:必须停下,取得用户当次明确确认。
- **`[STOP]`**:立即发 MotorPilot **Stop motor**,同时准备或直接切断母线。
- **`[POWER-OFF]`**:物理切断 IHM16M1 外部母线;USB 是否保留由现场排障需要决定。
- **`[ESC]`**:停止试验,把证据交给 CC;不靠反复 Start 或随机换线试错。

安全判断一律由 CC/用户现场决定。本文给出的两个前置选项是:

| 选项 | 触发条件 | 现场决定 |
|---|---|---|
| A. 延后首测,先增加快熔保险丝或可调限流电源 | CC/用户不接受“无保险丝 + 固定 1.9 A 电源”的残余风险,或无法保证 1 秒内物理断母线 | **须 CC/用户现场定** |
| B. 保持 0.3 A/100 rpm 做一次短时受控点动 | 所有安全闸全绿,用户明确接受残余风险,且全程有人值守并能立即断母线 | **须 CC/用户现场定;本文不推荐或批准该选择** |

## 1. 固件基线与停止点

只能使用下列已离线构建并审核过的 Debug ELF:

```text
GM16020-06_foc\STM32CubeIDE\Debug\GM16020-06_foc.elf
SHA-256 = 0036DEE8C9431ECA4AC763BF9B481E0C56F23C238EE5B1E77078C0A387092DB0
```

有效配置:

| 项目 | 固件值 |
|---|---|
| 目标板 | NUCLEO-G431RB,ST-Link SN `002A00403234510E33353533` |
| 功率板/电机 | X-NUCLEO-IHM16M1 / GM16020-06,4 对极 |
| 主反馈 | Hall,TIM3 PC6/PC7/PC8,120 度,电角度偏移 58 度 |
| 电流采样 | 三电阻、双 ADC、外置运放 |
| PWM | TIM1,30 kHz |
| 控制 | Speed mode;默认目标 100 rpm;Iqmax/nominal current 0.3 A |
| 保护阈值 | OV 15 V;UV 7 V;功率板温度 110 degC;PA11 DriverProtection |
| MCP | USART2 PA2/PA3,`1843200` baud;UART A data log 最多 10 项 |
| 自动启动 | 无应用层 `MC_StartMotor1`;复位后不会自行请求 Start |

源码边界检查接受 `-19806..+19806 rpm`,且配置下限为 `0 rpm`。这只是命令合法范围,**不是安全许可或低速闭环能力保证**。Hall 每机械圈仅有 `6 x 4 = 24` 个边沿;本草稿只批准候选首测值 `+100 rpm / 3000..5000 ms`,更低、更高或反向均须 CC/用户另行决定。

## 2. 前置安全闸

### 2.1 物理闸

现场逐项口头复核并记录:

- [ ] 轴上打印指针及其他附着物保持拆除。
- [ ] 电机机身牢固固定,三相线和 Hall 线不会卷入转轴;无人靠近旋转端。
- [ ] Profiler 之后 U/V/W 与 H1/H2/H3 没有交换、重插或重新标号。不能确认则 `[ESC]`,58 度标定不得沿用。
- [ ] 12.6 V 母线极性正确,尚未接通;电源开关/插头触手可及,用户能在 1 秒内物理断电。
- [ ] 人员全程在场;建议护目镜;不安排无人值守或自动重复 Start。
- [ ] 已明确现状为固定约 1.9 A 电源、无可调限流、无保险丝,并由用户与 CC 选择 §0 的 A 或 B。
- [ ] 烧录授权与母线上电授权分开;烧录成功不等于允许接通母线。

**正常判据:**全部勾选,且 CC/用户明确记录选项。

**异常与应对:**任一项不满足即停在本节。不得用“100 rpm 很低”或“0.3 A 已限流”跳过硬件闸。

### 2.2 接线和额定值闸

- 控制板与功率板应为本项目记录的 NUCLEO-G431RB + IHM16M1。
- 母线候选值为 `12.6 V`,位于 IHM16M1 `7..45 V` 范围内;固件 UV/OV 窗口是 `7..15 V`。
- 当前 `HEATS_TEMP` 是 PC4 功率板温度,不是电机绕组温度;短时温度读数正常不能证明绕组安全。
- IHM16M1 实物采样跳线应保持已确认的默认三电阻状态:J5/J6 closed、JP4/JP7 open。

## 3. 设备探测,母线保持断开

> 本节命令只供未来现场执行。本次 Codex 没有执行。

### 3.1 `[HITL]` USB 与目标身份

用户先确认:母线物理断开、仅 USB/Nucleo 供电、目标板 USB 已连接。然后现场运行:

```powershell
$prog = "C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
& $prog -l
```

必须同时看到:

- Board = `NUCLEO-G431RB`;
- ST-Link SN = `002A00403234510E33353533`;
- VCP 条目属于同一 SN;记录**当次** COM,不要沿用历史 `COM4`;
- 后续 SWD 连接报告 Device ID `0x468`。若同机有多块板或身份不唯一,立即停止。

**异常与应对:**SN、板名、VCP 关联或 Device ID 任一不符 -> 不烧录,由用户检查 USB/目标板;不得改成“不指定 SN”继续。

## 4. 烧录,母线仍保持断开

### 4.1 校验待烧产物

```powershell
$elf = (Resolve-Path "GM16020-06_foc\STM32CubeIDE\Debug\GM16020-06_foc.elf").Path
(Get-FileHash -Algorithm SHA256 -LiteralPath $elf).Hash
```

只在输出严格等于 `0036DEE8C9431ECA4AC763BF9B481E0C56F23C238EE5B1E77078C0A387092DB0` 时继续。不同则 `[ESC]`,先查构建/产物漂移。

### 4.2 `[HITL-FLASH]` 每次烧录单独停等

现场必须得到包含以下三项的当次明确回复:

```text
HITL-FLASH 授权:
1. 授权烧录本次已核 hash 的 GM16020-06_foc.elf;
2. IHM16M1 外部母线已物理断开,当前仅 USB/Nucleo 供电;
3. 目标板为 NUCLEO-G431RB,SN 002A00403234510E33353533。
```

缺一项、回复含糊或目标状态改变,都必须重新确认。授权后才由用户/CC现场执行:

```powershell
& "C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" `
  -c port=SWD mode=HOTPLUG sn=002A00403234510E33353533 `
  -w "GM16020-06_foc\STM32CubeIDE\Debug\GM16020-06_foc.elf" -v -rst
```

**正常判据:**连接对象显示 G431/Device ID `0x468`,program 与 verify 成功。`-rst` 会让新固件复位运行,但源码没有自动 Start。

**异常与应对:**任何连接、写入或校验失败 -> 不重试、不改 SN、不接母线;保留完整 CLI 输出交 CC。

**烧录后的硬停止点:**母线继续断开。必须经过下一节监视布置和另一次母线上电授权,才可触碰 12.6 V。

## 5. MotorPilot 连接与监视布置,母线仍断开

### 5.1 连接

由用户启动 MCSDK 6.4.2 MotorPilot,按 §3 当次扫描得到的 COM 连接,串口为 `1843200` baud。不要启动 Motor Profiler,不要用 Workbench 重生成。

USB-only 时 `BUS_VOLTAGE < 7 V` 可能产生 `MC_UNDER_VOLT[_NOW]`,这是预期保护表现;**不要在母线断开时反复 Ack 或 Start**。

### 5.2 常驻 Watch

| MotorPilot 注册名 | 看什么 | 正常/说明 |
|---|---|---|
| `STATUS` | 状态机 | USB-only 可能是 `FAULT_NOW`;母线有效且当前故障消失后到 `FAULT_OVER`,Ack 后才应为 `IDLE` |
| `FAULTS_FLAGS` | 当前/历史故障位 | 高 16 位是 current/NOW,低 16 位是 occurred/past;见附录 B |
| `HEATS_TEMP` | 功率板温度,degC | 起始接近环境/板温;不是绕组温度 |
| `CONTROL_MODE` | 控制模式 | 首测必须显示 `SPEED`(枚举值 3) |

### 5.3 Plot 八项

UART A data log 最多 10 项,首测建议固定以下 8 项:

| 注册名 | 单位 | 正常趋势 | 红旗 |
|---|---:|---|---|
| `SPEED_REF` | rpm | Start 后按 3-5 s 从当前值升到约 +100 | Start 前不是计划值;运行中意外跳变 |
| `SPEED_MEAS` | rpm | 与 reference 同号并受控接近 | 反号、剧烈翻转、超过目标约 2 倍仍上升 |
| `HALL_SPEED` | rpm | 与 `SPEED_MEAS` 同号、趋势一致 | 长期 0、反号或突变;注意两者同源,不是独立旁证 |
| `HALL_EL_ANGLE` | deg electrical | 随转动连续推进并周期回绕;4 对极每机械圈回绕 4 次 | 卡住、来回跳扇区、方向与速度不一致 |
| `I_Q_REF` | A | 启动时上升,受约 0.3 A 软件上限,速度建立后应回落 | 接近 0.3 A 持续而速度不上升 |
| `I_Q_MEAS` | A | 跟随 Iq ref;低电感/30 kHz 下允许纹波 | 长时顶限、符号乱跳、明显不跟随 |
| `I_D_MEAS` | A | 目标为 0,平均绝对值应明显小于 Iq | 持续与 Iq 同量级,并伴噪声/发热 |
| `BUS_VOLTAGE` | V | 母线上电后约 12-13 V | <7 V、接近/超过 15 V、明显塌陷或上冲 |

原始 H1/H2/H3 三位状态没有暴露在 stock MCP 中;不得在现场临时改生成树补遥测。

## 6. 100 rpm 首次点动

### 6.1 母线断开时预置,不要 Start

1. 在 Control 区选择 `Speed`;读回 `CONTROL_MODE=SPEED`/3。
2. 在 `Speed Ramp` 中填 `Target Speed = +100 RPM`,`Duration = 3000..5000 ms`;点击 `Execute Speed Ramp`。
3. 读回 `SPEED_RAMP=[100,所选 duration]`。不要靠 Speed slider,因为滑块写入 duration=0 的瞬时参考。
4. 确认 Start 尚未点击,图表/Watch 已开始记录,Stop 按钮位置和物理断电动作已演练清楚。

生成的 Hall 启动路径在切入 RUN 前先把速度 reference 强制为当时实测速度(静止时约 0),再执行已缓冲的 speed ramp;因此这里的 3000-5000 ms 确实作用于从当前速度到 +100 rpm,不是仅修改静态默认值。

**正常判据:**模式、目标和 duration 均读回正确;母线仍为 0 V/UV fault;电机不动。

**异常与应对:**任何值不符 -> 不 Start,不接母线;排查 GUI/连接后重新读回。

### 6.2 `[HITL-POWER]` 单独授权母线上电

这不是烧录授权的延续。用户与 CC 必须现场重新确认:

```text
HITL-POWER 授权:
1. §2 全部安全闸仍有效,用户在场且可 1 秒断母线;
2. MotorPilot 已连接并记录 STATUS/FAULTS/speed/Hall/Iq/Id/Vbus/temp;
3. CONTROL_MODE=SPEED,SPEED_RAMP=[+100 rpm,3000..5000 ms],尚未 Start;
4. 用户与 CC 已选择并接受 §0 的现场选项。
```

授权后由用户接通 12.6 V 母线。先等待、观察,**不要立刻 Start**:

- `BUS_VOLTAGE` 应约 12-13 V,处于 7-15 V 固件窗口;
- `FAULTS_FLAGS` 的 `MC_UNDER_VOLT_NOW` 应清零;
- 状态应从 `FAULT_NOW` 转到 `FAULT_OVER`。只有 current/NOW 全部清零且状态为 `FAULT_OVER` 时,才允许按一次 `Ack Faults`;
- Ack 后必须读回 `STATUS=IDLE` 且 `FAULTS_FLAGS=0`。MCP 的 Fault Ack 分支无论底层是否实际接受都返回 OK,所以只看按钮/返回消息不够。

若 UV_NOW 仍在、Vbus 不在窗口、出现其他 fault 或 Ack 后不回 IDLE -> `[POWER-OFF]` + `[ESC]`。

### 6.3 Start 一次

再次读回 `CONTROL_MODE=SPEED` 和 `SPEED_RAMP=[+100,3000..5000]`,然后由用户在 MotorPilot **只点击一次 Start**。

生成的 Hall 路径状态预期:

```text
IDLE -> OFFSET_CALIB(需要时) -> CHARGE_BOOT_CAP -> RUN
```

首测只有同时满足下列全部条件,才可判为候选正常;否则不要等软件自己恢复:

- 平稳升速,约 3-5 s 接近 +100 rpm;
- `SPEED_MEAS`、`HALL_SPEED` 与 `SPEED_REF` 同号;
- Iq 在启动后回落,不持续贴近 0.3 A;Id 平均明显小于 Iq;
- Vbus 保持约 12-13 V,无 fault,无尖啸/剧抖/焦味/位移。

上面全部成立才是候选“正常”。由 CC/用户现场决定保持多久;首测目的只是验证闭环方向、Hall/相序和保护遥测,不是耐久或性能测试。完成后点击 **Stop motor**,确认状态经 STOP 回到 IDLE,再物理断母线。

## 7. 异常分支:症状 -> 动作

| 症状 | 立即动作 | 断电后诊断/可选修法 |
|---|---|---|
| 转动稳定、速度/Hall/Iq 正常,但机械方向不是期望方向 | 正常 Stop 后断母线 | 这只是机械正方向定义。下一次经 CC/用户授权可选负速度命令;无需仅为方向偏好换线 |
| `SPEED_REF` 为正,但 `SPEED_MEAS`/`HALL_SPEED` 为负或起步反冲 | `[STOP]` + `[POWER-OFF]` | 核对 Profiler 后相线/Hall 线是否改过;禁止直接改负目标掩盖反馈符号错误 |
| 原地抽搐、尖啸、60 度附近来回跳、Iq 贴近 0.3 A且不加速 | `[STOP]` + `[POWER-OFF]` + `[ESC]` | CC 判断 Hall phase shift、相序、Hall 顺序或电流极性;不要连续重试 |
| `HALL_EL_ANGLE` 卡住/跳变,`HALL_SPEED=0`,报 `MC_SPEED_FDBK` | `[STOP]` + `[POWER-OFF]` | 查 Hall 5 V/GND/H1-H3 接线;如需原始位,另立只读诊断任务 |
| `I_D_MEAS` 长时与 Iq 同量级,噪声/发热明显 | `[STOP]` + `[POWER-OFF]` + `[ESC]` | CC 评估 58 度偏移、相/Hall 映射和 PI;不得现场随意改寄存器硬顶 |
| `MC_DRV_PROTEC[_NOW]` | 物理断母线优先 | PA11 只表示 combined DriverProtection,不能从该位推断具体原因;断电查功率级/接线 |
| `MC_BREAK_IN[_NOW]`/over-current、OV、OT、START_UP、FOC_DURATION 或反复 fault | 物理断母线 | 保存 fault 字和曲线交 CC;不得清 fault 后自动重试 |

### 7.1 相线/Hall 线修法的边界

只有以下选项,且都须 CC/用户在断电状态决定:

1. **稳定但机械方向不合偏好:**优先在下一次授权测试中改变速度目标符号。
2. **线序与 Profiler 记录不一致:**恢复 Profiler 当时 U/V/W、H1/H2/H3 原线序。
3. **有意交换任意两相或任意两根 Hall 线:**该动作会改变相/Hall 对应关系,现有 58 度 placement 不能继续视为有效。必须断电后重做 Hall profiling,回 Workbench 固化结果,重生成、重构建并重新走本文审核/HITL。
4. **序列正确但呈稳定角度偏差:**由 CC 决定是否在 Workbench 修改 Hall phase shift;同样需要重生成/重构建。不得仅凭一次抽搐猜一个角度。

## 8. 急停判据

以下任一项出现,立即物理断母线;来得及且界面仍响应时同时按 **Stop motor**:

- 尖啸、剧抖、敲击、焦味、冒烟、电机或夹具位移;
- Iq ref/meas 接近 0.3 A 持续而速度不上升;
- 速度反号、剧烈翻转、超过 +100 rpm 约 2 倍仍继续上升;
- Vbus <7 V、接近/超过 15 V或明显塌陷/上冲;
- `FAULT_NOW`、DriverProtection、过流、过压、过温、startup/speed feedback/FOC duration fault;
- MotorPilot 失联而电机仍受控/状态不明。

重要语义:

- **Stop motor 是停止 PWM/控制并让电机自由滑行,不是电磁刹车。** 不要用它保证瞬停。
- **Stop ramp 只停止参考斜坡并保持当时参考,不会停电机。急停绝不能点 Stop ramp。**
- `Start/Stop` toggle 根据当前状态二选一,现场避免使用;分别点明确的 Start 和 Stop。
- 软件/串口不可靠时,物理断母线优先于等待 GUI。

## 9. 首测记录与 Ke 复核

### 9.1 每次点动必须保存

- 日期、操作者、CC 旁站人、ELF SHA-256、当次 COM 与 SN;
- 母线实测/遥测值、SPEED_RAMP、机械实际方向(CW/CCW,观察端定义清楚);
- STATUS 状态序列、完整 32 位 FAULTS_FLAGS;
- speed ref/meas、Hall speed/angle、Iq ref/meas、Id meas、Vbus、temperature 曲线;
- 最大/稳态值、运行时长、Stop 后状态和是否需要人工断母线;
- 三相/Hall 接线在试验前后是否改变。

### 9.2 Ke 不能由 100 rpm 闭环点动直接确认

Profiler 最终值为 `Ke=0.449 V RMS phase-to-phase/kRPM`,生成代码中的 `0.4` 只是只读配置上报量化,当前 Hall FOC 控制路径不使用它。Stock Hall MCP 没有实测 BEMF 寄存器;`SPEED_MEAS` 跟踪 100 rpm 只能验证速度环,**不能证明 Ke 正确**。

按 0.449 计算,开路线间反电动势预期为:

```text
E_line-line_RMS = 0.449 * speed_rpm / 1000
100 rpm -> 0.0449 V RMS
1000 rpm -> 0.449 V RMS
```

100 rpm 仅约 44.9 mV RMS,不适合作为本次现场可信 Ke 复核。若 CC 判断必须量化复核,可选:

| 选项 | 触发条件 | 边界 |
|---|---|---|
| 重复 Motor Profiler | 首测控制异常且排除接线/Hall后仍怀疑模型 | 属新的带电任务,重新写 SOP/HITL |
| 外力以已知转速带动断电电机,测开路线间 RMS 电压 | 有合适转速源、转速旁证和仪表 | 功率级必须与测量方案电气隔离;方案由 CC/用户另审 |
| 暂不重测 Ke | 100 rpm 闭环稳定,当前仍使用 Hall 主反馈 | 只关闭首次低速阻塞项,不等于 Ke 已验证 |

**须 CC/用户现场定;本 runbook 不执行或批准任何 Ke 测试。**

## 10. 分级放开建议

| 级别 | 候选范围 | 进入前提 | 退出/升级条件 |
|---|---|---|---|
| L0 | USB-only,不接母线 | 固件身份、寄存器、UV 行为和监视布置正确 | CC 审核本文并单独授权 L1 |
| L1 | 当前固件 0.3 A,+100 rpm/3-5 s,一次短时点动 | §2 全绿、线序未变、监视/急停就绪、用户接受无保险丝残余风险 | 方向/状态/电流/电压/故障全部记录;任一异常回 L0/ESC |
| L2 | 保持 0.3 A,逐级改变速度或持续时间 | L1 记录由 CC 审核通过;CC 逐级指定目标和停机判据 | 每级单独审核回生抬压、Hall 分辨率、振动和温升;不得一次跳到高转速 |
| L3 | 改变 Iqmax/nominal current 或其他需固化参数 | 有保险丝/限流策略与新风险评估;Workbench 重生成、离线构建和差异审计通过 | 新 ELF 必须重新 hash、更新本文并重走全部 HITL |

`I_Q_REF`/`CURRENT_REF` 不是较低 Iqmax 的替代品:Speed mode 下 Iq ref 会被速度 PI 覆盖。当前 0.3 A 没有独立 runtime setter;任何上调/下调 Iqmax 都应回 Workbench 重生成。

## 附录 A. 精确 MCP 监视寄存器

### A.1 ID 规则

`register_interface.h` 定义完整 16 位 data ID:

```text
bits 15..6 = MotorPilot 目录 Id
bits 5..3  = data type (U8=0x08,U16/S16=0x10,U32/S32=0x18,RAW=0x28)
bits 2..0  = scope/motor;Motor 1 = 1
```

MotorPilot UI/JSON 中的 `Id` 只是目录 Id;下表 `M1 data ID` 才是线上 16 位值。MCP/STM32 使用 little-endian。现场优先使用 MotorPilot 符号名,不要手拼 UART 帧。

| MotorPilot 名 | 目录 Id/类型 | M1 data ID | 访问 | 显示单位/换算 | 本工程来源 |
|---|---|---:|---|---|---|
| `STATUS` | 1/U8ENUM | `0x0049` (73) | RP | 状态枚举 | `sync_registers.c:684-687` |
| `CONTROL_MODE` | 2/U8ENUM | `0x0089` (137) | RW | 3=`SPEED` | `sync_registers.c:184-201,690-693` |
| `BUS_VOLTAGE` | 22/U16 | `0x0591` (1425) | R | V,整数 | `sync_registers.c:775-778` |
| `HEATS_TEMP` | 23/U16 | `0x05D1` (1489) | R | degC,功率板温度 | `sync_registers.c:781-784` |
| `I_Q_MEAS` | 35/S16 | `0x08D1` (2257) | R | A;raw x current scale | `sync_registers.c:811-814` |
| `I_D_MEAS` | 36/S16 | `0x0911` (2321) | R | A;raw x current scale | `sync_registers.c:817-820` |
| `I_Q_REF` | 37/S16 | `0x0951` (2385) | RW | A;Speed mode 会覆盖写值 | `sync_registers.c:823-826` |
| `I_D_REF` | 38/S16 | `0x0991` (2449) | RW | A | `sync_registers.c:829-832` |
| `HALL_EL_ANGLE` | 59/S16 | `0x0ED1` (3793) | R | raw x `0.005493` deg | `sync_registers.c:859-863` |
| `HALL_SPEED` | 60/S16 | `0x0F11` (3857) | R | raw x `6` rpm | `sync_registers.c:866-870` |
| `FAULTS_FLAGS` | 0/U32FLAGS | `0x0019` (25) | RP | 低16 past,高16 current | `sync_registers.c:954-956` |
| `SPEED_MEAS` | 1/S32 | `0x0059` (89) | R | rpm | `sync_registers.c:959-962` |
| `SPEED_REF` | 2/S32 | `0x0099` (153) | R in MotorPilot | rpm | `sync_registers.c:965-968` |
| `MOTOR_CONFIG` | 1/RAW | `0x0069` (105) | RC | 含 `maxCurrent=0.3` | `mc_configuration_registers.c:53-62` |
| `APPLICATION_CONFIG` | 2/RAW | `0x00A9` (169) | RC | 含 nominalCurrent=0.3,maxSpeed=19806 | `mc_configuration_registers.c:43-50` |
| `SCALES` | 4/RAW | `0x0129` (297) | RC | voltage/current/frequency 三个 F32 | `mc_parameters.c:130-135` |
| `SPEED_RAMP` | 6/RAW | `0x01A9` (425) | RW | `[S32 rpm,U16 duration_ms]` | `sync_registers.c:470-478,1089-1096` |
| `CURRENT_REF` | 13/RAW | `0x0369` (873) | RW | `[S16 Iq,S16 Id]`;不等于 Iqmax | `sync_registers.c:492-500,1110-1118` |

当前 raw 电流换算来自 `65536 x 0.33 ohm x 1.53 / 3.3 V`,约 `10027 count/A`;MotorPilot `CURRENT_SCALE` 约 `9.973e-5 A/count`。`HALL_SPEED` 的频率比例为 `U_RPM/SPEED_UNIT=6 rpm/count`。MotorPilot 已按 `SCALES` 显示工程单位。

**限幅核验的边界:**不存在独立的 live `IQMAX` 寄存器。`MOTOR_CONFIG.maxCurrent=0.3` 与 `APPLICATION_CONFIG.nominalCurrent=0.3` 是只读静态配置旁证;真正限幅由编译期 `IQMAX_A=0.3`、`NOMINAL_CURRENT=3008` 进入速度 PI/STC。现场应同时核对 ELF hash和 Iq 遥测,不能把写 `I_Q_REF` 当成改 Iqmax。

### A.2 STATUS 数值

现场关键值:

| 值 | 名称 | 含义/动作 |
|---:|---|---|
| 0 | `IDLE` | 无当前/历史 fault 时才允许 Start |
| 4 | `START` | 启动过程;停留过久不重试 |
| 6 | `RUN` | 闭环运行 |
| 8 | `STOP` | 停止过程 |
| 10 | `FAULT_NOW` | 当前故障仍在;断母线/排因,不要 Ack 硬清 |
| 11 | `FAULT_OVER` | 当前故障已消失但历史未清;确认 NOW=0 后可 Ack 一次 |
| 16 | `CHARGE_BOOT_CAP` | 自举电容充电 |
| 17 | `OFFSET_CALIB` | 电流偏置校准 |
| 20 | `WAIT_STOP_MOTOR` | 等待电机停稳 |

完整 MotorPilot 6.4.2 枚举为 0..20:`IDLE,IDLE_ALIGNMENT,ALIGNMENT,IDLE_START,START,START_RUN,RUN,ANY_STOP,STOP,STOP_IDLE,FAULT_NOW,FAULT_OVER,ICLWAIT,ALIGN_CHARGE_BOOT_CAP,ALIGN_OFFSET_CALIB,ALIGN_CLEAR,CHARGE_BOOT_CAP,OFFSET_CALIB,CLEAR,SWITCH_OVER,WAIT_STOP_MOTOR`。

## 附录 B. FAULTS_FLAGS 位定义

`MCI_GetFaultState()` 将 `PastFaults` 放低 16 位、`CurrentFaults` 放高 16 位。因此同一故障低位表示“本轮发生过”,加 16 后表示“当前仍存在/NOW”。

| 低位/掩码 | 高位/NOW | MotorPilot 名 | 含义与现场动作 |
|---:|---:|---|---|
| bit0 / `0x0001` | bit16 / `0x00010000` | `MC_FOC_DURATION` | FOC 周期超时;Stop/断母线,交 CC 查低电感/30 kHz/执行时序 |
| bit1 / `0x0002` | bit17 / `0x00020000` | `MC_OVER_VOLT` | Vbus 过 15 V;立即断母线 |
| bit2 / `0x0004` | bit18 / `0x00040000` | `MC_UNDER_VOLT` | Vbus 低于 7 V;USB-only 预期,上母线后仍在则停 |
| bit3 / `0x0008` | bit19 / `0x00080000` | `MC_OVER_TEMP` | 功率板温度过阈;断母线冷却并查因 |
| bit4 / `0x0010` | bit20 / `0x00100000` | `MC_START_UP` | 启动失败;禁止自动重试 |
| bit5 / `0x0020` | bit21 / `0x00200000` | `MC_SPEED_FDBK` | Hall 速度反馈不可靠;断母线查 Hall/线序 |
| bit6 / `0x0040` | bit22 / `0x00400000` | `MC_BREAK_IN` | emergency/over-current 输入;断母线查功率级 |
| bit7 / `0x0080` | bit23 / `0x00800000` | `MC_SW_ERROR` | 软件错误;停止并保留日志 |
| bit10 / `0x0400` | bit26 / `0x04000000` | `MC_DRV_PROTEC` | PA11 combined DriverProtection;断母线,不能据此猜具体原因 |

bit8/9 及未列位在本工程无定义。Ack 只在 `STATUS=FAULT_OVER` 且 CurrentFaults=0 时真正生效;Ack 后必须确认 `STATUS=IDLE` 与 flags 清零。

## 附录 C. 精确控制写入项

| 操作 | MotorPilot | MCP base / M1 header | 格式/注意 |
|---|---|---:|---|
| 设 Speed mode | Control 下拉选 `Speed` | `SET_DATA_ELEMENT 0x08` / `0x0009` | data ID `CONTROL_MODE=0x0089`,U8 值 `3` |
| 设速度斜坡 | `Speed Ramp`:Target `+100`,Duration `3000..5000`,点 Execute | `SET_DATA_ELEMENT 0x08` / `0x0009` | data ID `0x01A9`;RAW size U16=`6`,随后 S32 rpm + U16 ms,little-endian |
| Start | 明确的 `Start` 按钮 | `START_MOTOR 0x18` / `0x0019` | 只有 IDLE 且 current/past fault 均为 0 才接受;随后读 STATUS |
| Stop motor | 明确的 `Stop` 按钮 | `STOP_MOTOR 0x20` / `0x0021` | 现场停止命令;不是刹车,仍要准备断母线 |
| Stop ramp | **首测/急停不用** | `STOP_RAMP 0x28` / `0x0029` | 只冻结斜坡并保持当前参考,不会停电机 |
| Start/Stop toggle | **首测不用** | `START_STOP 0x30` / `0x0031` | 行为取决于当前状态,不适合明确安全动作 |
| Fault Ack | Status 区 `Ack Faults` | `FAULT_ACK 0x38` / `0x0039` | 仅 FAULT_OVER 且 NOW=0 时有效;命令层总回 OK,必须读回状态 |
| 读取数据 | MotorPilot Watch/Plot | `GET_DATA_ELEMENT 0x10` / `0x0011` | payload 使用附录 A 的 M1 data ID |

`SPEED_RAMP=[+100,3000]` 的寄存器结构是 `rawSize=6`,`rpm=100`(S32),`duration=3000`(U16);`5000 ms`同理。本文不提供完整 UART transport frame,避免绕开 MotorPilot 的长度/校验/响应处理。

## 附录 D. 源码证据索引

- 生成配置上报:`GM16020-06_foc/Src/mc_configuration_registers.c:27-79`
- 默认速度、Iqmax、保护阈值:`GM16020-06_foc/Inc/drive_parameters.h:34-35,94-109`
- Hall 参数:`GM16020-06_foc/Inc/pmsm_motor_parameters.h:31-65`
- 寄存器类型/ID:`GM16020-06_foc/Inc/register_interface.h:44-65,112-147,202-246`
- 实际读写实现:`GM16020-06_foc/Src/sync_registers.c:167-500,671-1120`
- 高频 Hall/Iq/Id 可用项:`GM16020-06_foc/Src/hf_registers.c:113-167`
- current/frequency scale:`GM16020-06_foc/Src/mc_parameters.c:130-135`
- 故障掩码:`GM16020-06_foc/Inc/mc_type.h:94-102`
- fault packing/Ack 条件:`GM16020-06_foc/Src/mc_interface.c:620-656,850-878`
- MCP command 与分派:`GM16020-06_foc/MCSDK_v6.4.2-Full/MotorControl/MCSDK/MCLib/Any/Inc/mcp.h:29-48`;`GM16020-06_foc/Src/mcp.c:227-347`
- Hall 启动时先对齐当前速度再执行缓冲 ramp:`GM16020-06_foc/Src/mc_tasks_foc.c:284-293`
- 速度边界检查:`GM16020-06_foc/Src/speed_torq_ctrl.c:30,297-389`;配置 handle:`GM16020-06_foc/Src/mc_config.c:108-120`
- MotorPilot 6.4.2 register catalog:`C:/Program Files (x86)/STMicroelectronics/MC_SDK_6.4.2/Utilities/PC_Software/STMotorPilot/RegisterList/RegListSTMV2.json`
- MotorPilot Start/Stop/Speed Ramp UI:`.../STMotorPilot/GUI/MC_FOC_SDK.qml:174-205,778-880`
- MotorPilot Fault Ack UI:`.../STMotorPilot/mcqmllib/com/st/motorcontrol/qml/McStatusBox.qml:363-369`

## 11. 本草稿停止点

本文完成后仍停在“正式 FOC 已离线构建,尚未烧录/带电”的状态。下一步只能是:

1. CC 对本文做安全审核并修订/定稿;
2. 用户与 CC 现场选择是否接受无保险丝的残余风险;
3. 烧录前按 §4.2 单独 HITL;
4. 母线上电前再按 §6.2 单独 HITL。

在四项完成前,本文中的任何命令和操作都不得执行。
