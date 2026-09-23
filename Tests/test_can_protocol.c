#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "can_protocol.h"

int main(void)
{
    uint8_t frame[CAN_PROTOCOL_DLC];
    const uint8_t expected_status[CAN_PROTOCOL_DLC] = {
        0xDCU, 0x05U, 0x12U, 0x34U, 0x56U, 0x78U, 0x01U, 0x02U
    };
    const uint8_t expected_heartbeat[CAN_PROTOCOL_DLC] = {
        0x78U, 0x56U, 0x34U, 0x12U, 0x9AU, 0x02U, 0x01U, 0x00U
    };

    CanProtocol_EncodeStatus(1500U, 0x78563412UL, 1U, 2U, frame);
    assert(memcmp(frame, expected_status, sizeof(frame)) == 0);

    CanProtocol_EncodeHeartbeat(0x12345678UL, 0x9AU, 2U, 1U, 0U, frame);
    assert(memcmp(frame, expected_heartbeat, sizeof(frame)) == 0);

    puts("can_protocol tests passed");
    return 0;
}
