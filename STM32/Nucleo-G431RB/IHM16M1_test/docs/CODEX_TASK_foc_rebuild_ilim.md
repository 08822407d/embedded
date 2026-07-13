# Codex 任务书 — 降限流重生成后的离线重构建 + 差异复核(不烧录 · 不碰硬件 · 不转电机)

> 给 Codex:读完即可自主执行。**全程离线**——只读/构建生成工程 + 写 `docs/`;**绝不烧录、不连板、不上电、不转电机、不启动 MotorPilot/MC Workbench、不重生成、不手改生成树。**
> 项目根 = `C:\Users\cheyh\Desktop\embedded\STM32\Nucleo-G431RB\IHM16M1_test`。真相以 `docs/` 为准:`FACTS.md`、`FOC_FW_BUILD.md`、`FOC_BRINGUP_PREP.md`、`DECISIONS.md` #006、`GITIGNORE_AND_REGEN.md`、`SYNC_WORKFLOW.md`。**动手前先读。**

## 背景
用户已按 DECISIONS #006 选 **B 路线**:在 MC Workbench 把 **Max current(Iqmax)0.5→0.3A**、**Speed Target 500→100rpm** 改好并**用 WB 自带 CubeMX 重生成了 C 工程**(CC 已核:`.ioc` 与 `Inc/drive_parameters.h`/`pmsm_motor_parameters.h` 等 2026-07-13 重生成,`M1_IQMAX=0.3`/`M1_NOMINAL_CURRENT=0.3`/`M1_DEFAULT_TARGET_SPEED_RPM=100` 已落地)。**本任务 = 对新工程离线重构建 + 复核差异 + 更新文档。** 生成已由用户完成,你不再生成。

## ⛔ 红线
- 全程离线:只读生成工程 `GM16020-06_foc/`、构建、写 `docs/`。
- **绝不**:烧录/下载/擦写/复位运行、枚举连板、跑 Programmer 写操作、启动 MotorPilot/MC Workbench 驱动功能、给电机母线上电、重生成、手改生成树控制参数。
- 需硬件实测或需改控制参数/重生成的判断 → 停,`ESC` 交回 CC。

## §-1 交互(HITL/ESC,应极少)
遇须授权/超出离线职责 → 按既有格式停等或 ESC 交回 CC(见 `CODEX_TASK_foc_bringup_prep.md` §-1)。

## §0 权限预检
- CubeIDE headless 可执行、CubeCLT gcc/objcopy/size/readelf、文件写 `docs/`。烧录不预检、不做。

## §1 已确认(直接用)
- Eclipse 工程根 `GM16020-06_foc/STM32CubeIDE/`,工程名 `GM16020-06_foc`,配置 `Debug`,链接脚本 `STM32G431RBTX_FLASH.ld`。
- 构建工具 = STM32CubeIDE **1.19.0** headless(方式同 `FOC_FW_BUILD.md`:`--launcher.suppressErrors -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data <干净临时workspace> -no-indexer -import <STM32CubeIDE 路径> -cleanBuild "GM16020-06_foc/Debug" -verbose -printErrorMarkers`);此前 FOC 工程**无需** CMSIS-DSP `-I` 绕过。
- 改动落地(CC 已核 `.ioc`):`M1_IQMAX=0.3`、`M1_NOMINAL_CURRENT=0.3`、`M1_DEFAULT_TARGET_SPEED_RPM=100`;其余配置**理应不变**(Hall 58°/120°/PC6-8、三电阻双 ADC、PWM TIM1 30kHz、OV15/UV7、DriverProtection PA11、MCP USART2 1843200)。

## §2 任务(离线)
1. **重构建**:headless clean Debug → 新 `GM16020-06_foc.elf/.hex/.bin`(objcopy 出 hex/bin),记录 `size`(text/data/bss)与 SHA-256;确认 `0 errors/0 warnings`、构建后无 CubeIDE/java 残留进程。
2. **差异复核**(从生成的 `.ioc` + `Inc/*_parameters.h` + 相关 C,不只信 UI):
   - **确认降限流已进固件**:Iqmax/最大相电流对应的 define(如 `NOMINAL_CURRENT`/`IQMAX`/电流标幺相关)= **0.3A 等效**;`DEFAULT_TARGET_SPEED_RPM=100`。
   - **PI 增益已随 0.3A 重算**:对比 `FOC_FW_BUILD.md`/`FOC_BRINGUP_PREP.md` 里旧(0.5A/500rpm)的 Iq/Id/Speed P·I,给出新值并说明"变了、合理"。
   - **其余配置未跑偏**:Hall placement 58°/displacement 120°/PC6-8、三电阻双 ADC + 引脚、PWM 30kHz、保护阈值(OV15/UV7)、DriverProtection PA11、无自动 `MC_StartMotor1`、MCP USART2 1843200 —— 逐项确认**与上次审计一致**;不一致的**醒目列出**。
   - Ke 仍为量化 0.4(不变,已定论不进控制)——顺带确认没被这次改动带偏。

## §3 交付与落盘
- 更新 `FOC_BRINGUP_PREP.md`:凡基于 **0.5A/500rpm** 的数值/建议 → 改为 **0.3A/100rpm** 现状(尤其"保守首测参数集"——现在默认即保守值);保留 Ke、霍尔症状/修法、遥测、风险台账。
- 更新 `FACTS.md`:新增一条(接续 F 编号)记"降限流重生成 + 重构建":0.3A/100rpm 落地、新 build size/SHA、PI 重算值、其余不变;**不改旧条目**(旧的 0.5A 构建 F-27/F-30/F-31 属历史留存)。
- 更新 `STATUS.md`:反映"已按 #006-B 降 Iqmax=0.3A/目标 100rpm 重生成+重构建;待现场 SOP + 用户授权首次带电"。
- 在 `DECISIONS.md` #006 **补记结论**:用户已选 B(0.3A 重生成),不再是待定。
- **收尾 = commit + push**(SYNC_WORKFLOW)。

## §4 纪律
- 红线见顶部;差异复核发现**真问题**(会影响安全/正确性)→ 醒目写报告 + ESC 提醒 CC,不自行改/不重生成。
- 任何"只能带电才能定"的项 → 标注留给现场 SOP,不实测。
- 改动限 `docs/`;Windows 路径含空格加引号;别乱设 `core.autocrlf`。允许更周全的方法但结论要有证据、留痕。
