#ifndef CAN_RX_QUEUE_H
#define CAN_RX_QUEUE_H

#include <stdint.h>

/* One extra slot separates full from empty in the ISR-producer/main-consumer ring. */
#define CAN_RX_QUEUE_CAPACITY 32U
#define CAN_RX_QUEUE_STORAGE_SIZE (CAN_RX_QUEUE_CAPACITY + 1U)

typedef struct
{
    uint16_t standard_id;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t received_at_ms;
} CanIfFrame;

typedef struct
{
    CanIfFrame frames[CAN_RX_QUEUE_STORAGE_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
} CanRxQueue;

void CanRxQueue_Init(CanRxQueue *queue);
uint8_t CanRxQueue_PushFromIsr(CanRxQueue *queue, const CanIfFrame *frame);
uint8_t CanRxQueue_Pop(CanRxQueue *queue, CanIfFrame *frame);
uint8_t CanRxQueue_GetDepth(const CanRxQueue *queue);

#endif /* CAN_RX_QUEUE_H */
