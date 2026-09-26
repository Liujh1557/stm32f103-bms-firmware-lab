#ifndef CAN_IF_H
#define CAN_IF_H

#include "can_rx_queue.h"
#include "stm32f1xx_hal.h"

uint8_t CanIf_Init(CAN_HandleTypeDef *handle);
uint8_t CanIf_SendStandard(uint16_t standard_id,
                           const uint8_t *data,
                           uint8_t dlc);
uint8_t CanIf_TakeRxFrame(CanIfFrame *frame);
void CanIf_Service(void);
uint32_t CanIf_GetTxCompleteCount(void);
uint32_t CanIf_GetRxDropCount(void);
uint32_t CanIf_GetRxSeenCount(void);
uint32_t CanIf_GetRxHwOverrunCount(void);
uint8_t CanIf_GetRxQueueDepth(void);
uint8_t CanIf_GetRxQueuePeak(void);
uint32_t CanIf_GetErrorCount(void);
uint32_t CanIf_GetLastError(void);
uint32_t CanIf_GetRecoveryCount(void);
uint32_t CanIf_GetRecoveryFailCount(void);
uint8_t CanIf_IsFaulted(void);

#endif /* CAN_IF_H */
