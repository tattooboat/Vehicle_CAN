
#include "int_can.h"
#include "can.h"
#include "App_MotorCan.h"
#include "App_Fault.h"
#include <string.h>

extern CAN_HandleTypeDef hcan1;

/* 电机 CAN 控制帧 ID，与 App_MotorCan 中保持一致 */
#ifndef MOTOR_CAN_ID
#define MOTOR_CAN_ID  0x301
#endif

/*
 * 启动 CAN1，配置过滤器接收全部标准帧和扩展帧，
 * 并启用 FIFO0 接收中断。
 */
HAL_StatusTypeDef INT_CAN_Start(void)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;

    /*
     * 单个 bxCAN 控制器使用 Bank 0 即可。
     * 如果将来使用 CAN1/CAN2 双控制器，需要重新规划过滤器银行。
     */
    filter.SlaveStartFilterBank = 14;

    // if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK)
    // {
    //     return HAL_ERROR;
    // }
    HAL_StatusTypeDef status = HAL_CAN_ConfigFilter(&hcan1, &filter);
    if (status != HAL_OK) return status;   // 把真实的 HAL_TIMEOUT 透出来   

    if (HAL_CAN_Start(&hcan1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_CAN_ActivateNotification(
            &hcan1,
            CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* 发送标准数据帧，ID 范围 0x000～0x7FF */
HAL_StatusTypeDef INT_CAN_SendStd(
    uint16_t std_id,
    const uint8_t *data,
    uint8_t length,
    uint32_t timeout_ms)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t mailbox;
    uint32_t start_tick;

    if ((std_id > 0x7FFU) ||
        (length > 8U) ||
        ((length > 0U) && (data == NULL)))
    {
        return HAL_ERROR;
    }

    tx_header.StdId = std_id;
    tx_header.ExtId = 0;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = length;
    tx_header.TransmitGlobalTime = DISABLE;

    start_tick = HAL_GetTick();

    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0U)
    {
        if ((HAL_GetTick() - start_tick) >= timeout_ms)
        {
            return HAL_TIMEOUT;
        }
    }

    if (HAL_CAN_AddTxMessage(
            &hcan1,
            &tx_header,
            (uint8_t *)data,
            &mailbox) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* 从 FIFO0 读取一帧；FIFO 为空时返回 HAL_ERROR */
HAL_StatusTypeDef INT_CAN_Receive(INT_CAN_Frame_t *frame)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if (frame == NULL)
    {
        return HAL_ERROR;
    }

    if (HAL_CAN_GetRxFifoFillLevel(
            &hcan1,
            CAN_RX_FIFO0) == 0U)
    {
        return HAL_ERROR;
    }

    if (HAL_CAN_GetRxMessage(
            &hcan1,
            CAN_RX_FIFO0,
            &rx_header,
            rx_data) != HAL_OK)
    {
        return HAL_ERROR;
    }

    frame->id = (rx_header.IDE == CAN_ID_STD)
              ? rx_header.StdId
              : rx_header.ExtId;

    frame->dlc = rx_header.DLC;
    frame->is_extended = (rx_header.IDE == CAN_ID_EXT);
    frame->is_remote = (rx_header.RTR == CAN_RTR_REMOTE);

    memset(frame->data, 0, sizeof(frame->data));

    if (frame->is_remote == 0U)
    {
        memcpy(frame->data, rx_data, rx_header.DLC);
    }

    return HAL_OK;
}

volatile uint32_t g_can_last_id = 0;//最后接收帧的 ID
volatile uint8_t  g_can_last_dlc = 0;//最后接收帧的 DLC
volatile uint8_t  g_can_last_data[8] = {0};//最后接收帧的数据
// 接收中断回调函数
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if (hcan->Instance != CAN1)
    {
        return;
    }

    while (HAL_CAN_GetRxFifoFillLevel(
               hcan, CAN_RX_FIFO0) > 0U)
    {
        if (HAL_CAN_GetRxMessage(
                hcan,
                CAN_RX_FIFO0,
                &rx_header,
                rx_data) != HAL_OK)
        {
            break;
        }
        CAN_Protocol_OnFrame(
            (rx_header.IDE == CAN_ID_STD)
                ? rx_header.StdId
                : rx_header.ExtId,
            (rx_header.IDE == CAN_ID_EXT) ? 1U : 0U,
            (rx_header.RTR == CAN_RTR_REMOTE) ? 1U : 0U,
            (uint8_t)rx_header.DLC,
            rx_data
        );

        /* 电机控制帧（0x301）由 App_MotorCan 处理 */
        if ((rx_header.IDE == CAN_ID_STD) &&
            (rx_header.StdId == MOTOR_CAN_ID) &&
            (rx_header.RTR == CAN_RTR_DATA))
        {
            App_MotorCan_OnRx(
                rx_header.StdId,
                rx_data,
                (uint8_t)rx_header.DLC
            );
        }
        
        g_can_last_id = (rx_header.IDE == CAN_ID_STD)
                      ? rx_header.StdId
                      : rx_header.ExtId;

        g_can_last_dlc = rx_header.DLC;

        for (uint8_t i = 0; i < 8U; i++)
        {
            g_can_last_data[i] =
                (i < rx_header.DLC && rx_header.RTR == CAN_RTR_DATA)
                ? rx_data[i]
                : 0U;
        }

        g_can_rx_count++;
    }
}
