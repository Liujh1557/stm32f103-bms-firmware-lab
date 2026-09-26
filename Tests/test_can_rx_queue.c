#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "can_rx_queue.h"

int main(void)
{
    CanRxQueue queue;
    CanIfFrame input = {0};
    CanIfFrame output = {0};
    uint32_t index;

    CanRxQueue_Init(&queue);
    assert(CanRxQueue_GetDepth(&queue) == 0U);
    assert(CanRxQueue_Pop(&queue, &output) == 0U);

    for (index = 0U; index < CAN_RX_QUEUE_CAPACITY; index++) {
        input.standard_id = (uint16_t)(0x300U + index);
        input.data[0] = (uint8_t)index;
        input.received_at_ms = 1000U + index;
        assert(CanRxQueue_PushFromIsr(&queue, &input) != 0U);
    }
    assert(CanRxQueue_GetDepth(&queue) == CAN_RX_QUEUE_CAPACITY);
    assert(CanRxQueue_PushFromIsr(&queue, &input) == 0U);

    for (index = 0U; index < 16U; index++) {
        assert(CanRxQueue_Pop(&queue, &output) != 0U);
        assert(output.standard_id == (uint16_t)(0x300U + index));
        assert(output.data[0] == (uint8_t)index);
        assert(output.received_at_ms == 1000U + index);
    }
    assert(CanRxQueue_GetDepth(&queue) == 16U);

    for (index = CAN_RX_QUEUE_CAPACITY;
         index < CAN_RX_QUEUE_CAPACITY + 16U;
         index++) {
        input.standard_id = (uint16_t)(0x300U + index);
        input.data[0] = (uint8_t)index;
        input.received_at_ms = 1000U + index;
        assert(CanRxQueue_PushFromIsr(&queue, &input) != 0U);
    }
    assert(CanRxQueue_GetDepth(&queue) == CAN_RX_QUEUE_CAPACITY);

    for (index = 16U; index < CAN_RX_QUEUE_CAPACITY + 16U; index++) {
        assert(CanRxQueue_Pop(&queue, &output) != 0U);
        assert(output.standard_id == (uint16_t)(0x300U + index));
        assert(output.data[0] == (uint8_t)index);
        assert(output.received_at_ms == 1000U + index);
    }
    assert(CanRxQueue_GetDepth(&queue) == 0U);
    assert(CanRxQueue_Pop(&queue, &output) == 0U);

    puts("can_rx_queue tests passed");
    return 0;
}
