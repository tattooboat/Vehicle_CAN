# STM32F103 CAN 发送端工程（Vehicle CAN · Slave）

基于 STM32F103C8 + FreeRTOS 的车辆 CAN 双节点工程之**发送端（F103）**，配套 STM32F407 主控构成 CAN 双节点链路。F103 通过 CAN 定期发送 `ID=0x301` 电机控制帧，并可在测试阶段构造非法帧注入，用于验证 F407 侧的 CAN 故障检测与安全控制。本 README 汇总 Day 1 ~ Day 13 的搭建、改动与测试进展，并链接各 Day 文档。

> 对端 F407 主控详见 `../P01_stm32f407_hal` 工程的 README。

## 1. 项目简介

- 发送端：STM32F103C8（F103），FreeRTOS 任务周期发送电机控制帧；测试时可按需构造 DLC/帧头/命令/速度等非法帧。
- 主控：STM32F407VGT6（F407），负责 CAN 接收解析、故障检测、电机 PWM 控制与传感器采集。
- 控制链路：F103 → CAN 总线 → F407 接收中断逐级校验 → 电机任务 → TB6612FNG → 直流电机。

## 2. 开发目录结构

| 目录 | 说明 |
|---|---|
| `Core/Src/` | CubeMX 生成的底层初始化（clock、gpio、can 等）与 `freertos.c`（`CAN_Motor_Test_Task` 任务） |
| `Drivers/` | ST HAL 驱动 |
| `MDK-ARM/Application/` | 应用层：`App_MotorCan`（电机帧发送）、`App_can`（APP_CAN 协议编解码） |
| `MDK-ARM/Interface/` | 接口层：`Int_can`（CAN 过滤、收发中断、帧缓冲） |
| `MDK-ARM/`（根） | Keil MDK 工程文件（`.uvprojx`、`.uvoptx` 等） |

## 3. FreeRTOS 任务（freertos.c）

| 任务 | 优先级 | 周期 | 职责 |
|---|---|---|---|
| `CAN_Motor_Test` | Normal | ~50ms | 周期发送 `0x301` 电机控制帧；测试阶段可切换为构造非法帧 |
| `defaultTask` | Normal | - | 空壳默认任务 |

## 4. 电机控制帧协议（ID = 0x301，DLC = 8）

- `Data[0] = 0xA5`（帧标记）
- `Data[1] = 0x01`（SET 设置）/ `0x02`（STOP 停止）
- `Data[2..3] = 速度`（有符号 16 位，**高字节在前**，范围 `-1000 ~ 1000`）
- `Data[4..7]` 保留

F103 发送速度时按大端放置：高字节在 `Data[2]`、低字节在 `Data[3]`，与 F407 解析方式一致。

## 5. 发送接口（App_MotorCan）

| 接口 | 说明 |
|---|---|
| `App_MotorCan_SendSpeed(speed)` | 发送合法 SET 帧 |
| `App_MotorCan_SendStop()` | 发送合法 STOP 帧 |
| `App_MotorCan_SendTestFrame(cmd, speed, dlc)` | 测试：可控 DLC，构造帧长度错误帧 |
| `App_MotorCan_SendRawFrame(data, dlc)` | 测试：按字节指定内容与 DLC，构造各类非法帧 |

## 6. CAN 故障检测与安全控制（Day 13）

F103 发送端配合 F407 侧的逐级校验（帧长度 → 帧头 → 命令 → 速度范围）作故障注入。F407 故障标志（`App_Fault.h`，32 位按位叠加）：

| 标志 | 值 | 触发条件 |
|---|---|---|
| `FAULT_NONE` | 0x00 | 无故障 |
| `FAULT_CAN_TIMEOUT` | 0x01 | 500ms 未收到有效控制帧 |
| `FAULT_CAN_BAD_FRAME` | 0x02 | 帧长度 DLC≠8，或 Data[0]≠0xA5 |
| `FAULT_CAN_BAD_COMMAND` | 0x04 | Data[1] 非 0x01/0x02 |
| `FAULT_CAN_BAD_SPEED` | 0x08 | 速度超出 -1000 ~ 1000 |

> 测试提醒：非法帧测试期间若中断有效帧，F407 会在故障位基础上叠加 `TIMEOUT`(0x01)，例如 BAD_FRAME 显示为 0x03；只要包含对应位即判定通过。发送非法帧时穿插合法停止帧可稳定观察单一故障位。

## 7. 开发进度（Day 记录）

| Day | 主题 | 文档 |
|---|---|---|
| Day 1~5 | 环境搭建、STM32 CubeMX 初始化、CAN 外设配置与双机联通 | （详见 F407 端 README） |
| Day 6 | CAN 工程整理与基础收发接口建 | [Day6_CAN工程整理记录.md](../P01_stm32f407_hal/Day6_CAN工程整理记录.md) |
| Day 9 | PWM 与 TB6612FNG 电机驱动（F407 侧） | [Day9_PWM_TB6612FNG总结.md](../P01_stm32f407_hal/Day9_PWM_TB6612FNG总结.md) |
| Day 13 | CAN 故障检测与安全控制（非法帧测试矩阵、故障标志叠加分析） | [Day13_CAN故障检测与安全控制.md](Day13_CAN故障检测与安全控制.md) |

## 8. 下一步计划

- 为 F407 的 `g_fault_flags` 增加串口 / LED 可视化上报，F103 负责注入、无需调试器即可观察。
- 在本工程实现自动化非法帧注入脚本，覆盖 DLC/帧头/命令/速度的边界值做回归。
- 加入故障后安全停机策略（故障持续置位时强制停转并告警）。