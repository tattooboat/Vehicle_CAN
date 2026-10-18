#ifndef __INT_MOTOR_H
#define __INT_MOTOR_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef Int_Motor_Init(void);
void Int_Motor_SetSpeed(int16_t speed);
void Int_Motor_Stop(void);

#endif//__INT_MOTOR_H
