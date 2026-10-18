#ifndef __INT_CAN_H
#define __INT_CAN_H

#include "main.h"
#include <stdint.h>

typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint8_t is_extended;
    uint8_t is_remote;
} INT_CAN_Frame_t;

/* CAN 初始化与启动 */
HAL_StatusTypeDef INT_CAN_Start(void);

/* 发送标准数据帧 */
HAL_StatusTypeDef INT_CAN_SendStd(
    uint16_t std_id,
    const uint8_t *data,
    uint8_t length
);

/* 获取最近收到的一帧 */
uint8_t INT_CAN_GetLastFrame(INT_CAN_Frame_t *frame);

/* 接收统计变量 */
extern volatile uint32_t int_can_rx_count;
extern volatile uint32_t int_can_last_id;
extern volatile uint8_t int_can_last_dlc;
extern volatile uint8_t int_can_last_data[8];

#endif
