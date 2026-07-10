# GITIGNORE_AND_REGEN — 被忽略的生成物 · 重生成覆盖时如何保住改动

> 给任何(尤其 codex)要改动本项目"被 gitignore 忽略的生成工程"的人。
> 核心矛盾:**被忽略 = 无 git 备份;CubeMX / MC Workbench 重生成 = 覆盖生成文件。** 两者叠加 → 手动改动可能**既丢不了备份、又被重生成抹掉**。本文件说清忽略了什么、改动时怎么保、覆盖后怎么恢复。
> 忽略规则的权威在 [`../.gitignore`](../.gitignore);本文件是它的**解释 + 维护协议**,改忽略策略时**两处同步**。

## 1. 当前忽略了什么(镜像 .gitignore)
| 忽略项 | 是什么 | 有无"手改需备份/重生成覆盖"风险 | 如何复现 |
|---|---|---|---|
| `**/build/` `**/Debug/` `**/Release/` `*.elf/.bin/.hex/.map/.list/.o/.d` | 编译产物 | 无(纯产物,重编即得) | 重新编译 |
| `hw_smoke/g431_hw_smoke/`、`hw_smoke_win/g431_hw_smoke_win/` | CubeMX 冒烟工程(生成) | 低(诊断用,种子在) | 种子 `.ioc` + `cubemx_generate*.txt` / `build_win.ps1` |
| **`GM16020-06_profile/`** | **MC Workbench 生成的 Motor Profiler 工程**(HAL/CMSIS/MCSDK 中间件/`libMP/*.a`/CubeIDE 工程,数 MB) | **有 —— 见 §2/§3** | **`.stwb6` 种子 + MC Workbench GUI 重生成**(见 [`MOTOR_PROFILER_SOP.md`](MOTOR_PROFILER_SOP.md) §4.5) |
| `hw_ref/ihm16m1/source/` | UM2415 等手册 PDF 原件(大文件、版权) | 无(有 URL) | 从 st.com 重新下载(URL 见 `IHM16M1_SHUNT_CONFIG.md`) |
| `.settings/` `*.launch` `.DS_Store` `Thumbs.db` | IDE/OS 本地文件 | 无 | 无需 |

**保留在库的"种子/锚点"(不忽略):** `*.stwb6`(MC Workbench 工程,重生成的唯一输入)、`*.ioc` 冒烟种子、`cubemx_generate*.txt`、`build_win.ps1`、本 `docs/`。

## 2. 风险模型(动手前必读)
- 生成工程(如 `GM16020-06_profile/`)**不在 git 里** → 你对它的手改**没有版本历史**,误删/覆盖后无法 `git restore`。
- 用 MC Workbench / CubeMX **重生成**该工程 → **覆盖生成文件**。唯一天然幸存的是 CubeMX 的 **`USER CODE BEGIN … USER CODE END` 之间**的内容;此标记之外的改动、以及被整体重写的文件,**一律丢失**。
- 结论:对生成工程的任何"需要保留"的改动,**必须显式保护**(§3),否则默认视为**易失**。

## 3. 改动生成工程时的协议(codex 必守)
按优先级选:

**A. 首选:只在 `USER CODE BEGIN/END` 之间改。**
CubeMX/MCSDK 重生成会**保留**这些块。改完仍要登记到 §4 台账(方便审阅/迁移),但无需额外备份。

**B. 若必须改生成文件的"非 USER CODE"部分,或新增文件 → 必须做备份,二选一:**
- **B1(推荐)取反解除忽略**:在 `.gitignore` 里把**该具体文件**放行入库。注意父目录被忽略时需**逐级放行**,例:
  ```gitignore
  # 放行 GM16020-06_profile/Src/main.c 备份
  !GM16020-06_profile/
  GM16020-06_profile/*
  !GM16020-06_profile/Src/
  GM16020-06_profile/Src/*
  !GM16020-06_profile/Src/main.c
  ```
  （只放行确需备份的少数文件,别把整棵大树又放回来。）
- **B2 备份副本**:把改过的文件复制到**已跟踪目录**(如新建 `profiler_overrides/`,镜像相对路径),入库;并在 §4 台账记明"源路径→备份路径"。重生成后按台账拷回/合并。

**C. 无论 A/B,都要在 §4 台账登记**:改了哪个文件、改了什么、为什么、是否在 USER CODE 内、用了哪种备份。**目的:重生成覆盖后能照台账把改动重新应用到新工程。**

**D. 重生成前后动作:**
1. 重生成前:确认 §4 台账是最新;必要时先按 B 备份。
2. 重生成(MC Workbench GUI,见 SOP §4.5)后:逐条对照 §4 台账,把 A 的 USER CODE 改动确认仍在、把 B 的备份改动**重新应用**到新生成的树。
3. 应用完:编译验证通过,再更新台账状态。

## 4. 手动改动台账(对生成工程 `GM16020-06_profile/` 的一切手改)
> 空表 = 目前**未对生成工程做任何手改**(它是 MC Workbench 原样生成)。一旦手改,**立即在此加行**。

| 日期 | 文件(相对工程根) | 改动摘要 | 原因 | 在 USER CODE 内? | 备份方式(A/B1/B2) | 重生成后状态 |
|---|---|---|---|---|---|---|
| — | — | (暂无) | — | — | — | — |

## 5. 维护纪律
- 改 `.gitignore` 忽略策略 → **同步更新本文件 §1**。
- 对生成工程任何手改 → **同步登记 §4 台账** + 按 §3 做备份。
- 本文件与 [`MOTOR_PROFILER_SOP.md`](MOTOR_PROFILER_SOP.md)(如何重生成)、[`SYNC_WORKFLOW.md`](SYNC_WORKFLOW.md)(双机 commit+push)配套。
