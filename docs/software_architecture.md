# 软件架构

## 1. 总体分层

```text
┌─────────────────────────────────────────────────────────────┐
│ 应用/任务层                                                  │
│ F407: Motor_CAN、Sensor_Task、Debug_Task、LED_Task           │
│ F103: CAN_Motor_Test_Task                                    │
├─────────────────────────────────────────────────────────────┤
│ 应用逻辑层                                                   │
│ App_MotorCan、App_Fault、App_SpeedPID（未调用）               │
│ App_MotorControl、App_can_protocol、App_data                  │
│ App_can（F103 旧协议，当前未调用）                            │
├─────────────────────────────────────────────────────────────┤
│ 接口/驱动层                                                   │
│ Int_can、Int_motor、Int_Encoder、Int_aht20                     │
│ Int_adc/led/usart 为空壳保留                                  │
├─────────────────────────────────────────────────────────────┤
│ STM32 HAL / CubeMX 初始化                                     │
│ main.c、can.c、tim.c、gpio.c、usart.c、i2c.c、adc.c           │
├─────────────────────────────────────────────────────────────┤
│ FreeRTOS + CMSIS-RTOS2                                        │
└─────────────────────────────────────────────────────────────┘
```

目录结构没有改动。应用代码仍在各自工程的 `MDK-ARM/Application/`，接口代码仍在 `MDK-ARM/Interface/`，CubeMX 生成代码仍在 `Core/`。

## 2. 关键文件

| 文件 | 职责 |
|---|---|
| `F407_Controller/Core/Src/main.c` | HAL、时钟、外设初始化，调用 `INT_CAN_Start()`，启动 FreeRTOS |
| `F407_Controller/Core/Src/freertos.c` | F407 任务创建；`Motor_CAN` 内联 PI、目标 RPM 换算和调试输出 |
| `F407_Controller/MDK-ARM/Interface/Int_can.c` | CAN1 过滤器、启动、发送、RX0 中断回调、帧读取 |
| `F407_Controller/MDK-ARM/Interface/Int_motor.c` | TB6612 方向、STBY、TIM3_CH3 PWM 输出 |
| `F407_Controller/MDK-ARM/Interface/Int_Encoder.c` | TIM8 编码器启动、计数差读取、输出轴 RPM 换算 |
| `F407_Controller/MDK-ARM/Interface/Int_aht20.c` | AHT20 I2C 读取 |
| `F407_Controller/MDK-ARM/Application/App_MotorCan.c` | `0x301` 帧校验、CAN 目标速度和最后接收时刻；不包含编码器依赖 |
| `F407_Controller/MDK-ARM/Application/App_SpeedPID.c` | 通用 PI 模块；已加入工程但当前 `Motor_CAN` 未调用 |
| `F407_Controller/MDK-ARM/Application/App_Fault.c` | 故障位和 CAN 计数 |
| `F407_Controller/MDK-ARM/Application/App_MotorControl.c` | 限幅后调用 `Int_Motor_SetSpeed()` 的包装层；当前 Motor_CAN 任务直接调用 `Int_motor`，该包装层未进入主控制路径 |
| `F407_Controller/MDK-ARM/Application/App_can_protocol.c` | `0x100~0x201` 旧小协议监视，不驱动电机 |
| `F407_Controller/MDK-ARM/Application/App_data.h` | `SensorData_t` 传感器数据结构 |
| `F103_Sender/Core/Src/freertos.c` | F103 CAN 周期发送任务 |
| `F103_Sender/MDK-ARM/Application/App_MotorCan.c` | `0x301` 合法帧和测试帧发送 |
| `F103_Sender/MDK-ARM/Interface/Int_can.c` | F103 CAN 启动、发送、接收中断和最近一帧缓存 |
| `F103_Sender/MDK-ARM/Application/App_can.c` | `0x123/0x124` 旧协议编解码；当前 `APP_CAN_Process()` 未被任务调用 |

`Int_adc.c`、`Int_led.c`、`Int_usart.c` 当前为空文件，ADC、LED、UART 的逻辑直接写在 `freertos.c` 和 HAL 初始化中。

## 3. F407 FreeRTOS 任务

| 任务 | 优先级 | 栈大小 | 周期/阻塞方式 | 职责 |
|---|---|---|---|---|
| `Motor_CAN` | `osPriorityHigh` | 1024×4 = 4096 B | 50 ms | 读取 CAN 目标、映射目标 RPM、读取 TIM8 编码器实际 RPM、执行内联 PI、超时停止、PWM 输出和调试打印 |
| `Sensor_Task` | `osPriorityNormal` | 768×4 = 3072 B | 500 ms | 读取 PA0 ADC 和 AHT20，打包 `SensorData_t` 放入队列 |
| `Debug_Task` | `osPriorityNormal` | 768×4 = 3072 B | `osWaitForever` | 从 8 深度传感器队列取数据，通过 USART1 打印 |
| `LED_Task` | `osPriorityLow` | 256×4 = 1024 B | 500 ms | 翻转 PA5 LED |
| `defaultTask` | `osPriorityNormal` | 128×4 = 512 B | 1 ms | 空壳默认任务 |

FreeRTOS 配置为 1000 Hz tick、抢占式调度；F407 堆大小为 15360 B。

`Motor_CAN` 的闭环参数：

```c
#define MOTOR_MAX_RPM       169.0f
#define SPEED_PID_KP        10.0f
#define SPEED_PID_KI        1.0f
#define SPEED_PID_I_LIMIT   400.0f
#define SPEED_PID_PWM_MAX   1000.0f
#define SPEED_PID_DT        0.05f
```

任务用 `HAL_GetTick()` 计算实际 `dt`，限制在 `0.01~0.20 s`；异常时回退到 `0.05 s`。CAN 目标为 0 时清除积分并调用 `Int_Motor_Stop()`；目标变化超过 5 RPM 时重置积分。当前实际生效的是 `freertos.c` 的内联 PI，`App_SpeedPID.c` 的参数和实现不能当成当前运行参数。

## 4. F103 FreeRTOS 任务

| 任务 | 优先级 | 栈大小 | 周期 | 职责 |
|---|---|---|---|---|
| `CAN_Motor_Test_Task` | `osPriorityNormal` | 128×4 = 512 B | 50 ms 发送 | 按 `+100 → +200 → +100 → STOP` 循环，每档约 40 s |
| `defaultTask` | `osPriorityNormal` | 128×4 = 512 B | 1 ms | 空壳默认任务 |

F103 堆大小为 3072 B，tick 为 1000 Hz。

## 5. CAN 接收中断到电机的路径

```text
F103 CAN_Motor_Test_Task
        │ App_MotorCan_SendSpeed(100/200/...)
        ▼
MotorCan_SendFrame()
        │ HAL_CAN_AddTxMessage()
        ▼
CAN 总线（250 kbit/s）
        │
        ▼
F407 CAN1 RX0 中断
        │ HAL_CAN_RxFifo0MsgPendingCallback()
        │ 循环读取 FIFO0
        ├─ CAN_Protocol_OnFrame()       // 旧协议监视
        └─ App_MotorCan_OnRx()          // 标准数据帧 0x301
        │ 校验 len/Data[0]/Data[1]/速度
        │ 写 target_speed、last_rx_tick、故障位
        ▼
Motor_CAN 任务（每 50 ms）
        │ target_rpm = can_speed / 1000 × 169
        │ actual_rpm ← Int_Encoder_GetOutputRPM(dt) ← TIM8 PC6/PC7
        │ 超时检查 → PI（Kp/Ki、积分限幅）→ PWM 限幅
        ▼
Int_Motor_SetSpeed()
        │ AIN1/AIN2 + TIM3_CH3 CCR
        ▼
TB6612FNG → 直流电机
```

### 5.1 中断里做了什么

`HAL_CAN_RxFifo0MsgPendingCallback()` 在 `Int_can.c` 中：

1. 确认中断来自 CAN1；
2. 只要 FIFO0 非空就继续读；
3. 把每帧交给 `CAN_Protocol_OnFrame()` 做旧协议监视；
4. 如果帧是标准数据帧、`StdId==0x301`，再调用 `App_MotorCan_OnRx()`；
5. 更新 `g_can_last_id`、`g_can_last_dlc`、`g_can_last_data` 和 `g_can_rx_count`。

中断回调中没有阻塞延时、没有队列等待，也没有直接启动 PWM；这些动作留给任务。

### 5.2 任务和中断之间传递什么

`App_MotorCan.c` 中定义：

```c
static volatile int16_t target_speed = 0;
static volatile uint32_t last_rx_tick = 0;
```

中断写 `target_speed` 和 `last_rx_tick`，任务读；实际 RPM 和 PI 状态只在 `Motor_CAN` 任务内更新。`target_speed` 是 16 位、`last_rx_tick` 是 32 位，在 Cortex-M 上对齐访问是原子的；当前没有使用队列、信号量或事件组。

这种做法的优点是简单、延迟低；限制是：

- 目标和时间戳不是作为一个整体原子更新，任务极端情况下可能读到相邻两帧的组合；
- 如果以后增加更多状态，建议改成消息队列或带序号的结构；
- `g_fault_flags` 的中断/任务读改写已经用保存/恢复 PRIMASK 的临界区保护，避免任务清故障位时把中断刚置上的另一位覆盖掉。

## 6. 故障状态机

```text
上电/从未收到合法帧
        │
        ▼
FAULT_CAN_TIMEOUT ── 收到合法 SET/STOP ──► 清除全部 CAN 故障
        ▲
        │ 超过 500 ms 无合法帧
        │
合法帧保持运行
        │
        ├─ len != 8 或 Data[0] != 0xA5 → BAD_FRAME
        ├─ Data[1] 非 0x01/0x02        → BAD_COMMAND
        └─ SET 速度越界                → BAD_SPEED
```

故障位由 `App_Fault_Set()` / `App_Fault_Clear()` 更新。当前实现中这两个函数会保存 PRIMASK、关中断、修改 `g_fault_flags`、再恢复 PRIMASK，因此可以被中断和任务共同调用。

`g_can_rx_count` 在 `Int_can.c` 中对收到的每一帧加 1；`g_can_invalid_count` 在 `App_MotorCan.c` 中对非法 `0x301` 帧加 1。旧协议监视器 `g_can_protocol_monitor` 另有自己的计数。

## 7. 传感器和调试链

```text
Sensor_Task (500 ms)
    ├─ ADC_ReadRaw() → PA0，12 位
    ├─ AHT20_ReadData() → I2C1 PB6/PB7
    └─ SensorDataQueue (8 条)
                │
                ▼
        Debug_Task (阻塞等待)
                │
                ▼
        USART1 PA9/PA10，115200
```

`Sensor_Task` 在读取失败时会把 `aht20_valid` 置 0，`Debug_Task` 打印 `AHT20=ERROR`。`main.c` 中的 `AHT20_Init()` 调用目前被注释掉，因此当前代码路径没有显式初始化 AHT20；这是需要在硬件复测时确认的点。

`Debug_Task` 和 `Motor_CAN` 都会调用 `HAL_UART_Transmit()` 访问同一个 `huart1`，当前没有互斥保护。`Motor_CAN` 打印 `TgtRPM/ActRPM/Err/I/PWM`，`Debug_Task` 打印 ADC/AHT20；电机任务优先级更高，理论上可能在调试任务发送过程中抢占并写入串口，导致日志交错。

## 8. 旧代码和未使用路径

- `App_can_protocol.c` 的 `0x100/0x101/0x200/0x201` 只更新监视结构体，不控制电机。
- `App_can.c` 的 `0x123/0x124` 只在 F103 工程中编译，当前任务没有调用 `APP_CAN_Process()`。
- `App_MotorControl.c` 提供了限幅和停止包装，但 `Motor_CAN` 任务当前直接调用 `Int_Motor_SetSpeed()`；保留它是为了后续统一应用层接口。
- `App_SpeedPID.c/h` 已加入工程但未被 `Motor_CAN` 调用；当前生效的是 `freertos.c` 内联 PI。阶段三必须二选一：接入该模块，或删除/明确归档。
- `Int_Encoder_Init()` 目前没有检查 `HAL_TIM_Encoder_Start()` 返回值，也没有编码器断线/无脉冲故障位；目标非零而反馈为 0 时 PI 可能继续加大输出，阶段二必须补失效保护。
- `Int_adc.c`、`Int_led.c`、`Int_usart.c` 当前为空，实际逻辑在 `freertos.c` 和 HAL 初始化中。
- TIM3_CH1/PA6 已在 CubeMX 中配置，但当前没有用于 TB6612 的 PWMA。

## 9. 代码整理记录

本次只做了不改变控制流程的整理：

- 去掉 `App_MotorCan_OnRx()` 中对 `g_can_rx_count` 的重复自增，避免 `0x301` 帧被统计两次；
- 给 `App_Fault_Set()` / `App_Fault_Clear()` 增加 PRIMASK 临界区，避免中断和任务同时读改写故障位时丢位；
- 补齐 F103 应用文件的末尾换行，减少构建警告；
- 清理 F407 `freertos.c` 和 `App_MotorCan.c` 中的乱码注释，控制逻辑不变；
- 让 `App_MotorCan` 不再依赖 `Int_Encoder`，由控制任务显式包含编码器接口；
- `Int_Encoder.c` 使用 `11 × 4` 和 `35.5` 宏，删除重复的硬编码常量。

整理后 F407 全量重建 **0 error / 0 warning**，Code=30892 B；F103 全量重建 **0 error / 2 warning**，Code=14784 B（仅 FreeRTOS 第三方头文件末尾换行）。
