# Vehicle CAN：STM32F103 + STM32F407 分布式电机控制

## 项目简介

本项目用两块 STM32 通过 CAN 总线组成一个最小分布式电机控制系统：

- **F103 发送端**（STM32F103C8T6）：FreeRTOS 任务每 50 ms 发送一帧标准数据帧 `ID=0x301`；当前测试任务按 `+100 → +200 → +100 → STOP` 循环，每档约 40 s。代码中保留了非法帧注入接口，用于测试 F407 侧的校验和故障处理。
- **F407 主控端**（STM32F407VGT6）：在 CAN1 接收中断中解析 `0x301`，依次校验帧长、帧头、命令和速度范围；`Motor_CAN` 任务把 `-1000~1000` 的 CAN 指令映射为 `-169~169 RPM` 的目标转速，读取 TIM8 编码器的实际输出轴转速，经 PI 计算后输出 TIM3_CH3 PWM，驱动 TB6612FNG 和直流电机。
- **失效保护**：超过 500 ms 没有收到合法 `0x301` 帧时，目标转速归零并置位 `FAULT_CAN_TIMEOUT`；控制任务在下一个 50 ms 周期调用 `Int_Motor_Stop()`，同时清除积分项。这是软件停止，不是硬件急停。
- **辅助功能**：F407 还运行 ADC/AHT20 传感器采集、UART1 调试输出和 LED 闪烁任务。

当前实现是**编码器反馈的 PI 转速闭环**：

- CAN 的 `0~1000` 是指令值，按 `target_rpm = can_speed / 1000 × 169` 映射为目标 RPM；
- 编码器按“每电机转 44 计数、减速比 35.5”换算输出轴 RPM，这两个参数目前是标定假设，绝对 RPM 精度仍需实测校准；
- 当前实际运行的是 `freertos.c` 中的内联 PI，`App_SpeedPID.c` 虽然编译进工程但尚未被任务调用；
- 没有电流环、目标 RPM 斜坡或硬件急停；`+200 → +100` 的恢复过程来自积分重建，不是预设的渐变曲线。

## 已实现功能

| 功能 | 当前实现 |
|---|---|
| CAN 双节点通信 | F103 与 F407 均配置为 250 kbit/s；F103 周期发送 `0x301`，F407 使用 CAN1 RX0 中断接收；Day 13 记录已建立双节点通信 |
| 应用层协议 | 标准数据帧、11 位 ID `0x301`、DLC=8、帧头 `0xA5`、SET/STOP 命令、大端有符号 16 位速度 |
| 帧校验 | 帧长、帧头、命令、速度范围逐级校验；非法帧不更新目标速度和最后接收时刻 |
| 故障标志 | `TIMEOUT`、`BAD_FRAME`、`BAD_COMMAND`、`BAD_SPEED` 按位叠加，可由调试器观察 |
| 通信超时保护 | 500 ms 无合法帧后目标归零，置位 `FAULT_CAN_TIMEOUT`；下一个 50 ms 控制周期停止电机并清除积分 |
| 编码器测速 | TIM8 编码器模式（TI12），PC6/PC7 接编码器 A/B 相；中断/任务周期读取计数差并换算输出轴 RPM |
| 转速闭环 | 目标 RPM 与编码器实际 RPM 求误差，PI 输出限幅到 `±1000` PWM 指令；当前是单速度环，没有电流环 |
| 电机控制 | TIM3_CH3（PB0）输出 1 kHz PWM，PB12/PB13 控制方向，PB14 控制 TB6612 的 STBY |
| FreeRTOS | F407：电机控制、传感器采集、调试打印、LED；F103：CAN 周期发送 |
| 测试注入 | F103 提供 `App_MotorCan_SendTestFrame()` 和 `App_MotorCan_SendRawFrame()`，可构造 DLC、帧头、命令、速度异常帧 |

## 硬件概要

| 节点 | MCU | CAN 引脚 | 其他主要资源 |
|---|---|---|---|
| F407 主控 | STM32F407VGT6，LQFP100 | `PA11=CAN1_RX`，`PA12=CAN1_TX` | USART1 `PA9/PA10`（115200）、I2C1 `PB6/PB7`、ADC1_IN0 `PA0`、LED `PA5`、TB6612 `PB0/PB12/PB13/PB14`、编码器 TIM8 `PC6/PC7` |
| F103 发送端 | STM32F103C8T6，LQFP48 | `PB8=CAN_RX`，`PB9=CAN_TX`（CAN1 重映射） | SWD `PA13/PA14`；当前应用未使用其他外设 |

接线时两节点各使用一块 CAN 收发器，CANH/CANL 对接，并按 CAN 总线要求在上电前确认两端 120 Ω 终端电阻。收发器型号、开发板供电方式和电机型号在原工程记录中没有填写，详见 [hardware_wiring.md](docs/hardware_wiring.md)。

## 工程结构

```text
Vehicle_CAN/
├─ F103_Sender/                 # F103 发送端 Keil 工程
│  ├─ Core/                     # CubeMX 初始化、FreeRTOS 任务
│  ├─ MDK-ARM/Application/      # App_MotorCan、App_can
│  └─ MDK-ARM/Interface/        # Int_can
├─ F407_Controller/             # F407 主控 Keil 工程
│  ├─ Core/                     # CubeMX 初始化、FreeRTOS 任务
│  ├─ MDK-ARM/Application/      # 电机帧解析、PI、故障、数据、旧协议监视
│  └─ MDK-ARM/Interface/        # Int_can、Int_motor、Int_Encoder、Int_aht20 等
├─ docs/                        # 接线、协议、架构、测试、面试说明
├─ STM32_CAN_电机转速闭环控制阶段总结.md
└─ README.md
```

目录结构保持原样；Day 6/9/13 的原始过程记录仍保留在 `F103_Sender/` 和 `F407_Controller/` 下。

## 编译方法

本工程使用 **Keil MDK-ARM** 构建，历史构建记录使用的工具链为 Keil µVision 5.37、ARM Compiler 5.06 update 7。

1. 打开 `F407_Controller/MDK-ARM/F407_Controller.uvprojx`，选择目标 `F407_Controller`，执行 Build/Rebuild，确认生成 `F407_Controller.hex`。
2. 打开 `F103_Sender/MDK-ARM/F103_Sender.uvprojx`，选择目标 `F103_Sender`，执行 Build/Rebuild，确认生成 `F103_Sender.hex`。
3. 先给两块开发板和 CAN 收发器共地并上电，再分别通过 ST-Link/SWD 下载固件。
4. 用串口工具连接 F407 的 USART1，波特率 115200；电机任务大约每 500 ms 打印一次 `TgtRPM=... ActRPM=... Err=... I=... PWM=...`。

> 如果使用 CubeMX 重新生成代码，请先备份工程。F407 的 `.ioc` 中仍保留旧的工程名 `P01_stm32f407_hal`，F103 的 `.ioc` 名为 `F103_CAN_Slave.ioc`；重新生成前要确认目标文件名和用户代码区，避免覆盖手写应用代码。

## 构建与测试结果

以下结果来自本仓库的构建日志和已有测试记录，详细证据见 [test_report.md](docs/test_report.md)。

| 项目 | 结果 |
|---|---|
| F407 全量重建（PID 版本整理后） | **0 error，0 warning**；Code=30892 B，RO-data=728 B，RW-data=224 B，ZI-data=20192 B |
| F103 全量重建（PID 测试序列版本） | **0 error，2 warning**；Code=14784 B，RO-data=424 B，RW-data=176 B，ZI-data=7424 B；2 个警告均来自 FreeRTOS 第三方头文件缺少末尾换行 |
| TB6612 调速 | Day 9 记录：`200/500/800` 可改变电机转速，停止和反转可用 |
| 非法帧注入 | Day 13 记录：帧长、帧头、命令三项已做过注入；帧长错误实测看到 `0x02` 与 `0x03` 之间跳动，其中 `0x03 = BAD_FRAME | TIMEOUT` |
| 闭环 `+100` | 目标 16.9 RPM，记录到 `ActRPM=16.9 RPM`、误差接近 0 |
| 闭环 `+200` | 目标 33.8 RPM，记录到 `ActRPM=32.3 RPM`、误差约 1.5 RPM；最终稳定值待补 |
| 目标切换 `+200 → +100` | 曾出现约 12～13 RPM 下探，随后积分恢复；瞬态数据已记录，优化待做 |
| 速度越界 | **未实测**；Day 13 记录明确标注暂不测试 |
| F103 断电超时/恢复 | **未实测**；已给出复测步骤，见测试报告 |

## 文档索引

- [hardware_wiring.md](docs/hardware_wiring.md)：F103/F407 接线、CAN 收发器、TB6612 引脚和供电
- [can_protocol.md](docs/can_protocol.md)：`0x301` 帧格式、DLC、速度编码、STOP 和故障协议
- [software_architecture.md](docs/software_architecture.md)：驱动层、应用层、RTOS 任务和中断回调关系
- [test_report.md](docs/test_report.md)：测试步骤、预期结果、实际结果和异常处理
- [interview_notes.md](docs/interview_notes.md)：面向面试的简短技术说明和 7 个常见问题
- [STM32_CAN_电机转速闭环控制阶段总结.md](STM32_CAN_电机转速闭环控制阶段总结.md)：编码器/PI 调试过程、参数和修订后的阶段计划

## 当前限制

- F103 当前测试任务按 `+100/+200/+100/STOP` 循环，没有按键或上位机实时调速界面。
- 当前实际生效的是 `freertos.c` 内联 PI；`App_SpeedPID.c` 未被调用，阶段三必须统一实现，避免两套参数混淆。
- 没有目标 RPM 斜坡、电流环或硬件急停；目标大幅变化时只重置积分。
- 编码器每电机转 44 计数和减速比 35.5 是当前标定假设，绝对 RPM 精度尚未用已知转数或外部测速仪校准。
- 编码器失效/断线没有独立故障位；如果目标非零而反馈一直为 0，PI 可能继续加大 PWM。阶段二必须补编码器失效保护。
- `AHT20_Init()` 在 `main.c` 中被注释掉，传感器任务直接调用 `AHT20_ReadData()`；首次上电初始化路径未做专项验证。
- `App_can_protocol.c` 中的 `0x100/0x101/0x200/0x201` 小协议目前只做监视，不参与电机控制；F103 的 `App_can.c`（`0x123/0x124`）也没有被当前任务调用。
- 故障标志目前主要通过调试器观察，尚未做串口/LED 可视化上报。
- `Debug_Task` 和 `Motor_CAN` 共用 USART1 且没有互斥，日志可能交错。
- CAN 收发器型号、终端电阻实际状态、总线波形和误码率没有留存测量记录。
