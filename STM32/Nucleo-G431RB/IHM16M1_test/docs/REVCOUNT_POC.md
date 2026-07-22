# REVCOUNT_POC — PC 侧霍尔整圈数粗控 PoC

> **阶段状态(2026-07-22):离线实现与自测完成;USB-only 实板只读验证等待 HITL。**
>
> 本阶段没有枚举/打开 COM、没有连接板卡、没有烧录、没有修改或重生成 `GM16020-06_foc/`、没有启动 MotorPilot/Workbench、没有给母线上电、没有 Start/转电机。§3/§4 下表尚无实测数据,不得把离线 mock 结果当作硬件结论。

## 1. 边界与目标

目标对应 REQUIREMENTS §6 / DECISIONS #007-B:保持 F-36/F-37 已验证固件不变,在 PC 侧读取 `HALL_EL_ANGLE`,累计机械圈数,最终验证“转 N 圈后停”的误差和提前量 `lead`。

- 固件锚点:0.3 A/100 rpm FOC,ELF SHA-256 `0036DEE8C9431ECA4AC763BF9B481E0C56F23C238EE5B1E77078C0A387092DB0`。
- 接口:MCP/ASPEP,USART2 PA2/PA3,1843200 baud;COM 按 ST-LINK SN `002A00403234510E33353533` 从 Windows 注册表现场解析。
- 角度:F-39 的 `HALL_EL_ANGLE` 为 s16 电角度;4 对极,故 `机械圈数 = 累计电角度 counts / (4 * 65536)`。
- 硬件风险不变:0.3 A 是软件 Iq 限幅,不是短路保护;当前无保险丝。

## 2. 数据通路选型

### 2.1 MotorPilot QML 脚本:调研后不选

本机 MCSDK 6.4.2 确实带脚本入口,不是“完全不支持自动化”:

- `GUI/scriptExample.qml`/`scriptExample2.qml` 能调用 `connectToBoard()`、寄存器 `getValue()`/`setValue()`、Fault Ack、Start/Stop 和录制。
- `scriptExample2.qml` 还给出 `mcRegBank.configureDataLog(1321,2048,2,[...],1,[],255)`;十进制 1321 正是本工程 `MC_REG_ASYNC_UARTA=0x0529`。这旁证了生成源码推导出的异步配置 ID 与参数顺序。

未选原因:

1. 入口依赖 MotorPilot GUI、用户选择脚本并连接,没有发现可离线测试的 headless CLI。
2. 示例状态机按 500 ms/秒级 Timer 编排;录制由 GUI 内部消费,公开 QML 示例没有提供逐个异步样本及设备时间戳的直接回调。无法在发停前对每个样本执行丢样/回绕判定。
3. 工具必须与 MotorPilot 互斥;把核心闭环放入 GUI 会增加现场状态和复现依赖。

因此没有启动 MotorPilot 做采样率实测。若独立客户端实板协议不兼容,可把 QML 作为回退路线,但须重新设计样本导出与实时停止机制。

### 2.2 采用方案:独立 Python MCP/ASPEP 客户端

代码位于 [`tools/revcount_poc/`](../tools/revcount_poc/README.md),Python 3.12.2,仅标准库。串口层用 `ctypes` 调 Win32 API,不安装 `pyserial`;协议和控制逻辑可用内存 transport 测试。

选择理由:

- ASPEP 帧、MCP 请求、异步样本和设备 timestamp 全部可见,可严格判断丢样。
- 只读手转和带电控制是不同 CLI 路径;`probe`/`hand-turn` 源码不含 SET/命令调用。
- CSV 和 JSON 摘要格式固定,便于重复试验和统计。

## 3. 协议核对

依据本机 MCSDK 6.4.2 Documentation、MotorPilot QML 资源和实际生成源码 `aspep.c`、`mcp.c`、`mcpa.c`、`hf_registers.c`、`sync_registers.c`、`mcp_config.c`:

### 3.1 ASPEP

- 4-byte little-endian header;低 nibble packet ID;DATA 长度在 bits 4..16;bits 28..31 为 CRC-4,查表顺序照生成 `ASPEP_ComputeHeaderCRC()`。
- 连接:发送匹配 capabilities 的 Beacon(version=0,data CRC=0,RX=7,TXS=7,TXA=32),核对回包,再 Ping;生成固件回 configured bit 后进入 CONNECTED。
- 控制器请求 DATA=`0x9`;固件同步响应 `MCTL_SYNC=0xA`;异步响应 `MCTL_ASYNC=0x9`。
- 解码器按 header CRC 逐字节重同步,拒绝超 2048-byte payload 和 NACK。

### 3.2 MCP 项

沿用 runbook 已审核 ID/格式,工具实际使用:

| 项 | ID/命令 | 格式/用途 |
|---|---:|---|
| STATUS | `0x0049` | U8,IDLE=0/RUN=6/FAULT_NOW=10/FAULT_OVER=11 |
| CONTROL_MODE | `0x0089` | U8,运行前只读确认 speed mode=3 |
| FAULTS_FLAGS | `0x0019` | U32,低16 past/高16 current |
| SPEED_MEAS | `0x0059` | S32 rpm |
| I_Q_MEAS | `0x08D1` | S16 raw;约 10027 count/A |
| HALL_EL_ANGLE | `0x0ED1` | S16 electrical angle |
| HALL_SPEED | `0x0F11` | S16,raw x 6 rpm |
| SPEED_RAMP | `0x01A9` | RAW `[S32 rpm,U16 ms]` |
| ASYNC_UARTA | `0x0529` | RAW MCPA 配置;仅带电 `run` 使用 |
| Fault Ack/Start/Stop | `0x0039/0x0019/0x0021` | M1 命令 |

SET RAW 外层为 `dataID + U16 rawSize + rawData`;每个同步回包最后一 byte 是 MCP status。

### 3.3 异步样本

`run` 配置两个 HF 数据 `HALL_EL_ANGLE`、`HALL_SPEED`,字段如下:

```text
bufferSize=32, HFRate=29, HFNum=2, MFRate=255, MFNum=0,
HF IDs=[0x0ED1,0x0F11], mark=1
```

30 kHz HF task 下 `HFRate=29` 即每 30 tick 一次,设备采样率 1000 Hz。每包 payload 为 `U32 firstTimestamp + N * [S16 angle,S16 hallSpeed] + U8 mark + U8 asyncID`;当前 buffer 配置每包 6 个样本、约 6 ms 批延迟。相邻设备 timestamp 必须精确增加 30;否则整次 trial 标 invalid 并进入 Stop 路径。

USB-only §3 要求“只读/零控制写入”,而开启 MCPA 必须 SET `ASYNC_UARTA`,所以 §3 **刻意不用异步配置**,改为单个同步 GET 同时读取 angle/STATUS/FAULTS/speed。真实最差轮询率是否满足 1000 rpm 所需 266.7 Hz,必须由 §3 实测;离线不能预判。

## 4. 解回绕与无效判定

设角度先按 U16 看待,相邻最短模差为:

```text
delta = ((current - previous + 32768) mod 65536) - 32768
```

`accumulated += delta`,再除以 `4 * 65536` 得机械圈。正反向都使用同一公式,小幅反向抖动会自然抵消。

仅看模差无法识别“恰好丢了一整电气圈”。因此不能把“归一化后 `abs(delta)<=32768`”误当成无丢圈证明。工具补充两类证据:

- 异步:设备 tick 必须连续;缺一个 1 kHz 样本即 invalid。
- 轮询:若 host 间隔在 `design-rpm` 下允许转过 `>=32768` counts,即存在半圈歧义,invalid。默认按最高批准域 1000 rpm 设计,间隔必须 `<7.5 ms`。
- 模差恰为 32768 也视为歧义,不猜方向。

采样基准为 `4 * pole_pairs * rpm / 60`:500 rpm 至少 133.3 Hz,1000 rpm 至少 266.7 Hz。工具同时输出 mean Hz、最差间隔对应 Hz、P95/最大间隔。

## 5. 控制与安全实现

- `probe`/`hand-turn`:只构造 MCP GET;不 Ack、不 Start、不 Stop、不 Ramp、不 SET。§3 母线断开时即便欠压 FAULT 也只记录,不 Ack。
- `run`:必须同时给 `--allow-motor-control --confirm-bus-powered --confirm-user-present`,但这些参数不能代替任务书 HITL-POWER/CC 旁站。
- 顺序:Fault Ack -> 等 current fault 清除且 IDLE -> 只读确认 speed mode -> 开 1 kHz async -> Start -> speed ramp -> 到 `N-lead` -> 停止 -> 等 speed `<=30 rpm` 且角度稳定 0.3 s。
- 默认 `v_test=500 rpm`;硬上限 `abs(rpm)<=1000`。
- `ramp` 停法:先 `SPEED_RAMP=0`、1000..2000 ms(默认1500),等待完整 ramp 时间后 Stop;`direct` 立即 Stop 作对照。
- fault、意外退出 RUN、反向速度超过 60 rpm、Hall 累计反向超过 0.25 圈、Iq 连续 0.2 s 顶到 3008 raw、采样 invalid、计圈无进展超时或普通异常都会尝试 Stop;Stop 后还须回到 IDLE、无 current fault 且速度/角度稳定才算有效。Ctrl+C/BaseException 也先尝试 Stop。现场仍以用户 1 秒断母线为最终急停。

## 6. 离线验证

命令:

```powershell
cd tools\revcount_poc
python -m unittest discover -s tests -v
python -m compileall -q revcount revcount_poc.py tests
```

结果:2026-07-22,Python 3.12.2,**15/15 tests passed**,`compileall` 通过。覆盖:

- 正向 4 电气圈=1机械圈、反向两圈、跨 `s16` 边界抖动;
- 设备 tick 丢样、host 长间隔、恰好半电气圈歧义均 invalid;
- ASPEP CRC/碎片化/错位重同步、Beacon/Ping 握手、多寄存器 GET、命令、MCPA 配置、异步 payload 解码;
- `hand-turn` mock 路径只出现 GET 并生成 CSV/summary;
- powered mock 完成 Ack -> Start -> ramp -> Stop -> disable async,并验证运行中意外退出 RUN 会判 invalid 且尝试 Stop;
- CLI 缺少任一带电确认 flag 时在打开 COM 前以 exit 3 拒绝。

未覆盖/待实板确认:Windows VCP 在 1843200 的实际打开/握手、真实同步 GET 轮询率、固件回包时序、实际异步流连续性。按红线这些不能在离线阶段擅自验证。

## 7. §3 USB-only 手转结果

**未执行,等待 HITL-USB-READONLY。** 前置必须由用户确认:

1. IHM16M1 电机母线物理断开,仅 Nucleo USB 供电;
2. MotorPilot/Workbench 已关闭,串口无其他客户端;
3. 连接的是 SN `002A00403234510E33353533` 目标板;
4. 用户准备按指针对齐后手转 K=3..5 圈并保持终点。

计划先 `probe`,再正/反向各至少 3 次 K=4。这里后续填写真实 CSV、误差与采样率:

| Trial | 方向 | K | measured turns | error | mean/min Hz | max gap | valid |
|---|---|---:|---:|---:|---:|---:|---|
| pending | - | - | - | - | - | - | - |

通过判据:每次 `abs(error)<=0.25 turn`,无 invalid,且最差采样率达到所选 §4 目标速度的 4x 回绕裕量。不通过先修工具/降目标速度;不得直接进入带电试验。

## 8. §4 带电 N 圈停止结果

**未授权、未执行。** 即使 §3 通过,仍须新的 HITL-POWER、用户在场且可 1 秒断母线、CC 旁站后才能开始。第一组 500 rpm/N=5/`lead=0` 用来测滑行圈数,再由数据定 lead;不能离线拍值。

| rpm | N | stop | lead | repeats | mean error | worst | std | mean coast |
|---:|---:|---|---:|---:|---:|---:|---:|---:|
| pending | - | - | - | - | - | - | - | - |

## 9. 当前结论

独立客户端的协议、计圈、丢样判定、CSV 和安全命令边界已离线闭环;没有改变固件或生成树。**尚无真实采样率、手转误差、滑行圈数或停位精度**,所以当前不能给 `lead` 数值,也不能判断 REQUIREMENTS §6 是否满足。下一合法动作是 §3 的 USB-only、母线物理断开、只读 HITL。
