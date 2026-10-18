
#include "App_Fault.h"

volatile uint32_t g_fault_flags = FAULT_NONE;
volatile uint32_t g_can_rx_count = 0;
volatile uint32_t g_can_invalid_count = 0;

/* ISR and task both update flags; keep read-modify-write atomic. */
void App_Fault_Set(uint32_t fault)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    g_fault_flags |= fault;
    __set_PRIMASK(primask);
}

void App_Fault_Clear(uint32_t fault)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    g_fault_flags &= ~fault;
    __set_PRIMASK(primask);
}
