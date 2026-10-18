#ifndef APP_MOTOR_CAN_H
#define APP_MOTOR_CAN_H

#include "main.h"
#include <stdint.h>

void App_MotorCan_OnRx(
    uint32_t std_id,
    const uint8_t *data,
    uint8_t len);

/* 读取当前目标速度 / 最后一次有效帧时间（供任务使用） */
int16_t App_MotorCan_GetTargetSpeed(void);
uint32_t App_MotorCan_GetLastRxTick(void);

#endif
