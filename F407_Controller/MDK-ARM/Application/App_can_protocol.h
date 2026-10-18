#ifndef __APP_CAN_PROTOCOL_H__
#define __APP_CAN_PROTOCOL_H__
#include "main.h"
#include <stdint.h>

/* Mini VCU CAN 协议 ID */
#define CAN_ID_HEARTBEAT       0x100U//心跳包
#define CAN_ID_SENSOR          0x101U//传感器数据
#define CAN_ID_CONTROL         0x200U//控制命令
#define CAN_ID_STATUS          0x201U//状态报告

#define CAN_PROTOCOL_DLC_HEARTBEAT  2U
#define CAN_PROTOCOL_DLC_SENSOR     8U
#define CAN_PROTOCOL_DLC_CONTROL    4U
#define CAN_PROTOCOL_DLC_STATUS     4U

/* 控制命令 */
#define CAN_CMD_DISABLE       0x00U
#define CAN_CMD_ENABLE        0x01U

/* 协议解析结果 */
typedef enum
{
    CAN_PROTOCOL_UNKNOWN = 0,
    CAN_PROTOCOL_HEARTBEAT,
    CAN_PROTOCOL_SENSOR,
    CAN_PROTOCOL_CONTROL,
    CAN_PROTOCOL_STATUS,
    CAN_PROTOCOL_INVALID
} CAN_ProtocolType_t;

/* 解析后的控制命令 */
typedef struct
{
    uint8_t enable;
    int16_t target;
    uint8_t sequence;
} CAN_ControlData_t;

/* F407 当前接收到的协议数据 */
typedef struct
{
    volatile uint32_t rx_count;
    volatile uint32_t last_id;
    volatile uint8_t last_dlc;
    volatile uint8_t last_data[8];

    volatile uint32_t control_count;
    volatile uint8_t control_enable;
    volatile int16_t control_target;
    volatile uint8_t control_sequence;

    volatile uint32_t invalid_count;
    volatile uint32_t unknown_count;
} CAN_ProtocolMonitor_t;

extern CAN_ProtocolMonitor_t g_can_protocol_monitor;

/* 在接收回调中调用：解析一帧已读取的 CAN 报文 */
void CAN_Protocol_OnFrame(uint32_t id,
                          uint8_t is_extended,
                          uint8_t is_remote,
                          uint8_t dlc,
                          const uint8_t data[8]);

/* 打包控制命令，输出 4 字节数据 */
uint8_t CAN_Protocol_PackControl(uint8_t enable,
                                 int16_t target,
                                 uint8_t sequence,
                                 uint8_t data[8]);


#endif//__APP_CAN_PROTOCOL_H__
