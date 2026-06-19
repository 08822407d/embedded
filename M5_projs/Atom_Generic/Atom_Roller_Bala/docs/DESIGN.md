# DESIGN — 架构与当前设计

> CC 产出;Codex 据此实现。描述"系统怎么搭、当前打算怎么做",随设计演进更新(始终反映当前意图)。
> 进度:物理模型框架已立、RollerCAN 接口已摸清;控制律待细化。

## 1. 系统概念
平面反作用轮倒立摆(RWIP)。机体绕底部楔形棱(对地唯一接触)在竖直平面内单自由度转动;RollerCAN 驱动内部飞轮,用其反作用力矩把机体倾角 θ 稳定在 0(中轴竖直,不稳定平衡点)。
侧视几何:`docs/assets/balance-geometry-sketch.png`。

## 2. 物理模型(框架,参数待补)
- **广义坐标**:机体倾角 θ(相对竖直,平衡点 θ=0;|θ|≈30° 为恢复区边界,见 REQUIREMENTS §4);飞轮角度 / 角速度 φ、φ̇。
- **作动**:RollerCAN 输出力矩 τ,经飞轮反作用作用回机体。
- **模型结构**:标准 RWIP 双自由度 —— 机体绕支点摆动方程 + 飞轮转动方程,经反作用力矩耦合。参数(m、质心距 l、机体 I_b、飞轮 I_w、τ_max)待测量 / 查规格后填。
- **平衡点性质**:θ=0 不稳定;拟在其附近线性化用于设计稳定控制器。
- **理想 vs 真机**:上述角度为标称值;真机经 IMU 解算有微小偏差(零偏 / 安装倾角 / 公差)。真正的平衡设定点(θ=0)、静置与跌倒判定阈值须在测试中**标定并留裕度**,不照搬标称值。
- **静置态**:θ 到 ±30°(距竖直)时机体躺到一个斜面上(中轴与桌面 60°,F‑8),静止且稳定;反作用轮无法从此自起;有两个(左 / 右面)。
- **术语约定(与用户)**:
  - **恢复区**:平衡控制器能把机体拉回 θ=0 并稳住的状态集合(看 θ 与 θ̇);几何外界 |θ|<30°(距竖直),真实范围是其中受力矩 / 飞轮转速限制的**子集**。
  - **静置态**:机体躺在一个斜面上(|θ|=30° 距竖直)的稳定静止态 —— 原拟称"陷阱态",改名避免中文歧义。
  - **起跳(swing-up)**:从静置态主动甩起、越楔棱回到恢复区的机动。

## 3. 运行模式 / 状态机(初稿)
- `IDLE`:上电、自检、等待启动。
- `BALANCE`:恢复区内闭环自稳(核心)。
- `SWING_UP` / 起跳(可选):从静置态起跳、回到恢复区。
- `FALLEN`/`SAFE`:判定跌出恢复区 → 飞轮安全停转,进入安全态。
- (远控 / 调参如何介入这些状态待定。)

## 4. 软件架构(初稿,模块均优先用官方库)
| 模块 | 职责 | 库 / 接口 | 代码(待建) |
|---|---|---|---|
| imu | 读 MPU6886、估计 θ 及角速度 | `M5Atom` 的 `M5.IMU` | `src/imu.*` |
| motor | 封装 RollerCAN:使能、设模式、发指令、读反馈 | `M5Unit-Roller` 的 `UnitRollerI2C`(I2C) | `src/motor.*` |
| control | 平衡控制律(θ→τ)、限幅、跌倒判定 | — | `src/control.*` |
| state | 状态机(IDLE/BALANCE/.../SAFE) | — | `src/state.*` |
| telemetry | 经 USB 串口输出状态 / 遥测,供主机查看与调参 | `Serial`(USB Type-C) | `src/log.*`(或并入 main) |
| led | 状态指示 | FastLED | `src/led.*` |
| main | 初始化 + 主循环调度 | `M5Atom` | `src/main.cpp` |
（模块划分待定稿,先占位。)

## 5. 关键接口 / 数据结构

### 5.1 RollerCAN —— `M5Unit-Roller` 库,I2C 类 `UnitRollerI2C`
来源:`github.com/m5stack/M5Unit-Roller`,`src/unit_rolleri2c.hpp`(MIT)。本项目走 I2C。

- **初始化**:`begin(TwoWire* = &Wire, addr = I2C_ADDR, sda = 21, scl = 22, speed = 4 MHz)`。
  - ⚠️ Atom 走 Grove 口,SDA/SCL **不是默认 21/22**(ATOM 常见 G26=SDA、G32=SCL),`begin()` 须显式传 Atom 的脚 —— **待核实**;4 MHz 在 Grove 线长上能否稳定也需验证。
- **控制模式**:`setMode(roller_mode_t)` / `getMotorMode()`,值 **1=速度,2=位置,3=电流,4=编码器**(枚举在 `unit_roller_common.hpp`)。平衡用哪种 —— 待后续研究。
- **作动 setter**:`setOutput(en)` 上/下电;`setSpeed()` / `setPos()` / `setCurrent()`;限流 `setSpeedMaxCurrent()` / `setPosMaxCurrent()`;PID `setSpeedPID()` / `setPosPID()`;堵转保护 `setStallProtection()`。电流量程约 ±120000(单位/标度待核实)。
- **可读参数(getter)**:
  - 运动反馈:`getSpeedReadback()`、`getPosReadback()`、`getCurrentReadback()`(均 `int32`);命令值 `getSpeed/getPos/getCurrent()`;编码器 `getDialCounter()`。
  - 健康 / 状态:`getVin()` 输入电压、`getTemp()` 温度、`getSysStatus()`(0 待机/1 运行/2 错误)、`getErrorCode()`(0 无/1 过压/2 堵转)、`getOutputStatus()`(0 关/1 开)。
  - 配置回读:`getSpeedPID/getPosPID`、`getRGB*`、`getMotorID`、`getBPS`、`getStallProtection`、`getPosRangeProtect`、`getFirmwareVersion`、`getI2CAddress`、`getCalBusyStatus`。
  - ⚠️ 速度 / 位置 / 电流的物理标度(单位换算)需按官方文档或实测标定。

### 5.2 IMU / 姿态
- 板载 MPU6886,经 `M5Atom` 的 `M5.IMU` 读加速度 / 陀螺;θ 估计(互补滤波 / 卡尔曼)算法待定。

### 5.3 控制环数据流(待细化)
`M5.IMU` → θ 估计 → control(θ→τ 命令)→ `UnitRollerI2C` setter;同时读 `getXxxReadback()` 做反馈与飞轮卸荷。

## 6. 控制律
（待定:先 PID 还是 LQR?平衡环 + 飞轮卸荷(防转速饱和)。控制模式选型确定后再定。)

## 7. 待 Codex 实现的任务拆解
（待接口/模式钉死后再拆。预备第一批:① I2C 打通 `UnitRollerI2C`——使能 + 发指令 + 读反馈;② `M5.IMU` 读 θ;③ 最小平衡环。)
