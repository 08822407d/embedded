# 反作用轮倒立摆 开发 Playbook + 问题优先排查路径

> 用途：本仓库系统（单轴反作用轮倒立摆 / 自平衡三棱柱）开发中的**理论框架 + 成熟工程经验 + 踩坑排查表**。
> **遇到问题时先查第 5 节"问题→优先排查路径"**，再回到对应理论节。整理自公开文献/项目（见末尾来源），2026-05-30。
> 本项目实测细节见 [system-model](../system-model.md) 与 decisions/[003](../decisions/003-imu-filtering.md)/[004](../decisions/004-motor-power-and-mode.md)。

---

## 0. 一句话心法
反作用轮靠**牛顿第三定律**：电机给飞轮一个角加速度，飞轮就给机体一个**反向力矩**。
- 力矩 ∝ 飞轮角加速度（即电机**电流/力矩**）；机体能拿到的总冲量 ∝ 飞轮**动量变化**（受飞轮**转速上限**约束）。
- 一旦飞轮**转速饱和**（顶到 max rpm），角加速度→0，反作用力矩→0，**失去 authority**——这是头号敌人。

## 1. 系统架构与理论
- **建模**：拉格朗日方程 → 非线性动力学 → 在**直立不稳定平衡点线性化** → 状态空间。
  （或 CAD→Simscape Multibody→自动线性化，省手推。）
- **状态量（单轴 3 阶）**：机体角 θ、机体角速度 θ̇、飞轮角速度 φ̇（balance 控制需要这三个；θ̈/φ̇ 体现反作用耦合）。
- **两段式控制（共识）**：
  1. **起摆/起跳 (swing-up)**：从下垂/静止态把能量泵到接近直立。
  2. **交接 (handoff/catch)**：当 |θ|、|θ̇| 都进入小邻域，切换到平衡控制器。
  3. **平衡 (balance)**：直立点附近用线性状态反馈稳住。
  这是一个**混合/切换控制器**，常配带迟滞的有限状态机（如 tumbling↔stable）防抖动来回切。

## 2. 标准开发流程（按此顺序，逐层验证再往上叠）
1. **机械/参数**：装配；定出**质心位置**与平衡点角度（CoM 不在几何中心 → 平衡点有偏置，必须标）；估飞轮/机体**转动惯量**与惯量比。
2. **传感**：IMU 读数 → 零偏标定 → 滤波融合 → 拿到**可信的 θ 与 θ̇**（静止/运动下都可信）。
3. **作动**：电机**力矩(电流)控制**打通；**辨识摩擦**（库仑+粘滞，给前馈补偿）；飞轮转速/角度可读（编码器或电机回读）。
4. **建模/辨识**：线性化得状态空间；用台架实验辨识关键参数（力矩常数、惯量、摩擦）。
5. **平衡控制器**：先做**直立点附近的 LQR/状态反馈**（可手扶到直立点起控，先验证能稳住）。
6. **起摆控制器**：能量法把机体从静止泵到直立邻域。
7. **交接逻辑**：定义"何时算到位"（|θ|<阈 且 |θ̇|<阈）并切换；加迟滞。
8. **动量管理/退饱和**：平衡时让飞轮转速缓慢回零（消饱和），别把转速余量耗光。

## 3. 传感与数据处理（成熟做法）
- **角度估计**：加速度给**绝对角但动态下被线加速度污染**；陀螺给**动态/无滞后但漂移**。→ **互补滤波或卡尔曼融合**：陀螺主导动态、加速度慢校正。（本项目实测：飞轮一转，加速度角失真十几度，必须陀螺主导——见 [003](../decisions/003-imu-filtering.md)。）
- **陀螺零偏**：每次上电**静止现标**（取均值扣除），温度/上电会漂；标定时加"运动门"，被晃动则重标。
- **振动**：电机/飞轮高转速产生强振动，**会被采样混叠**进角速度（实测 ±80°/s），污染估计。
  对策：开**IMU 片上 DLPF**、**提高采样率/抗混叠**、必要时**陀螺陷波**（针对飞轮转频）。
- **飞轮转速**：编码器或电机回读；差分求速要滤波；注意**量化**噪声。
- **采样率/延迟**：平衡控制典型 ≥100Hz（10ms）起步，越快越稳；I²C 读延迟会吃掉相位裕度（本项目有 I²C 频率坑，见 [protocol](../protocols/esp32-i2c-frequency-caveat.md)）。

## 4. 控制方法细节
### 起摆 (swing-up)
- **能量法**：以"系统总能量 = 直立点势能"为目标，按 (θ, θ̇) **符号做 bang-bang / 速度梯度(EBSG)**：每个半周顺着运动方向加力，**像荡秋千一样逐周累积幅度**（共振泵能）。
- **Cubli 起跳**：把飞轮**加速储能到高转速，再急刹**，把动量一次性灌给机体直接跳起。
- 关键：泵能要**与机体自然摆动同步**；单次稳态力矩做不了大幅起摆。
### 平衡 (balance)
- **LQR / 全状态反馈**（3 态：θ, θ̇, φ̇）：u = −K·x，K 由线性化模型 + Q/R 权重求出；状态不可直接测时上**观测器**。
- 也有用 **PID/PD**（角度误差），或**串级**：外环角度→给定飞轮速度，内环控飞轮速度——**串级让系统对物理参数不敏感、好重调**。
- **平衡点偏置**：用实测平衡角作为 setpoint 零点（CoM 偏移、装配误差都体现在这）。
### 交接与退饱和
- **交接**：tumbling→stable 用 |θ̇| 迟滞判定，避免临界反复切。
- **退饱和**：平衡时给飞轮速度一个缓慢回零的偏置项，腾出双向 authority。

## 5. ★常见问题 → 优先排查路径（先看这张表）
| 症状 | 最可能原因（按概率排序） | 排查/对策 |
|---|---|---|
| **推不动 / 摆幅极小 / 起不来** | ① 电机**供电电压不足**(authority 不够) ② 飞轮**转速饱和** ③ 飞轮惯量太小 ④ 摩擦/静摩擦太大 | 先量 **Vin 与实际电流/转速**；对比规格；本项目曾因 Grove 5V 致 ~600rpm 上限，升 12V 后 ~1900rpm（[004](../decisions/004-motor-power-and-mode.md)） |
| **角度读数动态下乱跳/误判** | ① 加速度角被**线加速度/振动**污染 ② 没做陀螺融合 ③ 高转速**振动混叠** | 改**陀螺主导互补滤波**；开片上 DLPF；提采样率（[003](../decisions/003-imu-filtering.md)） |
| **静止也缓慢漂** | 陀螺**零偏**没标/温漂 | 上电静止现标零偏，运行中扣除 |
| **一个方向能控、反方向发抖/发散** | 电机**正反不对称**（BLDC 无主动刹车）；力矩死区不对称 | 用**力矩(电流)模式+主动刹车**；必要时换能双向主动出力的作动方式 |
| **平衡点不在 0°/总往一边偏** | **质心不在几何中心**，平衡点有偏置 | 实测平衡角作 setpoint 零点；标定 CoM |
| **接近平衡却突然失控** | 飞轮**转速饱和**，失去 authority | 加**退饱和**(平衡时飞轮速度缓回零)；增大飞轮惯量 |
| **小幅抖动/极限环** | D 项过大/噪声/采样太慢/延迟大 | 降噪、提采样率、减 I²C 每周期操作、重调增益 |
| **力矩命令大但实际电流小** | 飞轮**空载被 back-EMF 限流**(正常)；或**供电压不足** | 区分空载(电流本就小)vs带载；看 Vin 是否被压低 |
| **起摆泵不上能量** | 泵能**相位没对准**自然摆动；阻尼太大；authority 不足 | 用 (θ,θ̇) 符号定时；先确认单次脉冲能观测到机体响应 |
| **机械/走线干扰运动** | 供电/信号线太硬产生额外力矩、限制摆动 | 柔性细线、理线减力臂、**滑环**（reaction-wheel 项目常用） |

## 6. 本项目映射（已落地/待办）
- ✅ 传感：MPU6886，已做**陀螺主导互补滤波(pitch=gy / lateral=gx)** + 上电零偏标定 + 运动门（[003](../decisions/003-imu-filtering.md)）。
- ✅ 作动：RollerCAN(D3504 200KV) **电流模式**；模式 init 一次性定、运行不切（[002](../decisions/002-motor-control-strategy.md)）。
- ✅ 供电：Grove 5V 不足 → XT30 接 12V，转速/电流 ~3×（[004](../decisions/004-motor-power-and-mode.md)）。
- ⏳ 待办：抗混叠(片上 DLPF/提 ODR)；摩擦辨识；线性化建模 + 平衡 LQR；能量法起摆；交接迟滞；退饱和；走线（硬线干扰）。
- 单轴特性：被控量 pitch，静止态 B(+30°)/平衡(0°)/A(−31°)；侧向(垂直可控轴)无 authority，超限不可恢复（安全保底已实现）。

## 7. 来源
- ETH Cubli（3D 反作用轮倒立摆，起跳/平衡）：[ECC2013 PDF](https://ethz.ch/content/dam/ethz/special-interest/mavt/dynamic-systems-n-control/idsc-dam/Research_DAndrea/Cubli/Cubli_ECC2013.pdf) ｜ [IDSC 项目页](https://idsc.ethz.ch/research-dandrea/research-projects/archive/cubli.html)
- 专著 Block / Åström / Spong, *The Reaction Wheel Pendulum*（建模/状态反馈/摩擦/饱和的权威）：[Springer PDF](https://link.springer.com/content/pdf/10.1007/978-3-031-01827-5.pdf)
- 起摆策略综述：[Swing-up control strategies for a reaction wheel pendulum](https://www.researchgate.net/publication/220665740_Swing-up_control_strategies_for_a_reaction_wheel_pendulum)
- LQR 多体建模：[Multibody Modeling and Balance Control of a RWIP Using LQR](https://www.researchgate.net/publication/351314795_Multibody_Modeling_and_Balance_Control_of_a_Reaction_Wheel_Inverted_Pendulum_Using_LQR_Controller)
- 大学实验流程（含摩擦/速度估计/3 态反馈/观测器）：[UIUC ECE486 RWP](https://courses.grainger.illinois.edu/ece486/fa2020/laboratory/docs/Final_Project/fp_overview.htm)
- 业余项目实战经验（IMU 滤波/PD 串级/饱和/惯量比/电机选型）：[Charles' Labs — Reaction Wheel Attitude Control](https://charleslabs.fr/en/project-Reaction+Wheel+Attitude+Control) ｜ [Willem Pennings — Balancing cube](https://willempennings.nl/balancing-cube/)
