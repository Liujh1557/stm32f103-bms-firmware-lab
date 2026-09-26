#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>

#define UART_PROTOCOL_SOF1             0xAAU
#define UART_PROTOCOL_SOF2             0x55U
#define UART_PROTOCOL_CMD_PING         0x01U
#define UART_PROTOCOL_CMD_SIM_CELL0    0x20U
#define UART_PROTOCOL_CMD_SIM_CLEAR    0x21U
#define UART_PROTOCOL_MAX_PAYLOAD_SIZE 32U

typedef enum
{
    UART_PROTOCOL_OK = 0U,
    UART_PROTOCOL_ERROR_HEADER,
    UART_PROTOCOL_ERROR_LENGTH,
    UART_PROTOCOL_ERROR_CRC
} UartProtocolResult;

typedef struct
{
    uint8_t command;
    uint8_t payload_length;
    uint8_t payload[UART_PROTOCOL_MAX_PAYLOAD_SIZE];
} UartProtocolFrame;

uint16_t UartProtocol_Crc16Modbus(const uint8_t *data, uint16_t length);
UartProtocolResult UartProtocol_Parse(const uint8_t *data,
                                      uint16_t length,
                                      UartProtocolFrame *frame);

#endif /* UART_PROTOCOL_H */
