# Codex 任务书 — 整圈数位置粗控 PoC(PC 侧 MCP 工具,零固件改动;离线开发 + 分级 HITL 实测)

> 给 Codex:读完即可自主执行。**全程不烧录、不改固件、不改生成树**——板上保持 F-36/F-37 已验证的 0.3A/100rpm FOC 固件(ELF SHA `0036DEE8…092DB0`),你开发的是 **PC 侧工具**。任何连板/上电/转电机步骤都有分级硬闸门,见 ⛔ 红线。
> 项目根 = `C:\Users\cheyh\Desktop\embedded\STM32\Nucleo-G431RB\IHM16M1_test`。**动手前先读**:[`FACTS.md`](FACTS.md)(F-19/23/24/28/30/35/36/37/38/39)、[`POSITION_CTRL_ANALYSIS.md`](POSITION_CTRL_ANALYSIS.md)、[`FOC_BRINGUP_RUNBOOK.md`](FOC_BRINGUP_RUNBOOK.md)(精确寄存器 ID/格式/STATUS 枚举/安全闸/急停判据)、[`DECISIONS.md`](DECISIONS.md) #006/#007、[`REQUIREMENTS.md`](REQUIREMENTS.md) §6、[`OPERATIONS.md`](OPERATIONS.md) §0、[`SYNC_WORKFLOW.md`](SYNC_WORKFLOW.md)。

## 目标(对应 REQUIREMENTS §6 / DECISIONS #007-B)
不改固件,用 PC 侧工具经 MCP(USART2@1843200)实现并验证:**命令电机旋转 N 个整机械圈后停止**;统计停位误差(单位=圈),给出"提前量(lead)标定值 + 可达精度"结论。是否满足应用由用户判定。

## ⛔ 红线(分级,必守)
- **全程**:不烧录/不写 flash、不改 `GM16020-06_foc/` 生成树、不重生成;0.3A 是软件限幅**非短路保护**、无保险丝(#006-B 残余风险不变)。
- **§3 手转验证段**:允许 USB-only 连板 + **只读**寄存器;**母线必须物理断开**(用户确认);**禁止发送 Start/SPEED_RAMP/任何控制写入**;转子只由用户手转。
- **§4 带电试验段**:必须用户按 runbook 式 **HITL-POWER 重新授权** + 在场值守 + 可 1 秒断母线 + CC 旁站;你不得自行开始;每轮试验前 Ack 故障、确认 STATUS。
- 急停判据沿用 runbook:尖啸/剧抖/焦味/电机移位/Iq 顶格持续/反复 fault → 立即 Stop + 断母线,记录现场,`ESC` 交回 CC。
- 有疑/受阻/发现文档与实物矛盾 → `ESC` 交回 CC,别硬闯、别自行放宽。

## §0 已确认事实(直接引用,勿重查)
- **MCP**:USART2 PA2/PA3 @ **1843200**;COM 口**按 SN `002A00403234510E33353533` 现场扫**(OPERATIONS §0),别写死;串口**单客户端**——你的工具与 MotorPilot 互斥,试验前确认 MotorPilot 已关闭。
- **观测(F-39)**:`MC_REG_CURRENT_POSITION` 零实现**不可用**;唯一角度量 = `MC_REG_HALL_EL_ANGLE`(s16 电角度,只读,每**电气**圈回绕)。**4 对极(F-23/24)→ 4 电气圈 = 1 机械圈**,即机械圈数 = 解回绕累计电角度 / (4×65536 counts)。
- **控制与状态**:`SPEED_RAMP=[S32 rpm, U16 ms]`、Start/Stop/Fault-Ack 命令、`STATUS`(IDLE=0/RUN=6/FAULT_NOW=10/FAULT_OVER=11)、`FAULTS_FLAGS`、speed ref/meas、Iq/Id、Vbus —— **精确 ID/帧格式/下发方式已在 [`FOC_BRINGUP_RUNBOOK.md`](FOC_BRINGUP_RUNBOOK.md) 附录落盘,直接引用**,不要凭记忆再造。
- **运行域(F-37)**:低速下限 ~500rpm(更低无法启动/维持);首测目标速度从 **500rpm** 起步;上限本 PoC ≤1000rpm(更高须 CC/用户另批)。
- **§3 的物理基础(F-19)**:霍尔 5V 由 Nucleo(USB)供给——**母线断开时手转转子,霍尔与角度寄存器预期仍有效**;板上无自动 Start,上电初始 IDLE(欠压故障 Ack 后)。此预期若实测不成立(手转时角度不更新)→ 如实记录并 `ESC`(可能被迫提前走固件侧方案,由 CC 重新设计)。

## §1 方案选型(先做,选型结论落盘后再动手实现)
**数据通路**,候选两条(也允许你提出更优第三案,但必须完整记录方案与理由供 CC 复盘):
1. **MotorPilot 内置脚本/automation**(若 6.4.2 有此能力,查 `MC_SDK_6.4.2` 安装目录与 MotorPilot 资源):优点是零协议开发;**必须实测可达采样率**,不达 §2 要求即弃。
2. **独立 MCP/ASPEP 最小客户端**(Python 优先;先核本机 Python 可用性,无则报环境需求给用户/CC 再定):按本机 MCSDK 文档(`MC_SDK_6.4.2/Documentation/`)与 MotorPilot 资源(如 `RegListSTMV2.json`、QML/JS 源)实现握手 + 读/写寄存器 + 命令帧;可控性最好。
- **优先调研 MCP 异步高频遥测通道**(生成工程含 `hf_registers.c`;MotorPilot 示波器即用此机制):若能订阅 `HALL_EL_ANGLE` 高频流,解回绕裕量大增,优于慢速轮询。
- **解回绕约束(必须写进工具与选型判断)**:电角度回绕率 = `4×rpm/60` Hz;采样率必须 > 2× 回绕率,**设计余量 ≥4×**(含 Windows 串口抖动);相邻样本 |Δ|>32768 counts 视为可能丢圈 → 标记 invalid 并中止本次计数。参考值:500rpm→回绕 33.3Hz→采样 ≥133Hz;1000rpm→66.7Hz→≥267Hz。工具须记录时间戳并输出采样率统计。

## §2 工具实现(纯离线)
- 代码放 **`tools/revcount_poc/`**(新受控目录,正常入 git;**不在**被忽略的生成树里)。附 README:用法/参数/依赖。
- 功能:连接/握手;轮询或订阅 `HALL_EL_ANGLE`;解回绕累计 → 机械圈数(浮点);同时监视 STATUS/FAULTS/speed;命令序列 **"Ack→Start→ramp 至 v_test→计圈至 (N−lead)→执行停→记录 final 圈数与误差"**;全程 CSV 日志(时间戳、原始角、累计圈、STATUS、speed)。
- 停止策略**参数化**:`lead`(提前量,圈)、停法(先试 `SPEED_RAMP→0(1000–2000ms)` 后 Stop;Stop 直停作对照)、`v_test`。首版默认保守:v_test=500rpm。
- **离线自测**:用合成角度序列(含回绕/抖动/丢样/反向)单元测试解回绕与计圈逻辑;串口层用 mock/回环。自测通过才许连真板。

## §3 USB-only 手转计圈验证(用户协助;无 HITL-POWER,因不带电)
- 前置:用户确认**母线物理断开**;按 SN 扫 COM;连接后读 STATUS(预期含欠压 FAULT/IDLE,属正常)。**本段只读,零控制写入。**
- 用户手转转子整 K 圈(建议 K=3~5,正、反向各测多次;可装回 3D 打印指针辅助对齐起止——装不装由用户定,慢速手转无风险);工具计圈应与 K 一致。
- **通过判据**:多次试验 |计圈 − K| ≤ **0.25 圈** 且无 invalid;同时记录实际采样率是否达 §1 余量要求(按 §4 目标转速折算)。
- 不通过 → 回 §1/§2 修(采样率不足 → 换高频通道或降 §4 目标转速);修不动 → `ESC`。

## §4 带电 N 圈停止试验(HITL-POWER + 用户值守 + CC 旁站)
- **闸门**:开始前逐项复述 runbook §1 安全闸(电机固定、母线可 1 秒断、人在场、指针处置——≤1000rpm 下装/拆由用户定,建议先拆,标定 lead 后再议)+ 取得用户明确授权。MotorPilot 关闭、串口由你的工具独占。
- 每轮序列:Ack 残留故障 → Start → ramp 至 v_test → 计圈至 N−lead → 执行停 → 等静止(speed≈0 且角度不再变)→ 记录 final 圈数、误差、从发停到静止的**滑行圈数**。
- **试验矩阵**:v_test ∈ {500,(可选 800/1000)} rpm;N ∈ {5, 20, 50};每组 **≥5 次**。先用第一组实测滑行圈数**标定 lead**,再跑其余组验证误差分布。
- 输出:误差统计(均值/最差/std,按组),结论 = "lead 标定值 + 各速度下可达精度(圈)"。**是否满足应用由用户判定**(REQUIREMENTS §6),你不替用户拍板。
- 任何异常/越界 → 急停(红线)、记录、`ESC`。

## §5 交付与落盘
- `tools/revcount_poc/`(工具 + README + 自测);
- **`docs/REVCOUNT_POC.md`**:选型与理由(含被否方案)、协议要点(引用 runbook,不重复造)、解回绕设计、§3/§4 全部数据与统计、结论与遗留问题;
- 新实测事实(实际采样率、滑行圈数、精度统计)→ `FACTS.md` 新条目;`STATUS.md` 更新进度;
- **收尾 = commit + push**(SYNC_WORKFLOW);中途换阶段也先 commit。

## §6 纪律
- §1 选型与停止策略你有自主设计空间,但**所有方案/取舍必须落盘**,让 CC 能完整复盘;硬件红线不因方案创新而放松。
- 结论须有实测/源码证据;别写死 COM 号;Windows 路径含空格加引号;工具默认参数必须是保守值(500rpm、小 N)。
- 你只执行到"数据与统计",安全与验收判定归 CC/用户。
