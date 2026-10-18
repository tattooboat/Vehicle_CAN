#include "Int_can.h"
#include "can.h"
#include <string.h>

extern CAN_HandleTypeDef hcan;

volatile uint32_t INT_can_rx_count = 0;
volatile uint32_t INT_can_last_id = 0;
volatile uint8_t INT_can_last_dlc = 0;
volatile uint8_t INT_can_last_data[8] = {0};

static volatile uint8_t can_frame_pending = 0;

/* 配置过滤器、启动 CAN、启用 FIFO0 接收中断 */
HAL_StatusTypeDef INT_CAN_Start(void)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    /* 掩码为 0，接收所有标准帧和扩展帧 */
    filter.FilterIdHigh = 0;
    filter.FilterIdLow = 0;
    filter.FilterMaskIdHigh = 0;
    filter.FilterMaskIdLow = 0;

    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;

    /* 单 CAN 控制器工程中此参数通常不影响 Bank 0 的使用 */
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK)
        return HAL_ERROR;

    if (HAL_CAN_Start(&hcan) != HAL_OK)
        return HAL_ERROR;

    if (HAL_CAN_ActivateNotification(
            &hcan,
            CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

/* 发送标准 CAN 数据帧 */
HAL_StatusTypeDef INT_CAN_SendStd(
    uint16_t std_id,
    const uint8_t *data,
    uint8_t length)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t mailbox;
    uint32_t start_tick = HAL_GetTick();

    if (std_id > 0x7FFU ||
        length > 8U ||
        (length > 0U && data == NULL))
        return HAL_ERROR;

    tx_header.StdId = std_id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = length;
    tx_header.TransmitGlobalTime = DISABLE;

    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0U)
    {
        if ((HAL_GetTick() - start_tick) >= 100U)
            return HAL_TIMEOUT;
    }

    return HAL_CAN_AddTxMessage(
        &hcan,
        &tx_header,
        (uint8_t *)data,
        &mailbox);
}

/* 应用层读取最近一帧 */
uint8_t INT_CAN_GetLastFrame(INT_CAN_Frame_t *frame)
{
    if (frame == NULL || can_frame_pending == 0U)
        return 0U;

    __disable_irq();

    frame->id = INT_can_last_id;
    frame->dlc = INT_can_last_dlc;

    for (uint8_t i = 0; i < 8U; i++)
        frame->data[i] = INT_can_last_data[i];

    frame->is_extended = 0U;
    frame->is_remote = 0U;

    can_frame_pending = 0U;

    __enable_irq();

    return 1U;
}

/* FIFO0 接收中断回调 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan_ptr)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if (hcan_ptr->Instance != CAN1)
        return;

    while (HAL_CAN_GetRxFifoFillLevel(
               hcan_ptr, CAN_RX_FIFO0) > 0U)
    {
        if (HAL_CAN_GetRxMessage(
                hcan_ptr,
                CAN_RX_FIFO0,
                &rx_header,
                rx_data) != HAL_OK)
            break;

        INT_can_last_id =
            (rx_header.IDE == CAN_ID_STD)
            ? rx_header.StdId
            : rx_header.ExtId;

        INT_can_last_dlc = rx_header.DLC;

        for (uint8_t i = 0; i < 8U; i++)
        {
            INT_can_last_data[i] =
                (i < rx_header.DLC &&
                 rx_header.RTR == CAN_RTR_DATA)
                ? rx_data[i]
                : 0U;
        }

        INT_can_rx_count++;
        can_frame_pending = 1U;
    }
}
