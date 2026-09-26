#include "can_rx_queue.h"

static uint8_t CanRxQueue_Next(uint8_t index)
{
    index++;
    if (index >= CAN_RX_QUEUE_STORAGE_SIZE) {
        index = 0U;
    }
    return index;
}

void CanRxQueue_Init(CanRxQueue *queue)
{
    if (queue != 0) {
        queue->head = 0U;
        queue->tail = 0U;
    }
}

uint8_t CanRxQueue_PushFromIsr(CanRxQueue *queue, const CanIfFrame *frame)
{
    uint8_t head;
    uint8_t next;

    if ((queue == 0) || (frame == 0)) {
        return 0U;
    }

    head = __atomic_load_n(&queue->head, __ATOMIC_RELAXED);
    next = CanRxQueue_Next(head);
    if (next == __atomic_load_n(&queue->tail, __ATOMIC_ACQUIRE)) {
        return 0U;
    }

    queue->frames[head] = *frame;
    __atomic_store_n(&queue->head, next, __ATOMIC_RELEASE);
    return 1U;
}

uint8_t CanRxQueue_Pop(CanRxQueue *queue, CanIfFrame *frame)
{
    uint8_t tail;

    if ((queue == 0) || (frame == 0)) {
        return 0U;
    }

    tail = __atomic_load_n(&queue->tail, __ATOMIC_RELAXED);
    if (tail == __atomic_load_n(&queue->head, __ATOMIC_ACQUIRE)) {
        return 0U;
    }

    *frame = queue->frames[tail];
    __atomic_store_n(&queue->tail, CanRxQueue_Next(tail), __ATOMIC_RELEASE);
    return 1U;
}

uint8_t CanRxQueue_GetDepth(const CanRxQueue *queue)
{
    uint8_t head;
    uint8_t tail;

    if (queue == 0) {
        return 0U;
    }

    head = __atomic_load_n(&queue->head, __ATOMIC_ACQUIRE);
    tail = __atomic_load_n(&queue->tail, __ATOMIC_ACQUIRE);
    if (head >= tail) {
        return (uint8_t)(head - tail);
    }
    return (uint8_t)(CAN_RX_QUEUE_STORAGE_SIZE - tail + head);
}
