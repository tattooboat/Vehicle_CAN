#include "Int_motor.h"
#include "tim.h"
#include "gpio.h"

#define MOTOR_IN1_GPIO_Port  GPIOB
#define MOTOR_IN1_Pin        GPIO_PIN_12

#define MOTOR_IN2_GPIO_Port  GPIOB
#define MOTOR_IN2_Pin        GPIO_PIN_13

#define MOTOR_STBY_GPIO_Port GPIOB
#define MOTOR_STBY_Pin       GPIO_PIN_14

#define MOTOR_PWM_TIMER      htim3
#define MOTOR_PWM_CHANNEL    TIM_CHANNEL_3

#define MOTOR_PWM_MAX        1000U

HAL_StatusTypeDef Int_Motor_Init(void)
{
    /* 先无条件使能驱动器，避免 PWM 启动失败导致 STBY 停留在低电平 */
    HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port,
                      MOTOR_STBY_Pin,
                      GPIO_PIN_SET);

    HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port,
                      MOTOR_IN1_Pin,
                      GPIO_PIN_RESET);

    HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port,
                      MOTOR_IN2_Pin,
                      GPIO_PIN_RESET);

    if (HAL_TIM_PWM_Start(&MOTOR_PWM_TIMER,
                          MOTOR_PWM_CHANNEL) != HAL_OK)
    {
        return HAL_ERROR;
    }

    __HAL_TIM_SET_COMPARE(&MOTOR_PWM_TIMER,
                          MOTOR_PWM_CHANNEL,
                          0U);

    return HAL_OK;
}

void Int_Motor_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&MOTOR_PWM_TIMER,
                          MOTOR_PWM_CHANNEL,
                          0U);

    HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port,
                      MOTOR_IN1_Pin,
                      GPIO_PIN_RESET);

    HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port,
                      MOTOR_IN2_Pin,
                      GPIO_PIN_RESET);
}

void Int_Motor_SetSpeed(int16_t speed)
{
    uint32_t duty;

    if (speed > 1000)
    {
        speed = 1000;
    }
    else if (speed < -1000)
    {
        speed = -1000;
    }

    if (speed == 0)
    {
        Int_Motor_Stop();
        return;
    }

    if (speed > 0)
    {
        HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port,
                          MOTOR_IN1_Pin,
                          GPIO_PIN_SET);

        HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port,
                          MOTOR_IN2_Pin,
                          GPIO_PIN_RESET);

        duty = (uint32_t)speed;
    }
    else
    {
        HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port,
                          MOTOR_IN1_Pin,
                          GPIO_PIN_RESET);

        HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port,
                          MOTOR_IN2_Pin,
                          GPIO_PIN_SET);

        duty = (uint32_t)(-speed);
    }

    __HAL_TIM_SET_COMPARE(&MOTOR_PWM_TIMER,
                          MOTOR_PWM_CHANNEL,
                          duty);
}

