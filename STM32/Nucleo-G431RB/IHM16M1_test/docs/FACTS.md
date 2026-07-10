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
| F-5 | 本机已装 ST 工具:**STM32CubeCLT 1.18.0**(`/opt/st/stm32cubeclt_1.18.0`:`STM32_Programmer_CLI` **v2.19.0**、`openocd`、`arm-none-eabi-gcc` **13.3.1**;**CubeCLT 不含 `make`** → 编译用系统 GNU Make 4.3 + 其 GCC)、**STM32CubeIDE 1.18.1**;未装 stlink-tools、`stm32flash` | codex 冒烟实测 | 2026-06-30 |
| F-11 | **硬件/通信/工具链冒烟测试通过**(只读+离线,详见 [`HW_SMOKE_TEST.md`](HW_SMOKE_TEST.md)):SWD 只读读到 Device ID `0x468`/flash 128KB/option bytes;CubeMX headless 生成 + CubeCLT GCC 编译通过。ST-Link FW=`V3J16M9`。**注**:CubeMX `.ioc` 默认请求 G4 FW **V1.6.3**,本机仅 **V1.6.1**(已用本机版本;装 MCSDK 时留意 FW 版本匹配) | codex 冒烟实测 | 2026-06-30 |
| F-12 | **Windows 侧 ST 工具已就位**:CubeCLT **1.18.0**(`C:\Program Files\STMicroelectronics\STM32CubeCLT_1.18.0`, `STM32_Programmer_CLI` **v2.19.0**, `arm-none-eabi-gcc` **13.3.1**);CubeIDE **1.19.0**(含 GNU Make **4.4.1_st_20231030-1220** 与 bundled CubeProgrammer **2.20.0**);CubeMX **6.18.0**(`C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeMX`;**2026-07-08 由用户手工全量安装器从 6.17.0-RC5 升级**,运行时核验 `Accepted Software Licenses` 含 `STM32CubeMX.6.18.0`、`DB.6.0.141`;更新器缓存 `C:\Users\cheyh\.stm32cubemx\plugins\updater\loadedSoftware\MX\` 尚有 **6.18.0-RC3** 惰性残留,非生效版;更新器订阅了 pre-release 通道故此前会是 RC 版);standalone CubeProgrammer **2.18.0**;G4 FW **V1.6.1/V1.6.2** 已安装;X-CUBE-MCSDK **6.4.2** 已安装(`C:\Program Files (x86)\STMicroelectronics\MC_SDK_6.4.2`, MC Workbench `STMCWB.exe` product version **6.4.2**, MotorPilot 存在);原 **6.4.1** 已升级且安装目录不再存在 | Windows codex 只读盘点(`Get-Command`/`Get-ChildItem`/`--version`/file version);MCSDK 项于 2026-07-10 复核 | 2026-07-10 |
| F-13 | **Windows 侧同一块板在线**:用户确认目标硬件已通过 mini-USB 接到本 PC;`STM32_Programmer_CLI -l` 报 ST-Link SN `002A00403234510E33353533`, ST-Link FW `V3J17M10`, Board `NUCLEO-G431RB`, VCP 当前为 `COM4`;PnP 设备存在 `USB\VID_0483&PID_374E\002A00403234510E33353533`/`ST-Link Debug`/`NOD_G431RB` | Windows codex 冒烟实测 + 用户 HITL 确认 | 2026-07-03 |
| F-14 | **Windows 侧 SWD 只读通信通过**:HOTPLUG 读到 Device ID `0x468`, DBGMCU IDCODE `0x20036468`, flash size `128 KBytes`, flash-size register `0x1FFF75E0 : 0080`, option bytes 可显示(`RDP 0xAA Level 0`);全程未擦写/未烧录/未 run | Windows codex 冒烟实测(`STM32_Programmer_CLI -c port=SWD mode=HOTPLUG ... -r32/-r16/-ob displ`) | 2026-07-03 |
| F-15 | **Windows 侧离线工具链编译通过**:CubeMX 6.17.0-RC5 + G4 FW V1.6.2 生成 `hw_smoke_win/g431_hw_smoke_win/`;`hw_smoke_win/build_win.ps1` 对 Windows 生成的 `sysmem.c`/`syscalls.c` 和 Makefile `C_SOURCES` 做后处理;CubeIDE GNU Make + CubeCLT GCC 编译出 `g431_hw_smoke_win.elf/.bin/.hex`, `.elf` 87,352 bytes, size=`text 4400/data 12/bss 1572` | Windows codex 冒烟实测,详见 [`HW_SMOKE_TEST_WIN.md`](HW_SMOKE_TEST_WIN.md) | 2026-07-03 |
| F-16 | **UM2415 Rev 4 明确 IHM16M1 的 FOC 三电阻为出厂默认**:`JP4/JP7` 底部焊桥 open、`J5/J6` closed、`J2` 2-3、`J3` 1-2;**FOC 单电阻**为 `JP4/JP7` closed、`J5/J6` closed、`J2` 1-2、`J3` 2-3;**6-step**为 `JP4/JP7` closed、`J5/J6` open。当前实物确认见 F-18 | UM2415 Rev 4 p.7 Table 2 / Table 3 + p.14 schematic + p.16 BOM;截图与判读见 [`IHM16M1_SHUNT_CONFIG.md`](IHM16M1_SHUNT_CONFIG.md) | 2026-07-03 |
| F-17 | **校正 F-8 的旧简写**:`霍尔 FOC 必须三电阻`不是准确的通用事实。UM2415/MCSDK 均列出 IHM16M1 的 FOC single-shunt 与 three-shunt 变体,Hall 传感器是独立的速度/位置反馈变体;但本项目的 Motor Profiler/未知电机路线仍应优先使用默认三电阻,因为 MCSDK release notes 写明 Motor Profiler 不支持 single-shunt,且 STO-PLL 必须 three-shunt | UM2415 Rev 4 p.7;MCSDK 6.4.1 `X-NUCLEO-IHM16M1.json`;`Release Notes for X-Cube-MCSDK.html`;详见 [`IHM16M1_SHUNT_CONFIG.md`](IHM16M1_SHUNT_CONFIG.md) | 2026-07-03 |
| F-18 | **当前这块 IHM16M1 实物确认处于默认 FOC 三电阻采样拓扑**:用户对照 UM2415 截图确认 `J5/J6` 均有跳帽闭合;底面照片与用户观察确认 `JP4/JP7` 两组焊桥均干净断开(open),无焊锡/导线/元件连接。`J3` 用户观察为接在远离霍尔传感器接线针一侧两针,但该相对方向未换算成引脚编号;当前单/三电阻结论不依赖 J3 | 用户 HITL-2 视觉确认 + 用户提供底面照片 + [`IHM16M1_SHUNT_CONFIG.md`](IHM16M1_SHUNT_CONFIG.md) | 2026-07-03 |
| F-19 | **IHM16M1 霍尔/编码器接口 J1 的 5V 供电来自 Nucleo 开发板**,不是电机母线;J1 pin1/2/3 分别为 Hall 1/2/3,pin4 为 `5 V supply from Nucleo development board`,pin5 为 GND;板上为开漏/开集电极 Hall 提供 R20/R21/R22 上拉 | UM2415 Rev 4 p.9 Section 4.5 / Table 6;详见 [`HALL_PROBE_FW.md`](HALL_PROBE_FW.md) | 2026-07-08 |
| F-20 | **IHM16M1 + NUCLEO-G431RB 的默认霍尔读取引脚可用 PC6/PC7/PC8**:MCSDK 6.4.1 组合解选择 H1=PC6/TIM3_CH1,H2=PC7/TIM3_CH2,H3=PC8/TIM3_CH3;默认 VCP UART 为 USART2 PA2/PA3 | UM2415 Rev 4 p.11/p.12 schematic + MCSDK 6.4.1 power/control board JSON + MCSDK 示例组合解;详见 [`HALL_PROBE_FW.md`](HALL_PROBE_FW.md) | 2026-07-08 |
| F-21 | **只读霍尔诊断固件已离线编译并在 HITL-1 授权后烧录/校验成功**:`hall_probe_fw` 仅初始化 USART2 PA2/PA3 和 Hall 输入 PC6/PC7/PC8,不配置 TIM1/PWM/桥臂 enable/电机控制栈;Windows CubeCLT GCC 构建输出 `hall_probe_fw.elf/.hex/.bin`,size=`text 1964/data 4/bss 4`;STM32CubeProgrammer 对 SN `002A00403234510E33353533` 写入 `1.92 KB` 到 `0x08000000` 并 verify successful | codex 离线编译 + HITL-1 后烧录实测;源码与报告见 [`hall_probe_fw/src/main.c`](../hall_probe_fw/src/main.c) / [`HALL_PROBE_FW.md`](HALL_PROBE_FW.md) | 2026-07-08 |
| F-22 | **只读霍尔诊断串口初读正常**:VCP `COM4` 以 `115200 8N1` 输出有效 Hall 状态 `hall=010 sector=4 elec_deg_nom=240`,串口 `r` 命令已验证可清零到 `trans=0 h1_rise=0 invalid=0 skipped=0` | codex 串口读取实测;详见 [`HALL_PROBE_FW.md`](HALL_PROBE_FW.md) | 2026-07-08 |
| F-23 | **当前散货电机手转霍尔测得极对数 = 4 对极**:用户手转约一机械圈时,霍尔序列连续有效 `010 -> 011 -> 001 -> 101 -> 100 -> 110 -> 010`,最终 `h1_rise=4`, `trans=25`, `invalid=0`, `skipped=0`;理论 4 对极一圈约 24 个 Hall transition,实测多 1 个 sector 表示略微转过起点,不影响 H1 上升沿极对数结论 | HITL-2 用户手转 + codex 串口采集;详见 [`HALL_PROBE_FW.md`](HALL_PROBE_FW.md) | 2026-07-08 |
| F-24 | **目标电机 = GM16020-06(标签另印 `B1625S-006`)三相八线带霍尔无刷**:外转子稀土磁,外径 16mm/长 23mm/轴 1.5mm/重 17.5g;**额定 DC 12V,空载转速 18200 RPM,空载电流 0.16A**;**4 对极**(F-23 实测);6 槽定子;线序 = 霍尔 5 线 `VCC/GND/H1/H2/H3` + 三相 `U/V/W`。**推算 KV≈1517 RPM/V(高 KV)**——属"高转速、低电流、低电感"微型电机,Motor Profiler 对此类有已知坑(见 DECISIONS #003),需预期可能标不准/要调参 | 商家商品页(型号/铭牌/规格)+ 用户实拍 + F-23 极对数实测 | 2026-07-08 |
| F-25 | **已备妥 12V 电机母线电源**:用户确认取得合适的 12V 电源为 IHM16M1 供电(电机额定 12V,落在 STSPIN830 4.5–45V 供电范围内)。**Motor Profiler/标定时应经限流实验电源供电**,限流先设低值(空载 0.16A → 起步上限 ≤0.5–1A),电流/急停方案见 Motor Profiler 任务书 | 用户确认 | 2026-07-08 |
| F-26 | **MC Workbench 已生成的 Motor Profiler 工程结构已核实**:`GM16020-06_profile/STM32CubeIDE/` 是 Eclipse 工程根,有 Debug 配置和链接脚本 `STM32G431RBTX_FLASH.ld`;其 `.cproject` 将 `../../MCSDK_v6.4.2-Full/MotorControl/libMP/libmp-G4-IAR_ARMv7-M.a` 作为 Motor Profiler 库链接输入,库文件实存 | codex 只读核对 `.project`/`.cproject`/文件路径 | 2026-07-10 |
| F-27 | **Motor Profiler 固件 Phase B 已在 Windows STM32CubeIDE 1.19.0 headless 下离线编译通过**:Debug 生成 `GM16020-06_profile.elf/.hex/.bin`,size=`text 57728/data 3184/bss 7376`;生成工程漏配 CMSIS-DSP include,通过 headless 临时 `-I .../Drivers/CMSIS/DSP/Include` 补齐后 clean build 0 errors/0 warnings。无手改 `.cproject`、无烧录/硬件访问 | codex 离线构建实测;详见 [`PROFILER_FW_BUILD.md`](PROFILER_FW_BUILD.md) | 2026-07-10 |
| F-6 | **工具链非最新(查证当日)**:最新 = CubeCLT **1.21.0**(2026-02)/ CubeProgrammer **2.22**(2026-04)/ CubeIDE **2.1.0**(从 1.x 大版本跃迁);本机偏旧但**对成熟芯片 G431 功能够用、不阻塞**。升级收益 = GCC14 + 修复 + 新特性;升级需登录 st.com 下载、属系统级操作 → **由用户主导,勿擅自装** | WebSearch(st.com / community.st.com) | 2026-06-29 |
| F-7 | 硬件组合 `IHM16M1 + G431RB` = ST 官方套件 **P-NUCLEO-IHM03** 的板卡组合(原配 gimbal 电机 GBM2804H-100T);核心手册 **UM2415**(IHM16M1)、**UM2538**(IHM03 pack FOC 评估) | WebSearch(st.com) | 2026-06-29 |
| F-8 | IHM16M1 功率级 = **STSPIN830**,支持**单 / 三电阻**电流采样(板上跳线选)。⚠️ 本条末句"霍尔 FOC 需三电阻"及"当前配置待核实"**已被 F-16/F-17/F-18 校正/落实**:霍尔本身不强制三电阻,但本项目仍用默认三电阻;当前实物 = 默认 FOC 三电阻 | WebSearch + UM2415 | 2026-06-29 |
| F-9 | ST 电机套件 **X-CUBE-MCSDK 最新 = 6.4.1**(2025,含 **Motor Profiler** + **MC Workbench**,支持 STM32G4);**本机未装** | WebSearch(community.st.com) + `find` 无果 | 2026-06-29 |
| F-10 | 本机电机开发环境**只差 MCSDK**:已装 **STM32CubeMX 6.15.0**(`~/STM/STM32CubeMX`)+ **STM32Cube_FW_G4 V1.6.1**(Repository)+ CubeCLT 1.18 / CubeIDE 1.18.1(F-5);缺 X-CUBE-MCSDK(F-9) | `find` + `ls Repository` 实测 | 2026-06-29 |

> 注1:**端口 / 接口(`/dev/ttyACMx`)不在此记录** —— 每次插拔 / 枚举顺序会变,属易变状态。**每次烧录 / 调试前按 [`OPERATIONS.md`](OPERATIONS.md) §0 现场扫描**,靠 F-3 的 ST-Link SN 解析当前端口;此处只记固定标识。
> 注2:仍未核实的具体电气配置包括**母线电压范围、正式 FOC 工程用 PWM/ADC/enable/fault 完整映射**等;用前查数据手册(UM2415 / UM2538)/MCSDK 板卡定义/实测,**别从模型记忆里填具体引脚号**(易错且危险)。

## 待核实(核实后上移到上表)
- IHM16M1 板级母线电压允许范围的确切上下限(已知 12V 稳妥可用,F-25;精确范围可后续查 UM2415 核)
- G431 ↔ IHM16M1 完整引脚映射:三相互补 PWM(TIM1?)、ADC 通道、使能 / 故障线;板载跳线 / 焊桥默认配置(霍尔读取脚 PC6/7/8 已由 F-20 确认)
- 串口 VCP 波特率已用于只读霍尔诊断 `115200 8N1`;正式 MCSDK 工程默认值仍待由 Workbench 工程确认
- 正式 MCSDK 工程生成前,仍需确认当前 Windows 工具链组合与 **MCSDK 6.4.1 / CubeMX 6.18.0** 的实际生成兼容性(6.17.0-RC5 的 headless 生成 bug 是否在 6.18.0 消失,待生成时复验;见 F-12/F-15)
