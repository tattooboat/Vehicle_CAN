#include "App_can_protocol.h"
#include <string.h>

CAN_ProtocolMonitor_t g_can_protocol_monitor = {0};

/* 按小端格式解析有符号 16 位整数 */
static int16_t CAN_Protocol_ReadInt16LE(const uint8_t *data)
{
    uint16_t value;

    value = (uint16_t)data[0]
          | ((uint16_t)data[1] << 8);

    return (int16_t)value;
}

/* 组装控制命令：使能 + 目标值 + 序号 */
uint8_t CAN_Protocol_PackControl(uint8_t enable,
                                 int16_t target,
                                 uint8_t sequence,
                                 uint8_t data[8])
{
    uint16_t raw_target;

    if (data == NULL)
    {
        return 0U;
    }

    if ((enable != CAN_CMD_DISABLE) &&
        (enable != CAN_CMD_ENABLE))
    {
        return 0U;
    }

    raw_target = (uint16_t)target;

    data[0] = enable;
    data[1] = (uint8_t)(raw_target & 0xFFU);
    data[2] = (uint8_t)((raw_target >> 8) & 0xFFU);
    data[3] = sequence;

    return CAN_PROTOCOL_DLC_CONTROL;
}

void CAN_Protocol_OnFrame(uint32_t id,
                          uint8_t is_extended,
                          uint8_t is_remote,
                          uint8_t dlc,
                          const uint8_t data[8])
{
    uint8_t i;

    /* 当前应用协议只接受标准数据帧 */
    if ((is_extended != 0U) ||
        (is_remote != 0U) ||
        (dlc > 8U) ||
        (data == NULL))
    {
        g_can_protocol_monitor.invalid_count++;
        return;
    }

    g_can_protocol_monitor.rx_count++;
    g_can_protocol_monitor.last_id = id;
    g_can_protocol_monitor.last_dlc = dlc;

    for (i = 0U; i < 8U; i++)
    {
        g_can_protocol_monitor.last_data[i] =
            (i < dlc) ? data[i] : 0U;
    }

    switch (id)
    {
        case CAN_ID_HEARTBEAT:
            if (dlc != CAN_PROTOCOL_DLC_HEARTBEAT)
            {
                g_can_protocol_monitor.invalid_count++;
                return;
            }
            break;

        case CAN_ID_SENSOR:
            if (dlc != CAN_PROTOCOL_DLC_SENSOR)
            {
                g_can_protocol_monitor.invalid_count++;
                return;
            }
            break;

        case CAN_ID_CONTROL:
            if (dlc != CAN_PROTOCOL_DLC_CONTROL)
            {
                g_can_protocol_monitor.invalid_count++;
                return;
            }

            if ((data[0] != CAN_CMD_DISABLE) &&
                (data[0] != CAN_CMD_ENABLE))
            {
                g_can_protocol_monitor.invalid_count++;
                return;
            }

            g_can_protocol_monitor.control_enable = data[0];
            g_can_protocol_monitor.control_target =
                CAN_Protocol_ReadInt16LE(&data[1]);
            g_can_protocol_monitor.control_sequence = data[3];
            g_can_protocol_monitor.control_count++;
            break;

        case CAN_ID_STATUS:
            if (dlc != CAN_PROTOCOL_DLC_STATUS)
            {
                g_can_protocol_monitor.invalid_count++;
                return;
            }
            break;

        default:
            g_can_protocol_monitor.unknown_count++;
            break;
    }
}

