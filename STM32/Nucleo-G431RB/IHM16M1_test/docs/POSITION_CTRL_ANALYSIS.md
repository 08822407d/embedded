# 位置控制可行性分析 —— "把官方 FOC 改成位置模式"能不能做、怎么做

> 定位:回答用户 2026-07-16 的问题"如何将官方 FOC 改为位置模式控制(精度可接受到整圈数)"。
> 结论先行:**官方 MCSDK 位置模式对本套"仅霍尔"硬件被 ST 明确堵死**;要"位置控制"有三条路,按你"整圈数"容差,推荐**应用层数圈粗控(路线 B)**。
> 关联:[`FACTS.md`](FACTS.md) F-38(本文事实来源)、F-24(电机无编码器)、F-37(首次带电成功、低速下限 ~500rpm)、DECISIONS #003(霍尔低速难点)。

---

## 0. 一句话结论
- **当前固件**:只有速度/力矩控制,**根本没编译位置控制**(源码核实)。无论走哪条路都要重生成或加代码。
- **官方位置模式**:MCSDK 6.x 的 Position Control **要求正交编码器(quadrature encoder)为主速度传感器,霍尔因分辨率过低不被允许**——WB 在霍尔主传感器下**不出现**位置模式选项。这是 ST 的硬约束,不是配置勾一下的事。
- **本电机 GM16020-06 无编码器轴** → 官方路线要先**加装编码器**。
- **"整圈数"精度**:不需要官方位置模式,**应用层用霍尔数圈**即可,不加硬件。

---

## 1. 证据链(为什么这么判)

### 1.1 当前固件没有位置控制(源码核实)
| 检查点 | 结果 | 位置 |
|---|---|---|
| 默认控制模式 | `DEFAULT_CONTROL_MODE = MCM_SPEED_MODE` | `Inc/drive_parameters.h:97` |
| 位置控制器实例 | **无** `PosCtrl_Handle_t`/`TC_Init` 调用 | `Src/mc_config.c`、`Src/mc_tasks_foc.c`(只有 `STC_Init(pSTC[M1],&PIDSpeedHandle_M1,&HALL_M1._Super)`) |
| 命令分支 | 只处理 `MCM_SPEED_MODE` / `MCM_TORQUE_MODE` | `Src/mc_interface.c:710-719`,**无** `MCM_POSITION_MODE` case |
| 位置环 PID 参数 | `.c` 中搜不到 `POSITION_KP/KI/KD` | 全工程 grep 空 |
| 样板残留 | `MCM_POSITION_MODE` 枚举、`trajectory_ctrl.c/h`、`MC_REG_POSITION_*` 寄存器**存在但未接入** | MCLib 固定附带,非本构建使用 |

→ 结论:**必须回 WB 重生成(或手加代码),现固件不可能测位置。**

### 1.2 官方位置模式 = 编码器专属(ST 确认 + 代码印证)
- **ST 官方/社区**(WebSearch 2026-07-16):
  - "In MotorControl Workbench 6.3, to select **position control mode, the motor must provide an Encoder** sensor and the Encoder must be selected as the **main** Speed Sensing Selection."
  - "**Position Control Mode can only be used with Quadrature Encoder sensor; Hall sensors cannot be used due to low positioning resolution.**"
  - 霍尔只能作 auxiliary,不能作位置模式的主传感器。
- **代码印证**(`MCSDK.../MCLib/Any/.../trajectory_ctrl.{h,c}`):
  - `void TC_Init(PosCtrl_Handle_t*, PID_Handle_t* pPIDPosReg, SpeednTorqCtrl_Handle_t* pSTC, ENCODER_Handle_t* pENC)` —— 位置控制器**签名里就带编码器句柄**。
  - 句柄含 `ENCODER_Handle_t *pENC`;对齐流程 `TC_EncAlignmentCommand`/`TC_EncoderReset` 用**编码器 Z 索引**做绝对零位对齐(`TC_ABSOLUTE_ALIGNMENT_NOT_SUPPORTED = "Encoder sensor without Z output"`)。
  - ⚠️ **微妙点**:位置**反馈**读的是 `SPD_GetMecAngle(STC_GetSpeedSensor(pHandle->pSTC))`(`trajectory_ctrl.c:300/431/498`),这是 `SpeednPosFdbk` **基类虚方法,霍尔也实现了**。所以"反馈来自霍尔"技术上成立;编码器句柄主要用于对齐。这给"魔改"留了缝(见路线 C),但**不改变官方工具链的封锁**。

### 1.3 硬件现状
- 电机 **GM16020-06 无编码器**(F-24:霍尔 5 线 + 三相,无编码器线)。
- IHM16M1 有霍尔/编码器接口 J1(F-19),但当前霍尔占用 TIM3(PC6/7/8);编码器一般复用同一定时器的编码器模式 → **霍尔与编码器通常二选一**(需重新分配定时器/引脚,待重生成时由 WB 决定)。
- ⚠️ 待核实:IHM16M1 的 J1 是否同一物理口既接霍尔又接编码器、编码器走哪个定时器——**若走路线 A 再核**,现在不阻塞结论。

---

## 2. 三条路线对比

### 路线 A —— 加正交编码器,走官方位置模式(精度高,要硬件)
- **做什么**:给电机轴装正交编码器 → 接 IHM16M1 J1 → WB 里主传感器改**编码器** + 启用 **Position Control** → 重生成 → 重构建 → 重烧 → 新一轮带电 bring-up。霍尔可保留作 aux 或用于启动。
- **精度**:编码器 CPR 决定,可做到远细于"整圈"的角度定位与**位置保持**。
- **代价/风险**:
  - 需**采购+机械加装**编码器;这颗 16mm 微型高速电机(空载 18200rpm)轴 1.5mm,**机械上加编码器不一定容易**,可能改变机械结构。
  - 编码器 + WB 主传感器切换 = 相当于**重做一遍传感器标定/换相对齐**(编码器对齐,非霍尔 58°)。
  - 引脚/定时器资源重排,须重核 `.ioc`(不得沿用记忆引脚,注2)。
- **何时选**:目标应用**真的需要细角度定位或静止保持**时。

### 路线 B —— 应用层"数霍尔圈"粗控(匹配"整圈数",不加硬件)★推荐
- **做什么**:**保持现在的霍尔速度 FOC 固件**,在应用/固件层加"数圈"逻辑:
  - 用已有可读量 `MC_REG_CURRENT_POSITION`(32bit 机械角累计)或霍尔机械角,**累计转过的圈数**;
  - 下发速度斜坡让电机在**可运行区(≥~500rpm)**旋转,到达目标圈数附近**减速→停**(`SPEED_RAMP` 到 0 / `MC_StopMotor`)。
- **精度**:能满足"**整圈数**"(转 N 圈);**但**——
  - 停位受 F-37 低速难题影响:不能爬到精确角度平稳停,末端会有**过冲**;
  - **无位置保持**(停后靠 cogging/摩擦驻留,外力可推动)。
- **代价/风险**:纯软件,无新硬件;是**应用逻辑**,不是 MCSDK 位置模式。停位精度到"某一整圈±部分圈",不到"精确角度"。
- **何时选**:你说的"精度可接受到整圈数" → **正是这条**。

### 路线 C —— 魔改霍尔进位置环(不推荐)
- **做什么**:绕过 WB 限制,手动把 `trajectory_ctrl` 或自写位置 PID 接到霍尔 `SPD` 句柄(反馈路径技术可行,见 1.2)。
- **问题**:要**手改生成代码**(每次 WB 重生成被覆盖,须维护补丁,见 [`GITIGNORE_AND_REGEN.md`](GITIGNORE_AND_REGEN.md));绕不过霍尔 **60° 分辨率 + 低速难点**;编码器对齐假设要拆改;**脱离官方支持、调试成本高**。
- **何时选**:既要"官方位置模式的轨迹/PID 框架"又坚决不加编码器,且能接受粗分辨率与维护负担——一般不值得,**优先 B**。

---

## 3. 推荐
按你明确的容差(**整圈数可接受**)与不加硬件的现状:**走路线 B(应用层数圈粗控)**。
- 它用现有霍尔速度 FOC,**不需重生成 WB / 不需编码器**,风险最低;
- 若后续应用发现**需要细角度或静止保持** → 再上**路线 A(加编码器)**,这是唯一能给"真·位置控制"的路。
- **不建议路线 C**。

---

## 4. 下一步(待用户拍板)
需要你选方向,因为三条路后续工作完全不同:
- 选 **B** → 我把"数圈粗控"写成**需求 + codex 任务书**:定义命令接口(转 N 圈/到某圈)、圈数统计来源(`MC_REG_CURRENT_POSITION` vs 霍尔角累计,需先离线核该寄存器在速度模式下是否有效计数)、减速停策略、过冲上限、验收判据;实现由 codex 做,现场验证走带电 HITL。
- 选 **A** → 先离线**核实 IHM16M1 编码器接口/定时器 + 选型编码器**,再 WB 重配(用户 GUI)+ 重生成 + codex 重构建 + 新 bring-up runbook。
- 选 **C** → 我评估补丁范围与 regen 维护方案后再定(不推荐)。

> 红线不变:任何重生成=用户 GUI;任何烧录/带电=新的 HITL-FLASH + HITL-POWER 授权 + 母线断电烧录 + 值守带电([`FOC_BRINGUP_RUNBOOK.md`](FOC_BRINGUP_RUNBOOK.md))。本文纯离线分析,未碰硬件。
