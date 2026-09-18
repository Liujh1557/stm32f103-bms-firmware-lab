#include "can_if.h"

#include <string.h>

static CAN_HandleTypeDef *g_can_handle = 0;
static CanIfFrame g_can_rx_frame = {0};
static volatile uint8_t g_can_rx_ready = 0U;
static volatile uint32_t g_can_rx_drop_count = 0U;
static volatile uint32_t g_can_error_count = 0U;

uint8_t CanIf_Init(CAN_HandleTypeDef *handle)
{
    CAN_FilterTypeDef filter = {0};

    if (handle == 0) {
        return 0U;
    }

    g_can_handle = handle;
    g_can_rx_ready = 0U;
    g_can_rx_drop_count = 0U;
    g_can_error_count = 0U;

    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;

    if (HAL_CAN_ConfigFilter(handle, &filter) != HAL_OK) {
        return 0U;
    }
    if (HAL_CAN_Start(handle) != HAL_OK) {
        return 0U;
    }
    if (HAL_CAN_ActivateNotification(handle,
                                     CAN_IT_RX_FIFO0_MSG_PENDING
                                     | CAN_IT_RX_FIFO0_OVERRUN) != HAL_OK) {
        return 0U;
    }

    return 1U;
}

uint8_t CanIf_SendStandard(uint16_t standard_id,
                           const uint8_t *data,
                           uint8_t dlc)
{
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox;

    if ((g_can_handle == 0)
        || (data == 0)
        || (standard_id > 0x7FFU)
        || (dlc > 8U)
        || (HAL_CAN_GetTxMailboxesFreeLevel(g_can_handle) == 0U)) {
        return 0U;
    }

    header.StdId = standard_id;
    header.ExtId = 0U;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = dlc;
    header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(g_can_handle,
                             &header,
                             (uint8_t *)data,
                             &mailbox) != HAL_OK) {
        return 0U;
    }

    return 1U;
}

uint8_t CanIf_TakeRxFrame(CanIfFrame *frame)
{
    if ((frame == 0) || (g_can_rx_ready == 0U)) {
        return 0U;
    }

    __DMB();
    *frame = g_can_rx_frame;
    g_can_rx_ready = 0U;
    return 1U;
}

uint32_t CanIf_GetRxDropCount(void)
{
    return g_can_rx_drop_count;
}

uint32_t CanIf_GetErrorCount(void)
{
    return g_can_error_count;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];

    if (hcan != g_can_handle) {
        return;
    }

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, data) != HAL_OK) {
        g_can_error_count++;
        return;
    }

    if ((header.IDE != CAN_ID_STD)
        || (header.RTR != CAN_RTR_DATA)
        || (header.DLC > 8U)) {
        g_can_error_count++;
        return;
    }

    if (g_can_rx_ready != 0U) {
        g_can_rx_drop_count++;
        return;
    }

    g_can_rx_frame.standard_id = (uint16_t)header.StdId;
    g_can_rx_frame.dlc = (uint8_t)header.DLC;
    memset(g_can_rx_frame.data, 0, sizeof(g_can_rx_frame.data));
    memcpy(g_can_rx_frame.data, data, header.DLC);
    __DMB();
    g_can_rx_ready = 1U;
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan == g_can_handle) {
        g_can_error_count++;
    }
}
