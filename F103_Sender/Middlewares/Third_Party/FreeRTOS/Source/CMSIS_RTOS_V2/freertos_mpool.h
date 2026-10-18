/* --------------------------------------------------------------------------
 * Portions Copyright (c) 2022 STMicroelectronics. All rights reserved.
 * Portions Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Name:    freertos_mpool.h
 * Purpose: Memory pool definitions for FreeRTOS CMSIS-RTOS2 wrapper
 *---------------------------------------------------------------------------*/

#ifndef FREERTOS_MPOOL_H_
#define FREERTOS_MPOOL_H_

#include "FreeRTOS.h"
#include "semphr.h"

/* Memory pool status flags */
#define MPOOL_STATUS              0x554D0000U  /* 'UM' marker */
#define MPOOL_STATUS_CB_SEM       0x00000001U  /* control block semaphore on heap */
#define MPOOL_STATUS_MEM_ARR      0x00000002U  /* memory array on heap */

/* Memory array size required for block_count blocks of block_size each */
#define MEMPOOL_ARR_SIZE(count, size)  ((size_t)(count) * ((size_t)(size) + sizeof(void *)))

/* Memory pool block header */
typedef struct mem_pool_block {
  struct mem_pool_block *next;              /* pointer to next free block */
} MemPoolBlock_t;

/* Memory pool control block */
typedef struct mem_pool {
  SemaphoreHandle_t  sem;                   /* block counting semaphore handle */
  StaticSemaphore_t  mem_sem;               /* static semaphore storage */
  uint8_t           *mem_arr;               /* memory array */
  MemPoolBlock_t    *head;                  /* free block list head */
  size_t             mem_sz;                /* memory array size */
  const char        *name;                  /* pool name */
  uint32_t           bl_sz;                 /* block size */
  uint32_t           bl_cnt;                /* block count */
  uint32_t           n;                     /* allocated block index */
  uint32_t           status;                /* status flags */
} MemPool_t;

#endif /* FREERTOS_MPOOL_H_ */