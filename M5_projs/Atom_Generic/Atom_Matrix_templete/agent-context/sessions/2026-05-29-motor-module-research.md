# 2026-05-29 — 电机模块文档调研与方案确认（未编码）

## 本次进展
- 确认电机 = M5 **RollerCAN**，用 **I²C** 控制（用户只用 I²C，默认地址 0x64）。
- 用本地 `pdftotext` 解析了 Unit-RollerCAN I²C Protocol PDF，提取出帧协议、命令码、定标、CRC。
- 发现该 PDF 实为帧/串口协议；I²C 实际用法以官方库为准。
- 克隆 `m5stack/M5Unit-Roller`，确认 `UnitRollerI2C` 类、默认地址 0x64、Grove 接线、全部 set/get API。
- 加 `lib_deps` = `https://github.com/m5stack/M5Unit-Roller.git`（注册表无，用 git URL），安装验证通过。
- 确认控制方案：**电流(力矩)模式**为主，速度仅用于去饱和；明确状态读取项与初始化顺序。
  详见 [protocols/rollercan-i2c.md](../protocols/rollercan-i2c.md)、[decisions/002](../decisions/002-motor-control-strategy.md)。

## 当前状态
- 板上固件仍是第 1 步 IMU 姿态输出（未变）。
- 电机模块**尚未编码**，`motor.*` 未建；用户明确要求确认方案后、给具体需求前不编码。
- `platformio.ini` 已加 M5Unit-Roller 依赖。

## 未完成 / 待办
- [ ] 等用户给电机模块**具体需求** → 再建 `motor.*`（独立文件，按模块化约定）。
- [ ] 实现：电流模式初始化 + 力矩下发(±1200mA 饱和) + 状态读取 + 去饱和。
- [ ] 姿态融合（pitch 加陀螺仪互补/卡尔曼）后接入平衡控制器。

## 下一步
等用户给电机模块具体需求。

## 备注 / 坑
- 本机 GitHub API 未认证、会限流；读 GitHub 仓库用 `git clone --depth 1` 最稳。
- M5Unit-Roller 在 PlatformIO 注册表查不到，必须用 git URL 引用。
- 工具：`pdftotext`(poppler) 可解析协议 PDF；WebFetch 对压缩 PDF 解析不可靠。
