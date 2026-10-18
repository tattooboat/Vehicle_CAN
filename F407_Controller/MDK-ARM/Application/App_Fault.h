#ifndef APP_FAULT_H
#define APP_FAULT_H

#include "main.h"
#include <stdint.h>

/* 故障标志位，可以同时存在多个故障 */
#define FAULT_NONE             0x00000000U
#define FAULT_CAN_TIMEOUT      0x00000001U
#define FAULT_CAN_BAD_FRAME    0x00000002U
#define FAULT_CAN_BAD_COMMAND  0x00000004U
#define FAULT_CAN_BAD_SPEED    0x00000008U

extern volatile uint32_t g_fault_flags;
extern volatile uint32_t g_can_rx_count;
extern volatile uint32_t g_can_invalid_count;

void App_Fault_Set(uint32_t fault);
void App_Fault_Clear(uint32_t fault);

#endif
