# Codex 任务书 — G431RB + IHM16M1 硬件/通信/工具链冒烟测试(只读 · 不烧录 · 不转电机)

> 给 Codex:读完即可自主执行。项目根 = `/home/cheyh/projs/embedded/STM32/Nucleo-G431RB/IHM16M1_test`(在 monorepo `embedded` 内)。
> 真相以仓库 `docs/` 为准:硬件事实 `docs/FACTS.md`(F-1~F-10)、设备探测 `docs/OPERATIONS.md` §0、路线 `docs/DECISIONS.md` #003。**动手前先读它们。**

## ⛔ 红线(最高优先,违反即停)
当前**电机可能带电、板子可能无人值守**。本任务**全程只读 + 离线编译**:
- **绝对禁止**:烧录/下载(任何 write/download/program)、mass erase、擦写 flash/option bytes、复位后运行用户程序(run/start)、任何可能输出 PWM / 驱动电机的动作。
- **只允许**:① ST-Link **只读**连接(读 IDCODE/flash size/option bytes,不改任何字节);② 在磁盘**生成+编译**最小工程(产物留磁盘,**不下载到板**);③ 只读列举串口/USB。
- 连 SWD 用**不运行用户代码**的方式(hot-plug 或 connect-under-reset 均可,但**不要 `-rst` 后启动、不要 start application**)——只读芯片身份,不改 flash、不让 MCU 跑起来。
- 任何一步若**必须写/烧/转电机**才能继续 → **立即停,把情况写进报告,等用户**。

## 0. 权限预检(最先做,一次性)
把会触发权限弹窗的**只读**动作各跑一个最小探针,让值守用户一次性批准:
- 设备:`STM32_Programmer_CLI -l`、`ls -l /dev/serial/by-id/`
- 工具链:`arm-none-eabi-gcc --version`(CubeCLT 内)、`make --version`、确认 CubeMX 可执行存在
- (如需查文档)网络探针:`curl -sSI https://www.st.com`
先列出你判断会弹窗的动作,逐项探通,全过再进 §1。

## 1. 已验证事实(直接用,详见 docs/FACTS.md)
- 板 = NUCLEO-G431RB,MCU STM32G431RB,**Device ID `0x468`**(F-1/F-4)。
- 调试器 = **STLINK-V3**(USB `0483:374e`),**本板 SN = `002A00403234510E33353533`**(F-3)——用它确认连的就是本板。
- 本机工具链(F-5/F-10,**均已装**):
  - **STM32CubeCLT 1.18.0** `/opt/st/stm32cubeclt_1.18.0`(含 `STM32_Programmer_CLI` v2.20.0、`openocd`、`arm-none-eabi-gcc`、`make`)
  - **STM32CubeMX 6.15.0** `~/STM/STM32CubeMX/STM32CubeMX`
  - **STM32Cube_FW_G4 V1.6.1** `~/STM32Cube/Repository`
- **端口不固定**:每次现场扫,按 SN 从 `/dev/serial/by-id/` 解析当前 `ttyACM`(OPERATIONS §0)。**同机常并存 Espressif 板(VID `303a`)占一个 ttyACM,别拿错**。
- **MCSDK 未装**(F-9)→ 本任务**不涉及电机标定/FOC**,只验证底座硬件/通信/工具链。

## 2. 你(codex)自己查(别让 CC 代查)
- `STM32_Programmer_CLI` **只读**连接读芯片信息的确切命令(读 IDCODE/flash size/option bytes,**不写不擦不 run**)。
- **STM32CubeMX 命令行/headless 脚本**(`-q <script>`)怎么为 NUCLEO-G431RB 生成最小工程(默认时钟 + 板载 LED **LD2=PA5** GPIO 输出,可选 USART2 作 VCP),输出 Makefile 工程。
- 用 CubeCLT 的 `arm-none-eabi-gcc` + `make` 编译该工程的方式。

## 3. 执行阶段
0. §0 权限预检。
1. **设备探测**:`STM32_Programmer_CLI -l` → 确认 Board=NUCLEO-G431RB、SN=F-3、Device ID=0x468;按 SN 从 by-id 解析当前 VCP 端口(只记录,不打开收发)。
2. **ST-Link↔MCU 通信验证(只读)**:连 SWD 读 IDCODE/flash size/option bytes。读到即代表调试通信链路 OK。**不擦不写不 run**。
3. **工具链编译验证**:CubeMX headless 生成最小 G431 工程(LED GPIO + 可选 USART2)→ CubeCLT 工具链 `make` 编译 → 产出 `.elf`/`.bin` **留磁盘**。**不烧录**。
4. 写报告。

## 4. 测试目标(测什么;怎么测你设计)
1. **探针层**:ST-Link 在线、SN=F-3、Device ID=0x468。
2. **调试通信层**:SWD 只读连接成功读到 IDCODE/flash/option bytes。
3. **工具链层**:最小 G431 工程用本机 CubeMX+CubeCLT **编译通过**,产出 `.elf`。
4. **串口枚举**:by-id 里存在含本板 SN 的 VCP 链接(只确认存在,不收发)。
5. 任一项失败 → 记录完整现象(命令+输出+报错),不硬试、不越红线。

## 5. 产出
- 测试 procedure + 每项实测结果(命令+输出摘要)写进 `docs/`(新增 `docs/HW_SMOKE_TEST.md`),更新 `docs/STATUS.md`。
- 最小工程放 `hw_smoke/`(或 `test/g431_smoke/`),`.gitignore` 掉构建产物;CubeMX 脚本与工具链路径记入 docs(可复现)。

## 6. 纪律/边界
- 红线见顶部:**只读 + 离线编译,不烧/不擦/不 run/不转电机**。
- 改动限本项目目录,别碰 monorepo 其它项目。
- 端口/SN 现场探测,别写死。
- 不确定就停下问,别为"跑通"越界。
