# DECISIONS — 关键选择及其理由

> **只追加,别推翻已决之事。** 每条:编号 + 日期 + 决定 + 理由 + (可选)备选与放弃原因。
> 若要变更某条决策,**新增一条**说明"取代 #00X 及原因",不要原地删改历史。

---

## #001 — 采用 C6L 式持久化体系 + Codex 分工(2026-06-29)
- **决定**:本项目沿用 M5 `C6L_WalkieTalkie` 的方法论 —— 根 `CLAUDE.md` 接手入口 + `docs/` 七件套(STATUS/REQUIREMENTS/DESIGN/DECISIONS/OPERATIONS/FACTS/HANDOFF);CC 主需求/设计/验收,Codex 经 `docs/CODEX_TASK_*.md` 主实现。
- **理由**:对话上下文随时会断,真相须落盘 + git;把具体编码交 codex 省 Claude 额度。用户明确要求参考这两套机制。
- **适配**:红线从 C6L 的"持续射频发射"改为本项目的"未授权不烧录 / 不上电机电源 / 不驱动电机转动";工具链与外设事实针对 STM32G431 + IHM16M1 重写。
- ⚠️ **铁律(血泪)**:**备份绝不用"删文件"的方式** —— 上一个被删的 matrix/agent-context 模板就是这么把整套文档弄丢的。要快照用复制或 git。
- **配套**:`.claude/settings.json` 把只读/安全命令加 `allow` 白名单减弹窗,把烧录命令(多工具链候选)设 `ask` 必问,作红线的技术兜底。

## #002 — 工具链(部分定,见 #003)
- 初始候选:STM32CubeMX/CubeIDE、ST MCSDK、PlatformIO、裸 HAL/LL。**2026-06-29 定为 ST MCSDK 路线,详见 #003。**

## #003 — 2026-06-29 — 路线 = ST X-CUBE-MCSDK FOC + Motor Profiler 标定散货电机
- **决定**:走 ST 官方电机控制套件:**Motor Profiler 标定**散货电机 → **MC Workbench 生成 FOC 工程**(三电阻采样 + 霍尔位置反馈)→ 编译烧录 → 闭环调速。**不自研控制算法**。
- **理由**:硬件正是官方套件 **P-NUCLEO-IHM03**(IHM16M1+G431RB)组合,流程 / 文档(UM2538 / UM2415)齐全;电机散货、参数未知,Motor Profiler 正是自动测未知 PMSM/BLDC 参数的工具;MCSDK 支持 STM32G4。
- **已知风险(查证,详见 REQUIREMENTS 待确认 / FACTS)**:① Motor Profiler 对 gimbal / 低电流电机标定有坑(低压限、1-shunt 不稳),建议 >1000RPM;② **霍尔 FOC 必须三电阻**,否则只能 6-step;③ 霍尔低速 FOC 60° 跳变是已知难点。
- **依赖**:装 **X-CUBE-MCSDK 6.4.x**,由用户登录 st.com 安装(平台限制见 #004)。
- **否决**:自研 FOC / 六步 —— 官方套件现成,散货电机更需 Motor Profiler 自动标定,自研徒增工作量与风险。

## #004 — 2026-06-30 — 双机(Linux + Windows)开发,经 GitHub push/pull 保存与交接
- **决定**:本项目固定两台机器协作 —— **Linux(Ubuntu,本仓/编译/烧录)+ Windows PC(MCSDK 标定与 FOC 生成)**;**保存与交接统一走 GitHub `origin` 的 `commit + push` / `pull`**。详见 [`SYNC_WORKFLOW.md`](SYNC_WORKFLOW.md)。
- **理由**:**X-CUBE-MCSDK / MC Workbench / Motor Profiler 仅有 Windows 版**(2026-06-30 多个 ST 官方/社区来源查证:"ST MC Workbench runs on Windows",无 Linux 版),本机 Ubuntu 跑不了标定;而设计/文档/编译留在 Linux。两机须可靠同步 → GitHub。
- **配套**:① `.gitattributes` 统一仓库 LF,防跨 OS 换行翻动;② `.gitignore` 扩展忽略两 OS 构建产物 + 生成工程;③ **"保存"定义从"本地 commit"升级为"commit + push","接手"先 pull**(SYNC_WORKFLOW 纪律 1–2)。
- **取代**:此前 HANDOFF 里"整目录复制交接"的设想 → 改为 GitHub push/pull。

## #005 — 2026-07-03 — 校正采样拓扑约束:默认/推荐三电阻,但 Hall FOC 本身不强制三电阻
- **决定**:继续把本项目实际路线锁定为**确认并使用 IHM16M1 默认 FOC 三电阻配置**后再做 Motor Profiler/FOC 生成;同时校正 #003 风险项中的简写:不是"霍尔 FOC 必须三电阻",而是"UM2415/MCSDK 支持 FOC 单电阻与三电阻;本项目因 Motor Profiler 和未知电机调试风险而优先/需要三电阻基线"。
- **理由**:UM2415 Rev 4 Table 2 把 FOC 三电阻列为默认配置(`JP4/JP7` open、`J5/J6` closed、`J2` 2-3、`J3` 1-2),同时也列出 FOC single-shunt 配置;MCSDK 6.4.1 IHM16M1 board JSON 同时提供 `ThreeShunt_AmplifiedCurrents` 与 `SingleShunt_AmplifiedCurrents`,Hall 传感器作为独立速度/位置反馈变体存在。
- **风险边界**:MCSDK release notes 写明 Motor Profiler 不支持 single-shunt current sensing,且 STO-PLL 必须 three-shunt;因此即使 Hall FOC 不由霍尔本身强制三电阻,本项目的"未知电机 + Motor Profiler"路线仍应从三电阻开始。
- **配套**:截图和实物核对清单见 [`IHM16M1_SHUNT_CONFIG.md`](IHM16M1_SHUNT_CONFIG.md);当前实物到底是默认三电阻还是被改过,仍待用户按 `JP4/JP7` 底面焊桥与 `J5/J6` 跳帽确认。

## #006 — 2026-07-10 — 首次带电电流上限(ESC-1):CC 建议接受 Iqmax=0.5A(待用户最终确认)
- **背景**:codex 离线审计(FOC_BRINGUP_PREP)查明 **stock MCP 不能在运行时降低 speed-mode 的 Iqmax**;`Iqmax=0.5A` 是**固件控制限、非硬件保护**;硬件保护是 STSPIN830 DriverProtection(PA11/BKIN2)硬关断,阈值不可调。本机供电无可调限流、无保险丝(F-25)。
- **选项**:(A) 接受 0.5A 首测;(B) 回 MC Workbench 改 Iqmax/nominal current=0.3A 重生成+重编译;(C) 加硬件限流/保险丝(用户暂不可得)。
- **CC 建议 = A(接受 0.5A)**,理由:① Motor Profiler 已在 0.5A 实跑无恙;② 对空载 0.16A 的微型电机,0.3A vs 0.5A 差别小,却要多一轮 GUI 重生成+重编译;③ 首测另有三层兜底:STSPIN830 硬件 OCP 关断 + 100rpm 低速目标 + 用户 1 秒断电。以后弄到保险丝再补硬件那层。
- **状态**:**属用户风险决定**(用户在场、无保险丝),**首次带电前须用户最终拍板 A/B/C**;定了就在此条补记结论。Ke 量化(0.449→0.4)经 codex 查证不进 Hall FOC 运行时控制,故**不因 Ke 单独触发重生成**(与本决定独立)。
- **2026-07-13 补记结论:**用户最终选择 **B**。用户已在 MC Workbench 把 Iqmax/nominal current `0.5→0.3A`、默认目标 `500→100rpm`,并用内置 CubeMX 完成重生成;Codex 随后仅做离线 headless clean build 和源码差异复核。0.3A/100rpm 已进入生成固件,其余 Hall/三电阻/PWM/保护/MCP/Ke/无自动Start均保持不变,构建 `0 errors,0 warnings`(F-36)。因此本条不再待定;无可调限流/保险丝继续作为现场残余硬件风险,不得把0.3A软件限幅当短路保护。
