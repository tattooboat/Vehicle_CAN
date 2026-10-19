#include "Int_Encoder.h"
#include "tim.h"

/* 上一次读取时的计数值 */
static uint16_t encoder_last_count = 0;

#define ENCODER_PULSE_PER_MOTOR_REV  11.0f // 编码器每转的脉冲数
#define ENCODER_DECODE_MULTIPLIER   4.0f // 编码器解码倍数
#define MOTOR_GEAR_RATIO             35.5f // 电机齿轮比
/**
 * @brief 初始化编码器
 * @note TIM8 必须已经由 MX_TIM8_Init() 初始化
 */
void Int_Encoder_Init(void)
{
    __HAL_TIM_SET_COUNTER(&htim8, 0);
    encoder_last_count = 0;

    HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
}

/**
 * @brief 读取两次调用之间的计数变化
 * @return 正数、负数或 0
 */
int16_t Int_Encoder_GetDelta(void)
{
    uint16_t now = (uint16_t)__HAL_TIM_GET_COUNTER(&htim8);

    int32_t delta = (int32_t)now
                  - (int32_t)encoder_last_count;

    /* 处理 16 位计数器回绕 */
    if (delta > 32767)
    {
        delta -= 65536;
    }
    else if (delta < -32768)
    {
        delta += 65536;
    }

    encoder_last_count = now;

    return (int16_t)delta;
}

/**
 * @brief 获取当前计数器值
 */
uint16_t Int_Encoder_GetCount(void)
{
    return (uint16_t)__HAL_TIM_GET_COUNTER(&htim8);
}

float Int_Encoder_GetOutputRPM(float sample_period_s)
{
    int16_t delta;

    const float counts_per_motor_rev =
        ENCODER_PULSE_PER_MOTOR_REV * ENCODER_DECODE_MULTIPLIER;

    if (sample_period_s <= 0.0f)
    {
        return 0.0f;
    }

    delta = Int_Encoder_GetDelta();

    return -(float)delta * 60.0f
           / (counts_per_motor_rev
              * sample_period_s
              * MOTOR_GEAR_RATIO);
}
