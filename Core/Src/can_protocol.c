#include "can_protocol.h"

uint8_t CanProtocol_IsPeerFrameFresh(uint32_t now_ms,
                                      uint32_t received_at_ms,
                                      uint32_t timeout_ms)
{
    return ((uint32_t)(now_ms - received_at_ms) <= timeout_ms) ? 1U : 0U;
}

void CanProtocol_EncodeStatus(uint16_t cell0_mv,
                              uint32_t fault_flags,
                              uint8_t valid,
                              uint8_t fault_code,
                              uint8_t simulated,
                              uint8_t output[CAN_PROTOCOL_DLC])
{
    output[0] = (uint8_t)cell0_mv;
    output[1] = (uint8_t)(cell0_mv >> 8U);
    output[2] = (uint8_t)fault_flags;
    output[3] = (uint8_t)(fault_flags >> 8U);
    output[4] = (uint8_t)(fault_flags >> 16U);
    output[5] = (uint8_t)(fault_flags >> 24U);
    output[6] = ((valid != 0U) ? CAN_PROTOCOL_STATUS_VALID_BIT : 0U)
                | ((simulated != 0U) ? CAN_PROTOCOL_STATUS_SIMULATED_BIT : 0U);
    output[7] = fault_code;
}

void CanProtocol_EncodeHeartbeat(uint32_t uptime_seconds,
                                 uint8_t sequence,
                                 uint8_t protection_state,
                                 uint8_t fault_latched,
                                 uint8_t peer_online,
                                 uint8_t output[CAN_PROTOCOL_DLC])
{
    output[0] = (uint8_t)uptime_seconds;
    output[1] = (uint8_t)(uptime_seconds >> 8U);
    output[2] = (uint8_t)(uptime_seconds >> 16U);
    output[3] = (uint8_t)(uptime_seconds >> 24U);
    output[4] = sequence;
    output[5] = protection_state;
    output[6] = fault_latched;
    output[7] = peer_online;
}
