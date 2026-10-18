/* --------------------------------------------------------------------------
 * Portions Copyright (c) 2022 STMicroelectronics. All rights reserved.
 * Portions Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Name:    freertos_os2.h
 * Purpose: Configuration checks and setup for FreeRTOS CMSIS-RTOS2 wrapper
 *---------------------------------------------------------------------------*/

#ifndef FREERTOS_OS2_H_
#define FREERTOS_OS2_H_

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"
#include "stm32f1xx.h"

#if (configSUPPORT_STATIC_ALLOCATION == 0) && (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error "Invalid FreeRTOS configuration: at least one of " \
       "configSUPPORT_STATIC_ALLOCATION or configSUPPORT_DYNAMIC_ALLOCATION " \
       "must be enabled."
#endif

#if (configUSE_COUNTING_SEMAPHORES == 0)
#error "Invalid FreeRTOS configuration: configUSE_COUNTING_SEMAPHORES must be 1."
#endif

#endif /* FREERTOS_OS2_H_ */