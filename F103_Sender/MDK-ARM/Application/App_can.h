#ifndef __APP_CAN_H
#define __APP_CAN_H

#include "main.h"

/* CAN 应用层帧 ID */
#define APP_CAN_ID_CMD       0x123U
#define APP_CAN_ID_REPLY     0x124U

/* 命令字 */
#define APP_CAN_CMD_PING     0x01U
#define APP_CAN_CMD_SET      0x02U
#define APP_CAN_CMD_STOP     0x03U

/* 应答命令字 */
#define APP_CAN_REPLY_ACK    0x81U
#define APP_CAN_REPLY_NACK   0x82U

/* 应答状态 */
#define APP_CAN_STATUS_OK       0x00U
#define APP_CAN_STATUS_BAD_CMD  0x01U
#define APP_CAN_STATUS_BAD_DATA 0x02U

#define APP_CAN_FRAME_LEN    8U

typedef struct
{
    uint8_t cmd;
    uint8_t seq;
    uint8_t target;
    uint16_t value;
    uint8_t status;
    uint8_t reserved;
    uint8_t checksum;
} APP_CAN_Message_t;

void APP_CAN_Init(void);
void APP_CAN_Process(void);

uint8_t APP_CAN_Checksum(const uint8_t *data);
uint8_t APP_CAN_Encode(
    const APP_CAN_Message_t *msg,
    uint8_t data[APP_CAN_FRAME_LEN]
);

uint8_t APP_CAN_Decode(
    const uint8_t data[APP_CAN_FRAME_LEN],
    APP_CAN_Message_t *msg
);


#endif//__APP_CAN_H
