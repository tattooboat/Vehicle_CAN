#include "App_MotorControl.h"
#include "Int_motor.h"

void App_MotorControl_Init(void)
{
    Int_Motor_Stop();
}

void App_MotorControl_SetSpeed(int16_t speed)
{
    if (speed > 1000)
    {
        speed = 1000;
    }
    else if (speed < -1000)
    {
        speed = -1000;
    }

    Int_Motor_SetSpeed(speed);
}

void App_MotorControl_Stop(void)
{
    Int_Motor_Stop();
}
