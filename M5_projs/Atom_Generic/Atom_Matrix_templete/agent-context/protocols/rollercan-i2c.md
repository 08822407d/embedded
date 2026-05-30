# RollerCAN 电机 —— I²C 控制（协议 + 官方库 API）

> 反作用轮电机的硬件接线、I²C 协议要点与官方库用法。控制策略见 [decisions/002](../decisions/002-motor-control-strategy.md)。
> 来源：M5 官方库 `m5stack/M5Unit-Roller`（源码）+ Unit-RollerCAN I2C Protocol PDF。整理日期 2026-05-29。

## 硬件 / 接线
- 模块：**M5Stack Unit RollerCAN**（**D3504 200KV** 无刷电机 + 编码器 + FOC），支持 **CAN / I²C** 两种总线，**本项目用 I²C**。
- 接 ATOM **Grove 口**：**SDA = G26，SCL = G32**。I²C 频率 **TBD（暂定 ~200kHz）**：
  **100kHz 和 400kHz 均为危险频率、勿用**，见 [esp32-i2c-frequency-caveat.md](esp32-i2c-frequency-caveat.md)。
- **默认 I²C 从机地址 = `0x64`**（`#define I2C_ADDR (0x64)`），本项目用默认地址。
- 与 ATOM 内部 IMU(MPU6886) 的 I²C 是不同组，互不冲突。

## ⚡ 供电（关键！决定电机力矩/转速）—— 来源：m5-docs 规格表，2026-05-30 实测印证
- **两路供电**：**CAN(XT30) 口 6–16V DC**（电机动力）／**Grove 口 5V**（逻辑，自动调功率系数）。
- **力矩随电压差 3 倍**：**5V Grove → 0.021 N·m @350mA**；**16V CAN → 0.065 N·m @927mA**。
- 16V 负载表：无载 78mA；50g→2100rpm/225mA；200g→1400rpm/601mA；500g(满)→560rpm/918mA。
- **过压保护**：> **18V** 报 E:1「Over Voltage」、电机停、LED 蓝。上限 16V，**勿超**。
- **本项目现状坑**：只接 Grove，实测 Vin≈4.5V → 电机仅 ~0.021N·m、飞轮顶 ~600rpm、机体推不动(不脱阱)。
  **解决：给 XT30 接 12–16V DC（建议 12–15V），Grove 仍接做 I²C 通信。** 详见 [decisions/004](../decisions/004-motor-power-and-mode.md)。

## 官方库
- 仓库：`https://github.com/m5stack/M5Unit-Roller.git`（PlatformIO 注册表查不到，**用 git URL** 引用；已加入 `platformio.ini` lib_deps）。
- 安装为 `M5UnitRoller@0.0.1`。类 **`UnitRollerI2C`**，头文件 `unit_rolleri2c.hpp`。
- 依赖仅 `Arduino.h` + `Wire.h`，**不依赖 M5Unified**，可与 M5Atom 共存。
- 库已封装帧/CRC，**直接调方法即可，无需手写协议**。

## 库 API（I²C，UnitRollerI2C）
- 初始化：`begin(&Wire, 0x64, 26, 32, <freq>)` —— **freq 勿用 400000/100000**（危险频率），暂定 ~200000，待实测
- 模式枚举：`ROLLER_MODE_SPEED=1 / _POSITION=2 / _CURRENT=3 / _ENCODER=4`
- 控制：`setMode(mode)`、`setOutput(en)`(1开/0关)、`setCurrent(i)`、`setSpeed(s)`+`setSpeedMaxCurrent()`、`setPos(p)`+`setPosMaxCurrent()`、`setSpeedPID()`、`setPosPID()`
- 读回：`getCurrentReadback()`、`getSpeedReadback()`、`getPosReadback()`（均 ÷100）、`getCurrent()/getSpeed()/getPos()`（读设定值）
- 状态：`getErrorCode()`、`getOutputStatus()`、`getSysStatus()`、`getMotorMode()`、`getVin()`(÷100)、`getTemp()`、`getCalBusyStatus()`、`getFirmwareVersion()`
- 其它：`setRGB/setRGBMode/setRGBBrightness`、`setMotorID/getMotorID`、`setI2CAddress`、`setStallProtection`、`setDialCounter`(编码器模式)

## 数量纲与范围（raw ↔ 实际）
| 量 | 换算 | 范围 |
|----|------|------|
| 电流 | mA = raw / 100 | raw ±120000 → **±1200 mA** |
| 速度 | RPM = raw / 100 | — |
| 位置 | 值 = raw / 100 | — |
| 输入电压 | V = raw / 100 | — |
| PID | P÷1e5, I÷1e7, D÷1e5 | — |

## 状态码
- **ErrorCode**：1=过压，2=堵转，4=越界（堵转锁定后需发"解除保护"才恢复）。
- **SysStatus**：0=待机，1=运行，2=错误。
- **MotorMode**：1/2/3/4 同模式枚举。

## 底层帧协议（参考，库已处理，一般无需手写）
- 配置/控制帧定长 **15 字节**：`CMD(1) | DeviceID(1) | Data1(4) | Data2(4) | Data3(4) | CRC8(1)`，多字节**小端**。
- 响应 `CMD = 请求CMD + 0x10`，以 `0xAA 0x55` 开头（不计入 CRC）。CRC8：poly **0x8C**、init 0x00。
- 状态读回请求帧仅 4 字节(`0x40|ID|0x00|CRC`)，响应 17 字节(`0x50|ID|Speed4|Pos4|Cur4|Mode1|Status1|Err1|CRC`)。
- 命令码：0x00 使能 / 0x01 模式 / 0x06 解除堵转 / 0x07 存Flash / 0x20 速度 / 0x22 位置 / 0x24 电流 / 0x40 读状态。

## ⚠️ 已知风险：双 I²C 总线共存（IMU on Wire1 + 电机 on Wire）
- **现象（用户经验，待复现验证）**：M5 各开发板库初始化时会为板子建对象、其中**内含 I²C 控制器初始化**，
  即使你不用也会自初始化一遍。当**同时使用两个 I²C 控制器**（本项目：IMU 在 Wire1、RollerCAN 在 Wire）时，
  可能出现**其中一路失效**（数据冻结/控制无效）。
- **对策（用户验证可用，暂不写入代码）**：在 `stop`（停止/复位）之后，**把两个 I²C 控制器都重新初始化一遍**，
  通常即可恢复同时可用。
- **触发排查的判据**：若发现"读 IMU"和"读/控电机"**不能同时进行**（一动一坏），优先怀疑此问题，再上重初始化对策。
- 关联：[esp32-i2c-frequency-caveat.md](esp32-i2c-frequency-caveat.md)。
