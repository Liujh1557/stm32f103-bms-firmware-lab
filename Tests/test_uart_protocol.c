#include <assert.h>
#include <stdio.h>

#include "uart_protocol.h"

int main(void)
{
    const uint8_t valid_ping[] = {0xAAU, 0x55U, 0x01U,
                                  0x01U, 0xC1U, 0xE0U};
    uint8_t invalid_frame[sizeof(valid_ping)];
    UartProtocolFrame frame;
    uint8_t index;

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

    puts("uart_protocol tests passed");
    return 0;
}
