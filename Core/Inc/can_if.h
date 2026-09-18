#ifndef CAN_IF_H
#define CAN_IF_H

#include "stm32f1xx_hal.h"

typedef struct
{
    uint16_t standard_id;
    uint8_t dlc;
    uint8_t data[8];
} CanIfFrame;

uint8_t CanIf_Init(CAN_HandleTypeDef *handle);
uint8_t CanIf_SendStandard(uint16_t standard_id,
                           const uint8_t *data,
                           uint8_t dlc);
uint8_t CanIf_TakeRxFrame(CanIfFrame *frame);
uint32_t CanIf_GetTxCompleteCount(void);
uint32_t CanIf_GetRxDropCount(void);
uint32_t CanIf_GetErrorCount(void);
uint32_t CanIf_GetLastError(void);

#endif /* CAN_IF_H */
