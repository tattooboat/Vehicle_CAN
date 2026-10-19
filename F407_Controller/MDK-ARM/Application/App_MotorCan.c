#include "App_MotorCan.h"
#include "App_Fault.h"

#define MOTOR_CAN_ID       0x301
#define MOTOR_CMD_SET      0x01
#define MOTOR_CMD_STOP     0x02
#define MOTOR_FRAME_MARK   0xA5

static volatile int16_t target_speed = 0;
static volatile uint32_t last_rx_tick = 0;

// 处理 CAN 接收的电机控制帧
void App_MotorCan_OnRx(
    uint32_t std_id,
    const uint8_t *data,
    uint8_t len)
{
    int16_t speed;

    /* 只处理电机控制帧 */
    if (std_id != MOTOR_CAN_ID)
    {
        return;
    }

    /* 检查数据指针和帧长 */
    if (data == NULL || len != 8)
    {
        g_can_invalid_count++;
        App_Fault_Set(FAULT_CAN_BAD_FRAME);
        return;
    }

    /* 检查帧标记 */
    if (data[0] != MOTOR_FRAME_MARK)
    {
        g_can_invalid_count++;
        App_Fault_Set(FAULT_CAN_BAD_FRAME);
        return;
    }

    /* 检查命令类型 */
    if (data[1] != MOTOR_CMD_SET &&
        data[1] != MOTOR_CMD_STOP)
    {
        g_can_invalid_count++;
        App_Fault_Set(FAULT_CAN_BAD_COMMAND);
        return;
    }

    /* 停止命令 */
    if (data[1] == MOTOR_CMD_STOP)
    {
        target_speed = 0;
        last_rx_tick = HAL_GetTick();

        App_Fault_Clear(FAULT_CAN_TIMEOUT);
        App_Fault_Clear(FAULT_CAN_BAD_FRAME);
        App_Fault_Clear(FAULT_CAN_BAD_COMMAND);
        App_Fault_Clear(FAULT_CAN_BAD_SPEED);

        return;
    }

    /* 高字节在前，低字节在后 */
    speed = (int16_t)(
        ((uint16_t)data[2] << 8) | data[3]);

    /* 检查控制值范围 */
    if (speed > 1000 || speed < -1000)
    {
        g_can_invalid_count++;
        App_Fault_Set(FAULT_CAN_BAD_SPEED);
        return;
    }

    /* 更新有效的电机控制目标 */
    target_speed = speed;
    last_rx_tick = HAL_GetTick();

    /* 有效命令到达，清除已恢复的故障状态 */
    App_Fault_Clear(FAULT_CAN_TIMEOUT);
    App_Fault_Clear(FAULT_CAN_BAD_FRAME);
    App_Fault_Clear(FAULT_CAN_BAD_COMMAND);
    App_Fault_Clear(FAULT_CAN_BAD_SPEED);
}

/* 读取当前目标速度 */
int16_t App_MotorCan_GetTargetSpeed(void)
{
    return target_speed;
}

/* 读取最后一次有效帧时间 */
uint32_t App_MotorCan_GetLastRxTick(void)
{
    return last_rx_tick;
}
