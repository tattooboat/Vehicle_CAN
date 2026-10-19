#ifndef __APP_SPEED_PID_H
#define __APP_SPEED_PID_H

#include <stdint.h>

typedef struct
{
    float kp;
    float ki;
    float kd;

    float integral;
    float last_error;

    float output_min;
    float output_max;
    float integral_limit;
} SpeedPID_t;

void App_SpeedPID_Init(SpeedPID_t *pid,
                       float kp,
                       float ki,
                       float kd);

void App_SpeedPID_Reset(SpeedPID_t *pid);

float App_SpeedPID_Update(SpeedPID_t *pid,
                          float target_rpm,
                          float actual_rpm,
                          float dt);

#endif
