#ifndef _INT_CAN_H
#define _INT_CAN_H

#include "main.h"
#include <stdint.h>
#include "App_can_protocol.h"

typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint8_t is_extended;
    uint8_t is_remote;
} INT_CAN_Frame_t;

HAL_StatusTypeDef INT_CAN_Start(void);

HAL_StatusTypeDef INT_CAN_SendStd(
    uint16_t std_id,
    const uint8_t *data,
    uint8_t length,
    uint32_t timeout_ms
);

HAL_StatusTypeDef INT_CAN_Receive(
    INT_CAN_Frame_t *frame
);

#endif
