#include "App_MotorCan.h"

extern CAN_HandleTypeDef hcan;

#define MOTOR_CAN_ID       0x301
#define MOTOR_CMD_SET      0x01
#define MOTOR_CMD_STOP     0x02
#define MOTOR_FRAME_MARK   0xA5

static uint8_t tx_seq = 0;

static HAL_StatusTypeDef MotorCan_SendFrame(
    uint8_t cmd, int16_t speed, uint8_t dlc)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint8_t data[8] = {0};
    uint32_t tx_mailbox;

    if (speed > 1000) speed = 1000;
    if (speed < -1000) speed = -1000;

    if (dlc > 8U) dlc = 8U;

    tx_header.StdId = MOTOR_CAN_ID;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = dlc;
    tx_header.TransmitGlobalTime = DISABLE;

    data[0] = MOTOR_FRAME_MARK;
    data[1] = cmd;
    data[2] = ((uint16_t)speed >> 8) & 0xFF;
    data[3] = (uint16_t)speed & 0xFF;
    data[4] = tx_seq++;

    HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(
        &hcan, &tx_header, data, &tx_mailbox);

    return ret;
}

HAL_StatusTypeDef App_MotorCan_SendSpeed(int16_t speed)
{
    return MotorCan_SendFrame(MOTOR_CMD_SET, speed, 8U);
}

HAL_StatusTypeDef App_MotorCan_SendStop(void)
{
    return MotorCan_SendFrame(MOTOR_CMD_STOP, 0, 8U);
}

/* 测试接口：可指定 DLC，用于发送帧长度错误(DLC!=8)的校验测试帧 */
HAL_StatusTypeDef App_MotorCan_SendTestFrame(
    uint8_t cmd, int16_t speed, uint8_t dlc)
{
    return MotorCan_SendFrame(cmd, speed, dlc);
}

/* 原始测试接口：直接指定 8 字节内容与 DLC，用于帧头/命令/越界等各类异常帧 */
HAL_StatusTypeDef App_MotorCan_SendRawFrame(
    const uint8_t *data, uint8_t dlc)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox;

    if (data == NULL)
        return HAL_ERROR;

    if (dlc > 8U) dlc = 8U;
    if (dlc == 0U) return HAL_ERROR;

    tx_header.StdId = MOTOR_CAN_ID;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = dlc;
    tx_header.TransmitGlobalTime = DISABLE;

    return HAL_CAN_AddTxMessage(
        &hcan, &tx_header, (uint8_t *)data, &tx_mailbox);
}
