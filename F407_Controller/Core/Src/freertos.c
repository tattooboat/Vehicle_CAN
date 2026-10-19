/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os2.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "usart.h"
#include "App_data.h"
#include "adc.h"
#include "Int_aht20.h"
#include "tim.h"
#include "Int_motor.h"
#include "Int_Encoder.h"
#include "App_MotorCan.h"
#include <App_Fault.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMessageQueueId_t SensorDataQueueHandle;/* 传感器数据队列句柄 */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
// LED 任务
void LED_Task(void *argument);
// Debug 任务
void Debug_Task(void *argument);
// Sensor 任务
void Sensor_Task(void *argument);
// PWM 任务（已停用）
// void PWM_Test_Task(void *argument);
// CAN 电机控制任务
void App_MotorCan_ControlTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  const osMessageQueueAttr_t SensorDataQueue_attributes = {
    .name = "SensorDataQueue"
  };// 传感器数据队列属性

  SensorDataQueueHandle = osMessageQueueNew(
      8,
      sizeof(SensorData_t),
      &SensorDataQueue_attributes
  );// 创建传感器数据队列
  if (SensorDataQueueHandle == NULL)
  {
      /* 队列创建失败，可在此设置错误标志 */
  }

  const osThreadAttr_t sensorTask_attributes = {
      .name = "Sensor_Task",
      .stack_size = 768 * 4,
      .priority = (osPriority_t)osPriorityNormal
  };


  const osThreadAttr_t ledTask_attributes = {
    .name = "LED_Task",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow
  };

  const osThreadAttr_t debugTask_attributes = {
      .name = "Debug_Task",
      .stack_size = 768 * 4,
      .priority = (osPriority_t)osPriorityNormal
  };

  /* PWM_Test_Task 已停用，电机控制统一由 Motor_CAN_Control_Task 负责 */
  const osThreadAttr_t motorCanTask_attributes = {
        .name = "Motor_CAN",
        .stack_size = 1024 * 4,
        .priority = (osPriority_t)osPriorityHigh,
    };
  osThreadNew(App_MotorCan_ControlTask, NULL, &motorCanTask_attributes);
  osThreadNew(LED_Task, NULL, &ledTask_attributes);
  osThreadNew(Debug_Task, NULL, &debugTask_attributes);
  osThreadNew(Sensor_Task,NULL,&sensorTask_attributes);

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void LED_Task(void *argument)
{
  (void)argument;

  for (;;)
  {
      HAL_GPIO_TogglePin(LED_GPIO_Port,
                          LED_Pin);

      osDelay(500);
  }
}
void Debug_Task(void *argument)
{
    SensorData_t data;
    char msg[160];

    (void)argument;

    for (;;)
    {
        /*
         * 等待传感器数据。
         * osWaitForever 表示没有数据时阻塞等待，
         * 而不是反复空转占用 CPU。
         */
        if (osMessageQueueGet(
                SensorDataQueueHandle,
                &data,
                NULL,
                osWaitForever) == osOK)
        {
            int len;

            if (data.aht20_valid != 0U)//
            {
                len = snprintf(
                    msg,
                    sizeof(msg),
                    "ADC=%lu V=%.3f T=%.2fC H=%.2f%%RH\r\n",
                    (unsigned long)data.adc_raw,
                    (double)data.adc_voltage,
                    (double)data.temperature,
                    (double)data.humidity
                );
            }
            else
            {
                len = snprintf(
                    msg,
                    sizeof(msg),
                    "ADC=%lu V=%.3f AHT20=ERROR\r\n",
                    (unsigned long)data.adc_raw,
                    (double)data.adc_voltage
                );
            }

            if (len > 0 && len < (int)sizeof(msg))
            {
                HAL_UART_Transmit(
                    &huart1,
                    (uint8_t *)msg,
                    (uint16_t)len,
                    1000
                );
            }
        }
    }
}

static uint8_t ADC_ReadRaw(uint32_t *value)
{
    if (value == NULL)
    {
        return 1U;
    }

    /* 启动一次 ADC 转换 */
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return 2U;
    }

    /* 等待转换完成，最多等待 100 ms */
    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return 3U;
    }

    /* 读取 12 位 ADC 原始值 */
    *value = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    return 0U;
}

void Sensor_Task(void *argument)
{
    SensorData_t data;
    AHT20_Data_t aht;

    (void)argument;

    for (;;)
    {
        data.adc_raw = 0;
        data.adc_voltage = 0.0f;
        data.temperature = 0.0f;
        data.humidity = 0.0f;
        data.aht20_valid = 0U;

        /* 读取 PA0 对应的 ADC 通道 */
        if (ADC_ReadRaw(&data.adc_raw) == 0U)
        {
            data.adc_voltage =
                (float)data.adc_raw * 3.3f / 4095.0f;
        }

        /* 读取 AHT20 */
        if (AHT20_ReadData(&aht) == AHT20_OK)
        {
            data.temperature = aht.temperature;
            data.humidity = aht.humidity;
            data.aht20_valid = 1U;
        }

        /*
         * 这里使用 HAL_GetTick() 作为时间标记。
         * 后续如果增加多个时间源，需要统一时间基准。
         */
        data.timestamp_ms = HAL_GetTick();

        /*
         * 队列满时最多等待 10 ms。
         * 如果仍无法写入，就丢弃本次数据并继续下一轮。
         */
        (void)osMessageQueuePut(
            SensorDataQueueHandle,
            &data,
            0U,
            10U
        );

        osDelay(500);
    }
}

/* PID 参数定义 */
#define MOTOR_MAX_RPM       169.0f

/* 当前 Motor_CAN 使用本文件内联 PI；App_SpeedPID.c 尚未接入，阶段三统一。 */
/* 速度环增益与限幅 */
#define SPEED_PID_KP        10.0f
#define SPEED_PID_KI        1.0f

#define SPEED_PID_I_LIMIT   400.0f
#define SPEED_PID_PWM_MAX   1000.0f
#define SPEED_PID_DT        0.05f
/* 将 CAN 指令映射为 RPM，并由 PI 输出 PWM */
void App_MotorCan_ControlTask(void *argument)
{
    int16_t current_pwm = 0;
    int16_t can_speed = 0;

    uint32_t last_rx_tick;
    uint32_t print_cnt = 0;
    uint32_t last_loop_tick;

    float target_rpm = 0.0f;
    float previous_target_rpm = 0.0f;
    float actual_rpm = 0.0f;
    float error = 0.0f;
    float integral = 0.0f;
    float output = 0.0f;
    float dt = SPEED_PID_DT;

    char dbg_buf[128];

    (void)argument;

    Int_Motor_Init();
    Int_Encoder_Init();
    Int_Motor_Stop();

    last_loop_tick = HAL_GetTick();

    for (;;)
    {
        uint32_t now = HAL_GetTick();

        /* 计算本次控制周期 */
        uint32_t elapsed_ms = (uint32_t)(now - last_loop_tick);
        last_loop_tick = now;

        dt = (float)elapsed_ms / 1000.0f;

        /* 异常 dt 回退到固定周期 */
        if (dt < 0.01f || dt > 0.20f)
        {
            dt = SPEED_PID_DT;
        }

        last_rx_tick = App_MotorCan_GetLastRxTick();

        /* CAN 超时检查 */
        if (last_rx_tick == 0U ||
            (uint32_t)(now - last_rx_tick) > 500U)
        {
            can_speed = 0;
            target_rpm = 0.0f;

            App_Fault_Set(FAULT_CAN_TIMEOUT);
        }
        else
        {
            can_speed = App_MotorCan_GetTargetSpeed();

            App_Fault_Clear(FAULT_CAN_TIMEOUT);

            /* CAN 指令映射为目标 RPM */
            target_rpm =
                ((float)can_speed / 1000.0f) * MOTOR_MAX_RPM;
        }

        /* 读取编码器实际输出轴 RPM */
        actual_rpm = Int_Encoder_GetOutputRPM(dt);

        /* 目标大幅变化时重置积分，避免旧积分拖慢响应 */
        if (fabsf(target_rpm - previous_target_rpm) > 5.0f)
        {
            integral = 0.0f;
        }

        previous_target_rpm = target_rpm;

        /*
         * 目标为 0 时停止电机。
         * 同时清除积分和误差，避免下次启动带着旧积分。
         */
        if (target_rpm == 0.0f)
        {
            integral = 0.0f;
            error = 0.0f;
            output = 0.0f;
            current_pwm = 0;

            Int_Motor_Stop();
        }
        else
        {
            error = target_rpm - actual_rpm;

            /* PI 计算 */
            integral += error * dt;

            if (integral > SPEED_PID_I_LIMIT)
            {
                integral = SPEED_PID_I_LIMIT;
            }
            else if (integral < -SPEED_PID_I_LIMIT)
            {
                integral = -SPEED_PID_I_LIMIT;
            }

            output = SPEED_PID_KP * error
                   + SPEED_PID_KI * integral;

            /* PWM 限幅 */
            if (output > SPEED_PID_PWM_MAX)
            {
                output = SPEED_PID_PWM_MAX;
            }
            else if (output < -SPEED_PID_PWM_MAX)
            {
                output = -SPEED_PID_PWM_MAX;
            }

            current_pwm = (int16_t)output;

            Int_Motor_SetSpeed(current_pwm);
        }

        /* 每 10 次循环打印一次，约 500 ms */
        if ((print_cnt++ % 10U) == 0U)
        {
            int n = snprintf(
                dbg_buf,
                sizeof(dbg_buf),
                "TgtRPM=%.1f ActRPM=%.1f "
                "Err=%.1f I=%.1f PWM=%d\r\n",
                (double)target_rpm,
                (double)actual_rpm,
                (double)error,
                (double)integral,
                (int)current_pwm
            );

            if (n > 0 && n < (int)sizeof(dbg_buf))
            {
                HAL_UART_Transmit(
                    &huart1,
                    (uint8_t *)dbg_buf,
                    (uint16_t)n,
                    50
                );
            }
        }

        osDelay(50);
    }
}
/* USER CODE END Application */
