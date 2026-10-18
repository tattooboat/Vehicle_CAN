#ifndef APP_MOTOR_CONTROL_H
#define APP_MOTOR_CONTROL_H

#include <stdint.h>
#include "Int_motor.h"

void App_MotorControl_Init(void);
void App_MotorControl_SetSpeed(int16_t speed);
void App_MotorControl_Stop(void);

#endif
