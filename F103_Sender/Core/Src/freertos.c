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
#include "App_MotorCan.h"

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

/* Definitions for CAN_Motor_TestTask */
osThreadId_t CAN_Motor_TestTaskHandle;
const osThreadAttr_t CAN_Motor_TestTask_attributes = {
  .name = "CAN_Motor_TestTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

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

void CAN_Motor_Test_Task(void *argument);

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
  /* creation of CAN_Motor_TestTask */
  CAN_Motor_TestTaskHandle = osThreadNew(CAN_Motor_Test_Task, NULL, &CAN_Motor_TestTask_attributes);
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
/**
  * @brief  Function implementing the CAN motor test thread.
  * @param  argument: Not used
  * @retval None
  */
void CAN_Motor_Test_Task(void *argument)
{
  /* 测试序列：低速/中速/较高速/停止/反转，每档保持 5 秒后循环
   * 顺序   CAN速度命令   目标转速
   * 1     +50         +8.45 RPM
   * 2     +100        +16.9 RPM
   * 3     +200        +33.8 RPM
   * 4     0 (STOP)    0 RPM
   * 5     -100        -16.9 RPM
   */
  // const int16_t cmd_seq[] = {50, 100, 200, 0, -100};
  // const uint32_t hold_ms = 5000;      /* 每档保持时长 */
  // const uint32_t period_ms = 50;      /* 周期心跳间隔 */
  // const uint32_t send_count = hold_ms / period_ms;

  // for (;;)
  // {
  //   for (int i = 0; i < (int)(sizeof(cmd_seq) / sizeof(cmd_seq[0])); i++)
  //   {
  //     for (uint32_t j = 0; j < send_count; j++)
  //     {
  //       if (cmd_seq[i] == 0)
  //         App_MotorCan_SendStop();
  //       else
  //         App_MotorCan_SendSpeed(cmd_seq[i]);
  //       osDelay(period_ms);
  //     }
  //   }
  // }

  /* 循环测试序列：
   * 1) +100 稳定运行，记录实际转速
   * 2) +200 稳定后记录 ActRPM / Err / I / PWM
   * 3) 切回 +100，观察能否重新稳定在 16.9 RPM
   * 4) 发送 STOP，确认电机停止
   */
  const int16_t cmd_seq[] = {100, 200, 100, 0};
  const uint32_t hold_ms = 40000;      /* 每档保持时长 */
  const uint32_t period_ms = 50;      /* 周期心跳间隔 */
  const uint32_t send_count = hold_ms / period_ms;

  for (;;)
  {
    for (uint32_t i = 0; i < sizeof(cmd_seq) / sizeof(cmd_seq[0]); i++)
    {
      uint32_t j;
      for (j = 0; j < send_count; j++)
      {
        if (cmd_seq[i] == 0)
          App_MotorCan_SendStop();
        else
          App_MotorCan_SendSpeed(cmd_seq[i]);
        osDelay(period_ms);
      }
    }
  }
}

/* USER CODE END Application */
