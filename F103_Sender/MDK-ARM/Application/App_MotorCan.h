#ifndef APP_MOTOR_CAN_H
#define APP_MOTOR_CAN_H

#include "main.h"
#include <stdint.h>

HAL_StatusTypeDef App_MotorCan_SendSpeed(int16_t speed);
HAL_StatusTypeDef App_MotorCan_SendStop(void);
HAL_StatusTypeDef App_MotorCan_SendTestFrame(
    uint8_t cmd, int16_t speed, uint8_t dlc);
HAL_StatusTypeDef App_MotorCan_SendRawFrame(
    const uint8_t *data, uint8_t dlc);

#endif
