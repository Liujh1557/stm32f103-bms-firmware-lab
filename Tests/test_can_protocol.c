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

    CanProtocol_EncodeStatus(1500U, 0x78563412UL, 1U, 2U, 0U, frame);
    assert(memcmp(frame, expected_status, sizeof(frame)) == 0);
    CanProtocol_EncodeStatus(1500U, 0x78563412UL, 1U, 2U, 1U, frame);
    assert(frame[6] == (CAN_PROTOCOL_STATUS_VALID_BIT
                        | CAN_PROTOCOL_STATUS_SIMULATED_BIT));

    CanProtocol_EncodeHeartbeat(0x12345678UL, 0x9AU, 2U, 1U, 0U, frame);
    assert(memcmp(frame, expected_heartbeat, sizeof(frame)) == 0);

    assert(CanProtocol_IsPeerFrameFresh(1000U, 0U, 1000U) == 1U);
    assert(CanProtocol_IsPeerFrameFresh(1001U, 0U, 1000U) == 0U);
    assert(CanProtocol_IsPeerFrameFresh(5U, 0xFFFFFFF0UL, 21U) == 1U);
    assert(CanProtocol_IsPeerFrameFresh(5U, 0xFFFFFFF0UL, 20U) == 0U);

    puts("can_protocol tests passed");
    return 0;
}
