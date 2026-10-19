#include "App_SpeedPID.h"

static float clampf(float value, float min_value, float max_value)
{
    if (value > max_value) return max_value;
    if (value < min_value) return min_value;
    return value;
}

void App_SpeedPID_Init(SpeedPID_t *pid,
                       float kp,
                       float ki,
                       float kd)
{
    if (pid == 0) return;

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->integral = 0.0f;
    pid->last_error = 0.0f;

    pid->output_min = -1000.0f;
    pid->output_max = 1000.0f;
    pid->integral_limit = 500.0f;
}

void App_SpeedPID_Reset(SpeedPID_t *pid)
{
    if (pid == 0) return;

    pid->integral = 0.0f;
    pid->last_error = 0.0f;
}

float App_SpeedPID_Update(SpeedPID_t *pid,
                          float target_rpm,
                          float actual_rpm,
                          float dt)
{
    float error;
    float derivative;
    float output;

    if ((pid == 0) || (dt <= 0.0f))
    {
        return 0.0f;
    }

    error = target_rpm - actual_rpm;

    pid->integral += error * dt;
    pid->integral = clampf(pid->integral,
                           -pid->integral_limit,
                           pid->integral_limit);

    derivative = (error - pid->last_error) / dt;

    output = pid->kp * error
           + pid->ki * pid->integral
           + pid->kd * derivative;

    pid->last_error = error;

    output = clampf(output, pid->output_min, pid->output_max);

    return output;
}
