# STM32F407 车辆 CAN 控制工程（Vehicle CAN）

基于 STM32F407VGT6 + FreeRTOS 的车辆电机 CAN 控制工程，配套 STM32F103（发送端）构成 CAN 双节点。本 README 汇总 Day 1 ~ Day 13 的硬件搭建、软件改动、测试进展，并链接各 Day 详细文档。

## 1. 项目简介

- 主控：STM32F407VGT6（F407），负责 CAN 接收解析、故障检测、电机 PWM 控制，并采集 ADC / AHT20 传感器。
- 发送端：STM32F103（F103），通过 CAN 总线定期发送控制帧（含合法帧、故障测试用的非法帧）。
- 实时系统：FreeRTOS（CMSIS-RTOS2），多任务调度驱动 LED、传感器采集、调试打印与电机控制。
- 控制链路：F103 → CAN 总线 → F407 接收中断解析/校验 → 电机任务 → TB6612FNG → 直流电机。

## 2. 开发目录结构

| 目录 | 说明 |
|---|---|
| `Core/Src/` | CubeMX 生成的底层初始化（clock、gpio、can、tim 等）与 `freertos.c`（任务） |
| `Drivers/` | ST 标准固件库 / HAL 驱动 |
| `MDK-ARM/Application/` | 应用层：`App_MotorCan`、`App_Fault`、`App_MotorControl`、`App_can_protocol`、`App_data` |
| `MDK-ARM/Interface/` | 接口层：`Int_can`、`Int_motor`、`Int_usart`、`Int_led`、`Int_adc`、`Int_aht20` |
| `MDK-ARM/`（根） | Keil MDK 工程文件（`.uvprojx` 等） |

## 3. FreeRTOS 任务（Day 9 之后，`freertos.c`）

| 任务 | 优先级 | 周期 | 职责 |
|---|---|---|---|
| `Motor_CAN`（App_MotorCan_ControlTask） | High | 50ms | 独占电机控制：读取解析结果 → 目标速度 → 渐变调速 → PWM；500ms 无有效帧则归零并置 CAN 超时故障 |
| `Sensor_Task` | Normal | 500ms | 采集 PA0 ADC 与 AHT20 温湿度，打包为 `SensorData_t` 入队 |
| `Debug_Task` | Normal | 阻塞 | 从队列取数，通过串口1打印 `ADC/V/温度/湿度` |
| `LED_Task` | Low | 500ms | 周期翻转 LED |
| `defaultTask` | Normal | - | 空壳默认任务 |

> PWM 测试任务（`PWM_Test_Task`）已停用，电机控制统一由 `Motor_CAN` 任务独占，避免冲突。

## 4. 传感器数据（`App_data.h`）

```c
typedef struct {
    uint32_t adc_raw;        // PA0 ADC 原始值（12 位）
    float    adc_voltage;    // V = raw * 3.3 / 4095
    float    temperature;    // AHT20 温度
    float    humidity;       // AHT20 湿度
    uint32_t timestamp_ms;   // 采样时间戳
    uint8_t  aht20_valid;    // 本次温湿度是否有效
} SensorData_t;
```
由 `Sensor_Task` 生产，经消息队列 `SensorDataQueue` 交给 `Debug_Task` 消费。

## 5. CAN 通信与电机控制协议

- **电机控制帧**：标准数据帧，`ID=0x301`，`DLC=8`。
  - `Data[0] = 0xA5`（帧标记）
  - `Data[1] = 0x01`（SET 设置）/ `0x02`（STOP 停止）
  - `Data[2..3] = 速度`（有符号 16 位，高字节在前，范围 `-1000 ~ 1000`）
  - `Data[4..7]` 保留
- 接收流程：`HAL_CAN_RxFifo0MsgPendingCallback` → 过滤出 `0x301` → `App_MotorCan_OnRx` 逐级校验 → 更新目标速度 / 故障标志。

## 6. 故障检测与安全控制（Day 13）

故障标志位于 `App_Fault.h`，32 位按位叠加（可同时存在多个故障）：

| 标志 | 值 | 触发条件 |
|---|---|---|
| `FAULT_NONE` | 0x00 | 无故障 |
| `FAULT_CAN_TIMEOUT` | 0x01 | 500ms 未收到有效控制帧 |
| `FAULT_CAN_BAD_FRAME` | 0x02 | 帧长度 DLC≠8，或 Data[0]≠0xA5 |
| `FAULT_CAN_BAD_COMMAND` | 0x04 | Data[1] 非 0x01/0x02 |
| `FAULT_CAN_BAD_SPEED` | 0x08 | 速度超出 -1000 ~ 1000 |

校验顺序：帧长度 → 帧头 → 命令 → 速度范围。非法帧直接 `return`，不更新速度、不视为有效帧；电机保持安全状态、不误转。收到有效帧会清除 CAN 相关全部故障。

> 测试提醒：非法帧测试期间若中断有效帧，g_fault_flags 会在故障位基础上叠加 `TIMEOUT`(0x01)，例如 BAD_FRAME 显示为 0x03；只要包含对应位即校验通过。

## 7. 开发进度（Day 记录）

| Day | 主题 | 文档 |
|---|---|---|
| Day 6 | CAN 外设配置与工程整理（CAN1、过滤器、基础收发接口，目录整理） | [Day6_CAN工程整理记录.md](Day6_CAN工程整理记录.md) |
| Day 9 | PWM 与 TB6612FNG 电机驱动（TIM3 调速、正反转、停止、STBY 控制） | [Day9_PWM_TB6612FNG总结.md](Day9_PWM_TB6612FNG总结.md) |
| Day 13 | CAN 故障检测与安全控制（非法帧测试矩阵、故障标志叠加分析） | [Day13_CAN故障检测与安全控制.md](Day13_CAN故障检测与安全控制.md) |

## 8. 下一步计划

- 为 `g_fault_flags` 增加串口 / LED 可视化上报，测试无需调试器即可观察。
- 用 F103 构造自动化非法帧注入脚本，覆盖 DLC/帧头/命令/速度的边界值做回归。
- 加入故障后安全停机策略（故障持续置位时强制停转并告警）。
- 完善 Day6 文档中待填写的硬件与测试结果项。