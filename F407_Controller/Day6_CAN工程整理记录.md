# Day 6：CAN 外设配置与工程整理

## 1. 实验目标
- 配置 STM32F407 CAN1
- 完成 CAN 过滤器和基础发送接口
- 整理 BSP、Components、Application 目录

## 2. 硬件配置
- MCU：STM32F407VGT6
- CAN 外设：CAN1
- CAN1_TX 引脚：
- CAN1_RX 引脚：
- CAN 收发器型号：
- CAN 波特率：
- CAN 总线节点数量：

## 3. 软件改动
- 新增 BSP CAN 接口
- 保留 CubeMX 自动生成的初始化文件
- 保留 AHT20 驱动
- 整理 ADC、UART、FreeRTOS 任务代码

## 4. 测试结果
- 编译结果：
- LED 测试：
- ADC 测试：
- AHT20 测试：
- FreeRTOS 测试：
- CAN 初始化结果：
- CAN 真实收发结果：

## 5. 遇到的问题
记录编译错误、接线问题和解决过程。

## 6. 下一步
Day 7：连接 STM32F407 与 STM32F103，完成 CAN 双节点通信。