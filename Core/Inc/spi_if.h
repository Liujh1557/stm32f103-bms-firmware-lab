#ifndef SPI_IF_H
#define SPI_IF_H

#include "stm32f1xx_hal.h"

void SpiIf_Init(SPI_HandleTypeDef *handle);
uint8_t SpiIf_StartTransfer(uint8_t *tx_buffer,
                            uint8_t *rx_buffer,
                            uint16_t length);
uint8_t SpiIf_TakeComplete(void);
uint8_t SpiIf_TakeError(void);
uint8_t SpiIf_IsBusy(void);

#endif /* SPI_IF_H */
