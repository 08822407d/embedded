# FACTS — 已验证事实台账

> **这里只放确凿事实**:每条 = 事实 + **来源**(实测 / 用户确认 / 官方数据手册页码)+ 日期。
> **调试反复失败时先读本文件,把这些当地面真值,别无故怀疑已验证的读数 / 接线 / 状态。** 先怀疑你新写的代码 / 接线 / 假设。
> 没核实的别往这写;不确定的放对应文档的「待确认」区。

## 已验证条目
| 编号 | 事实 | 来源 | 日期 |
|---|---|---|---|
| F-1 | 主控为 STM32 Nucleo-G431RB(MCU = STM32G431RB) | 用户陈述 + 目录命名 + ST-Link 实测报 `Board: NUCLEO-G431RB` | 2026-06-29 |
| F-2 | 扩展板为 X-NUCLEO-IHM16M1(三相电机驱动评估板) | 用户陈述 + 目录命名 | 2026-06-29 |
| F-3 | 板载调试器 = **STLINK-V3**(USB `0483:374e`);**本板 ST-Link SN = `002A00403234510E33353533`**(唯一固定标识,用于现场匹配端口) | `STM32_Programmer_CLI -l` 实测 | 2026-06-29 |
| F-4 | MCU **Device ID = `0x468`** = STM32G431 系列(与 F-1 板名一致) | ST-Link 探测 | 2026-06-29 |
| F-5 | 本机已装 ST 工具:**STM32CubeCLT 1.18.0**(`STM32_Programmer_CLI` v2.20.0 + `openocd` + `dfu-util`,在 `/opt/st/stm32cubeclt_1.18.0`)、**STM32CubeIDE 1.18.1**(`/opt/st/stm32cubeide_1.18.1`);未装 stlink-tools(`st-info`/`st-flash`)、`stm32flash` | `command -v` + `ls /opt/st` 实测 | 2026-06-29 |
| F-6 | **工具链非最新(查证当日)**:最新 = CubeCLT **1.21.0**(2026-02)/ CubeProgrammer **2.22**(2026-04)/ CubeIDE **2.1.0**(从 1.x 大版本跃迁);本机偏旧但**对成熟芯片 G431 功能够用、不阻塞**。升级收益 = GCC14 + 修复 + 新特性;升级需登录 st.com 下载、属系统级操作 → **由用户主导,勿擅自装** | WebSearch(st.com / community.st.com) | 2026-06-29 |
| F-7 | 硬件组合 `IHM16M1 + G431RB` = ST 官方套件 **P-NUCLEO-IHM03** 的板卡组合(原配 gimbal 电机 GBM2804H-100T);核心手册 **UM2415**(IHM16M1)、**UM2538**(IHM03 pack FOC 评估) | WebSearch(st.com) | 2026-06-29 |
| F-8 | IHM16M1 功率级 = **STSPIN830**,支持**单 / 三电阻**电流采样(板上跳线选,本板当前配置**待核实**;**霍尔 FOC 需三电阻**) | WebSearch + UM2415 | 2026-06-29 |
| F-9 | ST 电机套件 **X-CUBE-MCSDK 最新 = 6.4.1**(2025,含 **Motor Profiler** + **MC Workbench**,支持 STM32G4);**本机未装** | WebSearch(community.st.com) + `find` 无果 | 2026-06-29 |
| F-10 | 本机电机开发环境**只差 MCSDK**:已装 **STM32CubeMX 6.15.0**(`~/STM/STM32CubeMX`)+ **STM32Cube_FW_G4 V1.6.1**(Repository)+ CubeCLT 1.18 / CubeIDE 1.18.1(F-5);缺 X-CUBE-MCSDK(F-9) | `find` + `ls Repository` 实测 | 2026-06-29 |

> 注1:**端口 / 接口(`/dev/ttyACMx`)不在此记录** —— 每次插拔 / 枚举顺序会变,属易变状态。**每次烧录 / 调试前按 [`OPERATIONS.md`](OPERATIONS.md) §0 现场扫描**,靠 F-3 的 ST-Link SN 解析当前端口;此处只记固定标识。
> 注2:F-1~F-10 是**身份 / 工具链 / 选型**层面确认;**具体电气配置(采样单 / 三电阻跳线、母线电压、引脚映射、霍尔接入引脚)尚未核实**,用前查数据手册(UM2415 / UM2538)/ 实测,**别从模型记忆里填具体引脚号**(易错且危险)。

## 待核实(核实后上移到上表)
- **IHM16M1 当前电流采样跳线 = 单电阻 / 三电阻**(霍尔 FOC 必须三电阻);采样电阻阻值 / 运放增益
- **霍尔 H1/H2/H3 在 IHM16M1 → G431 的对应引脚 / 定时器输入**;母线电压允许范围
- G431 ↔ IHM16M1 完整引脚映射:三相互补 PWM(TIM1?)、ADC 通道、使能 / 故障线;板载跳线 / 焊桥默认配置
- 串口 VCP 波特率
- **散货电机是否 gimbal 类型 / 大致 KV / 转速范围**(影响 Motor Profiler 标定成败)
- 待装 **X-CUBE-MCSDK 6.4.1**(见 F-9/F-10);装后确认与 CubeMX 6.15 兼容、Motor Profiler 是否列出 IHM16M1
