#include "app_can.h"
#include <Int_can.h>

/* 计算前 7 字节的 8 位累加和 */
uint8_t APP_CAN_Checksum(const uint8_t *data)
{
    uint16_t sum = 0U;

    if (data == NULL)
    {
        return 0U;
    }

    for (uint8_t i = 0U; i < 7U; i++)
    {
        sum += data[i];
    }

    return (uint8_t)sum;
}

/* 将协议结构体编码为 8 字节 */
uint8_t APP_CAN_Encode(
    const APP_CAN_Message_t *msg,
    uint8_t data[APP_CAN_FRAME_LEN])
{
    if (msg == NULL || data == NULL)
    {
        return 0U;
    }

    data[0] = msg->cmd;
    data[1] = msg->seq;
    data[2] = msg->target;

    /* value 使用大端序 */
    data[3] = (uint8_t)(msg->value >> 8);
    data[4] = (uint8_t)(msg->value & 0xFFU);

    data[5] = msg->status;
    data[6] = msg->reserved;
    data[7] = APP_CAN_Checksum(data);

    return 1U;
}

/* 将 8 字节解析为协议结构体 */
uint8_t APP_CAN_Decode(
    const uint8_t data[APP_CAN_FRAME_LEN],
    APP_CAN_Message_t *msg)
{
    if (data == NULL || msg == NULL)
    {
        return 0U;
    }

    if (APP_CAN_Checksum(data) != data[7])
    {
        return 0U;
    }

    msg->cmd = data[0];
    msg->seq = data[1];
    msg->target = data[2];

    msg->value =
        ((uint16_t)data[3] << 8) |
        (uint16_t)data[4];

    msg->status = data[5];
    msg->reserved = data[6];
    msg->checksum = data[7];

    return 1U;
}
void APP_CAN_Init(void)
{
    /* 暂无额外初始化 */
}

void APP_CAN_Process(void)
{
    INT_CAN_Frame_t frame;
    APP_CAN_Message_t request;
    APP_CAN_Message_t reply;
    uint8_t reply_data[APP_CAN_FRAME_LEN];

    /* 没有新帧就直接返回 */
    if (INT_CAN_GetLastFrame(&frame) == 0U)
    {
        return;
    }

    /* 只处理 F407 发来的标准帧 */
    if (frame.id != APP_CAN_ID_CMD ||
        frame.dlc != APP_CAN_FRAME_LEN ||
        frame.is_extended != 0U ||
        frame.is_remote != 0U)
    {
        return;
    }

    /* 校验失败，丢弃错误数据 */
    if (APP_CAN_Decode(frame.data, &request) == 0U)
    {
        return;
    }

    /* 默认回复 NACK */
    reply.cmd = APP_CAN_REPLY_NACK;
    reply.seq = request.seq;
    reply.target = request.target;
    reply.value = request.value;
    reply.status = APP_CAN_STATUS_BAD_CMD;
    reply.reserved = 0U;
    reply.checksum = 0U;

    switch (request.cmd)
    {
        case APP_CAN_CMD_PING:
            reply.cmd = APP_CAN_REPLY_ACK;
            reply.status = APP_CAN_STATUS_OK;
            break;

        case APP_CAN_CMD_SET:
            /*
             * Day 8 暂时只验证参数通信。
             * 后续 Day 9~12 再把参数接入 PWM、电机控制。
             */
            reply.cmd = APP_CAN_REPLY_ACK;
            reply.status = APP_CAN_STATUS_OK;
            break;

        case APP_CAN_CMD_STOP:
            /*
             * 当前仅确认收到停止命令。
             * 真正的电机停止逻辑后续再接入。
             */
            reply.cmd = APP_CAN_REPLY_ACK;
            reply.status = APP_CAN_STATUS_OK;
            break;

        default:
            reply.cmd = APP_CAN_REPLY_NACK;
            reply.status = APP_CAN_STATUS_BAD_CMD;
            break;
    }

    if (APP_CAN_Encode(&reply, reply_data) == 0U)
    {
        return;
    }

    (void)INT_CAN_SendStd(
        APP_CAN_ID_REPLY,
        reply_data,
        APP_CAN_FRAME_LEN);
}
