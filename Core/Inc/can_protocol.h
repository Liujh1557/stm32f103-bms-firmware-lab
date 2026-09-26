#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>

#define CAN_PROTOCOL_STATUS_ID    0x321U
#define CAN_PROTOCOL_COMMAND_ID   0x322U
#define CAN_PROTOCOL_HEARTBEAT_ID 0x323U
#define CAN_PROTOCOL_DLC          8U
#define CAN_PROTOCOL_STATUS_VALID_BIT     0x01U
#define CAN_PROTOCOL_STATUS_SIMULATED_BIT 0x80U

uint8_t CanProtocol_IsPeerFrameFresh(uint32_t now_ms,
                                      uint32_t received_at_ms,
                                      uint32_t timeout_ms);

void CanProtocol_EncodeStatus(uint16_t cell0_mv,
                              uint32_t fault_flags,
                              uint8_t valid,
                              uint8_t fault_code,
                              uint8_t simulated,
                              uint8_t output[CAN_PROTOCOL_DLC]);

void CanProtocol_EncodeHeartbeat(uint32_t uptime_seconds,
                                 uint8_t sequence,
                                 uint8_t protection_state,
                                 uint8_t fault_latched,
                                 uint8_t peer_online,
                                 uint8_t output[CAN_PROTOCOL_DLC]);

#endif /* CAN_PROTOCOL_H */
