# Day 13：CAN 故障检测与安全控制

## 一、实验目标

在 STM32F407（主控/电机控制）+ STM32F103（发送端）已有的 CAN 双节点通信基础上，为 F407 的电机控制帧解析增加**异常数据校验与故障标志上报**，并设计非法 CAN 帧测试矩阵，验证各类非法数据能否被正确识别并触发对应故障标志，保证电机控制的安全性（坏帧不停机、不误控）。

## 二、协议与故障标志定义

### 1. 电机控制帧格式（ID = 0x301，标准数据帧，DLC = 8）

| 字节 | 含义 | 取值 |
|---|---|---|
| Data[0] | 帧标记 | `0xA5`（`MOTOR_FRAME_MARK`） |
| Data[1] | 命令 | `0x01` 设置（`MOTOR_CMD_SET`）/ `0x02` 停止（`MOTOR_CMD_STOP`） |
| Data[2] | 速度高字节 | 有符号 16 位，高字节在前 |
| Data[3] | 速度低字节 | 同上 |
| Data[4..7] | 保留 | 任意 |

> 速度读取：`speed = (data[2] << 8) | data[3]`（大端），有效范围 `-1000 ~ 1000`。

### 2. 故障标志位（`App_Fault.h`）

```c
#define FAULT_NONE            0x00000000U
#define FAULT_CAN_TIMEOUT     0x00000001U  // CAN 通信超时
#define FAULT_CAN_BAD_FRAME   0x00000002U  // 帧长度 / 帧头错误
#define FAULT_CAN_BAD_COMMAND 0x00000004U  // 命令字段非法
#define FAULT_CAN_BAD_SPEED   0x00000008U  // 速度越界
```

- `g_fault_flags` 为 32 位按位叠加（`|=` / `&=`），**可同时存在多个故障**。
- 相关计数器：`g_can_rx_count`（接收帧数）、`g_can_invalid_count`（非法帧数）。

## 三、核心代码逻辑（F407 侧）

### 1. 接收链路

`HAL_CAN_RxFifo0MsgPendingCallback`（`Int_can.c`）：当帧 ID 为 `0x301` 且为标准数据帧时，将 `DLC` 与 `rx_data` 原样传给 `App_MotorCan_OnRx`，**无论 DLC 是否等于 8 都会进入解析**，从而保证长度校验能被真实触发。

### 2. 帧校验顺序（`App_MotorCan.c`）

按以下顺序逐级校验，任一失败立即置位对应故障并 `return`（不再更新速度和超时时间）：

1. **帧长度**：`data == NULL || len != 8` → `FAULT_CAN_BAD_FRAME`（0x02）
2. **帧头**：`data[0] != 0xA5` → `FAULT_CAN_BAD_FRAME`（0x02）
3. **命令**：`data[1]` 既不是 `0x01` 也不是 `0x02` → `FAULT_CAN_BAD_COMMAND`（0x04）
4. **速度范围**：`speed > 1000 || speed < -1000` → `FAULT_CAN_BAD_SPEED`（0x08）

> 注意：以上校验通过判断后都是直接 `return`，**不会更新 `last_rx_tick`**，因此这些非法帧不会被当作"有效控制帧"，也不会清除任何故障。

### 3. 安全控制（`freertos.c` 电机控制任务）

- 控制任务每 50ms 轮询一次目标速度，若 `500ms` 内没有收到有效控制帧，目标速度归 0，并置 `FAULT_CAN_TIMEOUT`（0x01）。
- 收到有效控制帧后清除 `FAULT_CAN_TIMEOUT`。
- 非法帧触发故障后**不会改变电机目标速度**，电机保持之前安全状态，避免误转。

### 4. 有效帧对故障的清除

成功解析到有效控制帧（含停止命令）时，会同时清除 `FAULT_CAN_TIMEOUT`、`FAULT_CAN_BAD_FRAME`、`FAULT_CAN_BAD_COMMAND`、`FAULT_CAN_BAD_SPEED`。

## 四、非法帧测试矩阵

测试时保持 F407 正常运行，先让电机停止（F103 发停止命令），再通过 F103 构造并发送非法帧。

| 测试项 | 修改内容 | 预期故障标志 | 实测 |
|---|---|---|---|
| 帧长度错误 | 固定 ID=0x301，令 **DLC ≠ 8** | `FAULT_CAN_BAD_FRAME` = 0x02 | |
| 帧头错误 | DLC=8，令 **Data[0] ≠ 0xA5** | `FAULT_CAN_BAD_FRAME` = 0x02 | |
| 命令错误 | DLC=8、Data[0]=0xA5，令 **Data[1] ∈ {3, ...}**（非 0x01/0x02） | `FAULT_CAN_BAD_COMMAND` = 0x04 | |
| 速度越界 | DLC=8、Data[0]=0xA5、Data[1]=0x01，令速度超出 **-1000 ~ 1000** | `FAULT_CAN_BAD_SPEED` = 0x08 | |

### 关于读数"一闪闪"的说明（重要）

故障标志按位叠加可同时存在多个，测非法帧时若**测试期间中断了有效控制帧**，500ms 后控制任务会同时置上 `FAULT_CAN_TIMEOUT`（0x01），因此看到的可能不是单一 0x02：

| 预期（仅坏帧） | 实际叠加（含超时） |
|---|---|
| 0x02（BAD_FRAME） | 0x02 ↔ 0x03（BAD_FRAME\|TIMEOUT） |
| 0x04（BAD_COMMAND） | 0x04 ↔ 0x05（BAD_COMMAND\|TIMEOUT） |
| 0x08（BAD_SPEED） | 0x08 ↔ 0x09（BAD_SPEED\|TIMEOUT） |

- **只要观测值中包含对应位（0x02/0x04/0x08）即为通过**；额外的 0x01 是超时故障的正常叠加，不是校验 bug。
- 若想让 g_fault_flags 稳定停留在单个故障值，可在发送非法帧时**周期性穿插发送合法停止帧**（ID=0x301、DLC=8、Data={0xA5,0x02,...}，如每 300ms 一次），合法帧会清除超时，使数值稳定在 0x02/0x04/0x08。

## 五、测试结果

- 帧长度错误（DLC≠8）：□ 通过　□ 未通过，实测故障标志 =
- 帧头错误（Data[0]≠0xA5）：□ 通过　□ 未通过，实测故障标志 =
- 命令错误（Data[1]非法）：□ 通过　□ 未通过，实测故障标志 =
- 速度越界（超 ±1000）：□ 通过　□ 未通过，实测故障标志 =

## 六、遇到的问题

### 1. 观测值不是预期单一值、而是"闪"多个值

- **现象**：帧长度错误测试时，g_fault_flags 在 0x02 与 0x03 之间跳动。
- **原因**：故障按位叠加，测试期间无有效帧导致 500ms 超时置位 `TIMEOUT`(0x01)，与 `BAD_FRAME`(0x02) 叠加成 0x03。
- **结论**：帧长度检测（0x02）本身正确触发；0x01 为超时故障叠加，属正常逻辑。
- **解决/规避**：发送非法帧时穿插合法停止帧清除超时，即可稳定观察单一故障位。

## 七、下一步

- 将 `g_fault_flags` 增加串口/LED 等可视化上报，便于测试时实时观察而无需调试器观看。
- 结合 F103 端实现自动化非法帧注入脚本，覆盖边界值（DLC=0/9、Data[0] 边界、速度 +1000/-1000/±1001）做完整边界回归。
- 加入故障后安全停机逻辑（如故障持续置位时强制停止电机并声光报警）。

## 八、参考资料

- 相关文件：
  - `Application/App_MotorCan.c`、`App_MotorCan.h`
  - `Application/App_Fault.c`、`App_Fault.h`
  - `Interface/Int_can.c`
  - `Core/Src/freertos.c`