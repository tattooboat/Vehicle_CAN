# Day 13：CAN 故障检测与安全控制

## 一、实验目标

在 STM32F103（发送端）+ STM32F407（主控/电机控制）的 CAN 双节点链路中，验证 F407 侧为电机控制帧解析增加的**异常数据校验与故障标志上报**。本 Day 聚焦 F103 发送端如何构造非法帧注入，配套 F407 的帧校验逻辑，逐项确认非法数据能被正确识别并触发对应故障标志，保证电机控制安全（坏帧不误控、不误转）。

## 二、双端角色划分

| 节点 | 角色 | 职责 |
|---|---|---|
| STM32F103（本工程） | 发送端/测试端 | 通过 CAN 定期发送 `ID=0x301` 电机控制帧；测试时按需构造 DLC/帧头/命令/速度等非法帧用于故障注入 |
| STM32F407（对端工程） | 主控/电机控制 | 接收 `0x301` 帧，逐级校验并置位故障标志，驱动 TB6612FNG 控制直流电机 |

## 三、电机控制帧格式（ID = 0x301，标准数据帧，DLC = 8）

| 字节 | 含义 | 取值 |
|---|---|---|
| Data[0] | 帧标记 | `0xA5`（`MOTOR_FRAME_MARK`） |
| Data[1] | 命令 | `0x01` 设置（`MOTOR_CMD_SET`）/ `0x02` 停止（`MOTOR_CMD_STOP`） |
| Data[2] | 速度高字节 | 有符号 16 位，高字节在前 |
| Data[3] | 速度低字节 | 同上 |
| Data[4..7] | 保留 | 任意 |

> 速度读取（F407 侧）：`speed = (data[2] << 8) | data[3]`（大端），有效范围 `-1000 ~ 1000`。

## 四、F407 故障标志位（App_Fault.h）

```c
#define FAULT_NONE            0x00000000U
#define FAULT_CAN_TIMEOUT     0x00000001U  // CAN 通信超时
#define FAULT_CAN_BAD_FRAME   0x00000002U  // 帧长度 / 帧头错误
#define FAULT_CAN_BAD_COMMAND 0x00000004U  // 命令字段非法
#define FAULT_CAN_BAD_SPEED   0x00000008U  // 速度越界
```

- `g_fault_flags` 为 32 位按位叠加（`|=` / `&=`），**可同时存在多个故障**。

## 五、F407 帧校验顺序（App_MotorCan.c）

按顺序逐级校验，任一失败立即置位对应故障并直接 `return`（不更新目标速度、不更新超时时间，非法帧不被当作有效控制帧）：

1. **帧长度**：`data == NULL || len != 8` → `FAULT_CAN_BAD_FRAME`（0x02）
2. **帧头**：`data[0] != 0xA5` → `FAULT_CAN_BAD_FRAME`（0x02）
3. **命令**：`data[1]` 非 `0x01` 且非 `0x02` → `FAULT_CAN_BAD_COMMAND`（0x04）
4. **速度范围**：`speed > 1000 || speed < -1000` → `FAULT_CAN_BAD_SPEED`（0x08）

收到有效控制帧会清除上述全部 CAN 故障；500ms 内无有效帧则置 `FAULT_CAN_TIMEOUT`（0x01）。

## 六、本工程（F103 发送端）测试实现

### 1. 发送接口（App_MotorCan.c / App_MotorCan.h）

| 接口 | 说明 |
|---|---|
| `App_MotorCan_SendSpeed(speed)` | 发送合法 SET 帧（DLC=8，Data[0]=0xA5，Data[1]=0x01） |
| `App_MotorCan_SendStop()` | 发送合法 STOP 帧（DLC=8，Data[0]=0xA5，Data[1]=0x02） |
| `App_MotorCan_SendTestFrame(cmd, speed, dlc)` | 测试用：可控 DLC，构造帧长度错误的异常帧 |
| `App_MotorCan_SendRawFrame(data, dlc)` | 测试用：直接按字节指定帧内容与 DLC，构造帧头/命令/速度等各类非法帧 |

- 内部 `MotorCan_SendFrame()` 支持传入 DLC，`dlc > 8` 会被钳位到 8。
- 上述测试接口仅在测试阶段被 `CAN_Motor_Test_Task` 调用，正常工作时任务只使用 `App_MotorCan_SendSpeed`。

### 2. 测试任务（freertos.c 的 CAN_Motor_Test_Task）

正常状态仅周期发送合法帧（保持 300 转速）：

```c
for (;;)
{
    App_MotorCan_SendSpeed(300);   // 保持 300
    osDelay(50);
}
```

故障测试阶段，用 `App_MotorCan_SendRawFrame` 按测试项构造对应非法帧取代上面的合法帧。

## 七、非法帧测试矩阵

测试时保持 F407 正常运行；F103 端先发合法停止帧令电机停止，再构造并发送对应非法帧逐一验证：

| 测试项 | F103 发送帧（ID=0x301，双向大端） | 预期故障标志 |
|---|---|---|
| 帧长度错误 | DLC ≠ 8（如 DLC=7），其余字段合法 | `FAULT_CAN_BAD_FRAME` = 0x02 |
| 帧头错误 | DLC=8，Data[0] ≠ 0xA5，命令/速度合法 | `FAULT_CAN_BAD_FRAME` = 0x02 |
| 命令错误 | DLC=8，Data[0]=0xA5，Data[1]=0x03（非 0x01/0x02） | `FAULT_CAN_BAD_COMMAND` = 0x04 |
| 速度越界 | DLC=8，Data[0]=0xA5，Data[1]=0x01，速度超出 ±1000 | `FAULT_CAN_BAD_SPEED` = 0x08 |

F103 发送构造示例（帧头错误帧）：

```c
uint8_t bad_frame[8] = {
    0x00,       // Data[0] 错误：应为 0xA5
    0x01,       // Data[1] 命令 SET（合法）
    0x01, 0x2C, // Data[2..3] 速度 300（合法）
    0x00, 0x00, 0x00, 0x00
};
App_MotorCan_SendRawFrame(bad_frame, 8U);
```

> 设计原则：每项测试只改一个字段，其余字段全部合法，保证故障标志确实由目标校验项触发，不被前面校验挡住或误判。

### 关于读数"一闪闪"的说明（重要）

故障按位叠加可同时存在多个。若测试期间中断了合法控制帧，F407 在 500ms 后会自动叠加 `FAULT_CAN_TIMEOUT`（0x01）：

| 预期（仅坏帧） | 实际叠加（含超时） |
|---|---|
| 0x02（BAD_FRAME） | 0x02 ↔ 0x03（BAD_FRAME\|TIMEOUT） |
| 0x04（BAD_COMMAND） | 0x04 ↔ 0x05（BAD_COMMAND\|TIMEOUT） |
| 0x08（BAD_SPEED） | 0x08 ↔ 0x09（BAD_SPEED\|TIMEOUT） |

- 观测值只要包含对应位（0x02/0x04/0x08）即判定通过；多出的 0x01 是超时故障的正常叠加。
- 规避方法：F103 在发送非法帧期间**周期性穿插发送合法停止帧**（如每 300ms 一次），可用合法帧清除超时，使故障值稳定在单一故障位。

## 八、测试结论与结果记录

- 测试清单：帧长度错误（DLC≠8）、帧头错误（Data[0]≠0xA5）、命令错误（Data[1] 非法）、速度越界（超 ±1000）。
- Day 13 进度：已完成帧长度 / 帧头 / 命令三项故障注入测试，代码恢复为正常发送合法帧；速度越界项暂不测试。
- 结果记录：

| 测试项 | 预期 | 实测 | 通过 |
|---|---|---|---|
| 帧长度错误 | 0x02 | | □ |
| 帧头错误 | 0x02 | | □ |
| 命令错误 | 0x04 | | □ |
| 速度越界 | 0x08 | 未测 | — |

## 九、遇到的问题

### 1. 非法帧测试导致故障标志"闪"多个值

- 现象：单测帧长度错误时，F407 的 g_fault_flags 在 0x02 与 0x03 之间跳动。
- 原因：故障按位叠加，测试期间无有效控制帧，500ms 超时置位 `TIMEOUT`(0x01)，与 `BAD_FRAME`(0x02) 叠加成 0x03。
- 结论：帧长度检测（0x02）本身触发正确，0x01 为超时故障的叠加，属正常逻辑。
- 规避：发送非法帧时穿插合法停止帧清除超时，即可稳定观察单一故障位。

### 2. Keil 构建时 uv4.log 被占用的工程问题

- 现象：构建报"文件 uv4.log 正由另一进程使用"，构建任务退出码 1。
- 原因：残留的 `UV4.exe` 进程（及 `uv4.log.lock` 锁文件）一直占用构建日志。
- 处理：`taskkill /PID <pid> /F` 终止驻留的 `UV4.exe`，并删除 `.vscode\uv4.log`、`uv4.log.lock` 后再构建。

## 十、下一步

- 为 F407 的 `g_fault_flags` 增加串口 / LED 可视化上报，测试无需调试器即可观察。
- 在本工程实现自动化非法帧注入脚本，覆盖边界值（DLC=0/9、Data[0] 边界、速度 +1000/-1000/±1001）做完整回归。
- 加入故障后安全停机策略（故障持续置位时强制停转并告警）。

## 十一、参考资料

- 本工程（F103 发送端）：
  - `MDK-ARM/Application/App_MotorCan.c`、`App_MotorCan.h`
  - `Core/Src/freertos.c`（`CAN_Motor_Test_Task`）
- 对端（F407 主控）：
  - `Application/App_MotorCan.c`、`App_MotorCan.h`
  - `Application/App_Fault.c`、`App_Fault.h`
  - `Interface/Int_can.c`
  - `Core/Src/freertos.c`