#include <assert.h>
#include <stdio.h>

#include "uart_protocol.h"

int main(void)
{
    const uint8_t valid_ping[] = {0xAAU, 0x55U, 0x01U,
                                  0x01U, 0xC1U, 0xE0U};
    uint8_t invalid_frame[sizeof(valid_ping)];
    uint8_t sim_frame[] = {0xAAU, 0x55U, 0x03U, 0x20U,
                           0x80U, 0x0CU, 0U, 0U};
    uint8_t clear_frame[] = {0xAAU, 0x55U, 0x01U, 0x21U, 0U, 0U};
    UartProtocolFrame frame;
    uint8_t index;
    uint16_t crc;

    assert(UartProtocol_Crc16Modbus(&valid_ping[2], 2U) == 0xE0C1U);
    assert(UartProtocol_Parse(valid_ping,
                              sizeof(valid_ping),
                              &frame) == UART_PROTOCOL_OK);
    assert(frame.command == UART_PROTOCOL_CMD_PING);
    assert(frame.payload_length == 0U);

    for (index = 0U; index < sizeof(valid_ping); index++) {
        invalid_frame[index] = valid_ping[index];
    }
    invalid_frame[0] = 0xABU;
    assert(UartProtocol_Parse(invalid_frame,
                              sizeof(invalid_frame),
                              &frame) == UART_PROTOCOL_ERROR_HEADER);

    invalid_frame[0] = UART_PROTOCOL_SOF1;
    invalid_frame[2] = 0x02U;
    assert(UartProtocol_Parse(invalid_frame,
                              sizeof(invalid_frame),
                              &frame) == UART_PROTOCOL_ERROR_LENGTH);

    invalid_frame[2] = 0x01U;
    invalid_frame[4] ^= 0x01U;
    assert(UartProtocol_Parse(invalid_frame,
                              sizeof(invalid_frame),
                              &frame) == UART_PROTOCOL_ERROR_CRC);

    crc = UartProtocol_Crc16Modbus(&sim_frame[2], 4U);
    sim_frame[6] = (uint8_t)crc;
    sim_frame[7] = (uint8_t)(crc >> 8U);
    assert(UartProtocol_Parse(sim_frame, sizeof(sim_frame),
                              &frame) == UART_PROTOCOL_OK);
    assert(frame.command == UART_PROTOCOL_CMD_SIM_CELL0);
    assert(frame.payload_length == 2U);
    assert(((uint16_t)frame.payload[0]
            | ((uint16_t)frame.payload[1] << 8U)) == 3200U);

    crc = UartProtocol_Crc16Modbus(&clear_frame[2], 2U);
    clear_frame[4] = (uint8_t)crc;
    clear_frame[5] = (uint8_t)(crc >> 8U);
    assert(UartProtocol_Parse(clear_frame, sizeof(clear_frame),
                              &frame) == UART_PROTOCOL_OK);
    assert(frame.command == UART_PROTOCOL_CMD_SIM_CLEAR);
    assert(frame.payload_length == 0U);

    puts("uart_protocol tests passed");
    return 0;
}
