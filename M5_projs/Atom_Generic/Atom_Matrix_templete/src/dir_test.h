// dir_test —— 电机旋转方向辨识 + 防翻保底
//
// 目的：从静止态 B 出发，用渐增的小电流脉冲驱动飞轮，借 IMU(pitch) 观察机体
//       转动方向，辨识出"使 pitch 减小(朝平衡)"对应的电流符号。
// 安全：全程内嵌角度看门狗。一旦方向错误使机体冲过危险角(越过 A/B 外棱)，
//       先尝试全力反向恢复；**若限时内仍无法回到静止态 A/B，则直接断电**
//       （setOutput(0)），防止飞轮空转烧毁/耗电。
// 形态：一次性阻塞执行（当前顶层测试程序）。需在 imuInit/motorInit 之后调用。
#pragma once

// 运行一次方向辨识（含保底）。结果与过程打印到串口。
void dirTestRun();

// 速度模式 authority 测试：逐级提高目标转速，对比电流模式的扭矩/摆幅。
//   需在 imuInit/motorInitSpeed(速度模式) 之后调用。含速度模式专用断电保底。
void speedModeTest();

// 上电(XT30 12-15V)谨慎起跳表征：电流模式，仅朝平衡、极小电流起步、脱阱即停、遇险只断电。
//   需在 imuInit/motorInit(电流模式) 之后调用。含 Vin/过压自检。
void poweredBreakawayTest();

// 电机台架能力测试：测 12V 下最大实际电流/最大飞轮转速，**不做起跳**（机体受硬线约束时用）。
//   交替方向短脉冲，仅看电机读数；含 Vin/过压/超速/侧向看门狗。需在 motorInit(电流模式) 后调用。
void motorBenchTest();

// ===== 起跳策略（可更换）=====
enum SwingUpStrategy {
    SWINGUP_CUBLI = 0,   // 蓄能急刹（反向蓄能→全力急刹反转飞轮）—— 默认（电流/力矩模式）
    SWINGUP_IMPULSE,     // 突然启动电机单次冲量（早期方案，逐级加大单次驱动）（电流模式）
    SWINGUP_PUMP,        // 莱洛三角震荡式蓄能（秋千式逐周泵能）—— 当前硬件不适用，留空
    SWINGUP_SPEED,       // 速度模式起跳 + 速度环"动量接住"缓落（研究用，整段跑速度模式）
};
extern SwingUpStrategy g_swingUpStrategy;          // 当前起跳策略（默认 SWINGUP_CUBLI）
void swingUpSetStrategy(SwingUpStrategy s);         // 切换策略

// 起摆 (swing-up)：自动识别 A/B → Vin 自检 → 按 g_swingUpStrategy 分派到对应策略实现。
//   全程安全保底。需在 imuInit/motorInit(电流模式) 后调用。
void swingUpTest();

// 供电充足性探测：缓升电流(<挣脱)平稳转飞轮、测顶速，对比 5V 基准(~600rpm)判 12V 是否生效。
//   机体姿态无关(翻倒也能测)；机体异动/过压即停。返回 true=供电充足(12V)。需在 motorInit(电流模式) 后调用。
bool powerDetectTest();

// 翻倒自救：机体已翻到正常范围外时，用大动量 Cubli 冲量两个方向各试一次，尝试磕回 ±34° 内。
//   放宽保底(仅过压/超速)。需在 imuInit/motorInit(电流模式) 后调用。
void recoverFlipTest();

// 起跳扭矩特征扫描：逐档增大参数，采"输入→机体响应(gy峰/Δθ)"曲线，供后续精细前馈标定。
//   directSweepTest(命令d)=突然加速(电流递增)；brakeSweepTest(命令k)=蓄能急停(蓄能转速递增)。姿态无关。
void directSweepTest();
void brakeSweepTest();

// 危险边界渐进探测：当前静止态 st(+1=B/-1=A)，朝外棱方向渐进加流找逼近翻倒的临界电流(带符号)。
//   速度钳+小偏移即停，绝不真翻。需在 imuInit/motorInit(电流模式) 后调用。
float dangerBoundaryProbe(int st);

// 固定启动表征序列：I²C自检 → 供电探测 → 识别A/B → 当前侧危险边界。
//   需在 M5.begin/imuInit/motorInit 之后调用。
void startupSequence();

// 辨识结果：使 pitch 减小(朝平衡)的电流符号，+1/-1；0=未知/未测出。
int dirTestRestoreCurrentSign();

// 姿态状态监视（流程终点态）：电机断电，每 1s 采样窗逐帧打印全部传感器，窗末用均值/抖动判定机体静止态。
//   只读 IMU/电机状态、绝不驱动。在 loop() 中反复调用即可持续监视。
void attitudeReportTick();

// 开机方向表征（decision 006 简化）：只判定+记录"使机体朝平衡起跳的飞轮**速度变化方向**"，然后终止
//   (断电+仅监视)。安全：超平衡40°/横向/超速即断电、不自救。需 motorInitSpeed 后调用。
void probeSwingUpDirection();
int  swingUpDirection();   // 表征结果：+1/-1（飞轮速度变化方向），0=未测出

// 探测"能起跳的扭矩"（system-model §6：作用量是扭矩）：逐增速度环力矩上限(maxCurrent≈扭矩)施加朝平衡，
//   找机体挣脱静止态阱起跳的最小扭矩 τ_break(并确认方向)。结果存 g_breakawayTorque/g_swingUpDir。需 motorInitSpeed 后调用。
void  probeBreakawayTorque();
float swingUpBreakawayTorque();   // 探测结果：能起跳的扭矩(mA当量)，0=未测出

// 电流模式 起跳+回落缓冲测试：朝平衡电流脉冲起跳 → 反角速度阻尼力矩缓冲落回静止态。需 motorInit 后调用。
void  swingUpCushionTest();

// 系统辨识激励实验（方案B）：已知电流台阶驱动→断电自由摆动，全速率记录(SID_DRV/SID_FREE)供离线拟合
//   I_b/Kt/mgl。MCU 只激励+记录、不拟合。需 motorInit(电流模式) 后调用。
void  sysIdExperiment();

// 正式起跳测试（decision 006）：一记起跳冲量 → 单次消能(回落同向/越过反向) → 断电滑行、观察落点。
//   只施加一次消能、不连续阻尼。安全：超平衡40°/横向/超速即断电、仅监视。需 motorInitSpeed 后调用。
void swingUpOneShotTest();

// 逐步起跳测试（decision 006）：起跳目标转速 SU 从小到大**逐轮渐增**，每轮强制一次消能+断电滑行+看落点，
//   打印每档落点并记最大安全回落档(lastSafeSU)。越平衡成功/翻越即停。需 motorInitSpeed 后调用。
void swingUpStepwiseTest();

// 起跳直接到平衡 + 保持平衡（decision 006）：识别A/B→实测起跳方向→起跳冲量送机体到平衡附近→
//   接管平衡控制器(balance.*，PID主用/LQR备用)钳 θ≈0。危险即断电、仅监视。需 motorInitSpeed 后调用。
void swingUpToBalance();
