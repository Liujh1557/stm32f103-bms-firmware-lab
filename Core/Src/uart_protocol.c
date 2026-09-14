#include "uart_protocol.h"

#include <string.h>

#define UART_PROTOCOL_MIN_FRAME_SIZE 6U

uint16_t UartProtocol_Crc16Modbus(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    uint16_t index;
    uint8_t bit;

    for (index = 0U; index < length; index++) {
        crc ^= data[index];
        for (bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

UartProtocolResult UartProtocol_Parse(const uint8_t *data,
                                      uint16_t length,
                                      UartProtocolFrame *frame)
{
    uint8_t content_length;
    uint16_t expected_length;
    uint16_t received_crc;
    uint16_t calculated_crc;

    if ((data == 0) || (frame == 0) || (length < UART_PROTOCOL_MIN_FRAME_SIZE)) {
        return UART_PROTOCOL_ERROR_LENGTH;
    }

    if ((data[0] != UART_PROTOCOL_SOF1) || (data[1] != UART_PROTOCOL_SOF2)) {
        return UART_PROTOCOL_ERROR_HEADER;
    }

    content_length = data[2];
    if ((content_length == 0U)
        || (content_length > (UART_PROTOCOL_MAX_PAYLOAD_SIZE + 1U))) {
        return UART_PROTOCOL_ERROR_LENGTH;
    }

    expected_length = (uint16_t)content_length + 5U;
    if (length != expected_length) {
        return UART_PROTOCOL_ERROR_LENGTH;
    }

    received_crc = (uint16_t)data[length - 2U]
                   | ((uint16_t)data[length - 1U] << 8U);
    calculated_crc = UartProtocol_Crc16Modbus(&data[2],
                                               (uint16_t)content_length + 1U);
    if (received_crc != calculated_crc) {
        return UART_PROTOCOL_ERROR_CRC;
    }

    frame->command = data[3];
    frame->payload_length = (uint8_t)(content_length - 1U);
    if (frame->payload_length > 0U) {
        memcpy(frame->payload, &data[4], frame->payload_length);
    }

    return UART_PROTOCOL_OK;
}
