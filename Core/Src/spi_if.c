#include "spi_if.h"

static SPI_HandleTypeDef *g_spi_handle = 0;
static volatile uint8_t g_spi_busy = 0U;
static volatile uint8_t g_spi_complete_pending = 0U;
static volatile uint8_t g_spi_error_pending = 0U;

void SpiIf_Init(SPI_HandleTypeDef *handle)
{
    g_spi_handle = handle;
    g_spi_busy = 0U;
    g_spi_complete_pending = 0U;
    g_spi_error_pending = 0U;
}

uint8_t SpiIf_StartTransfer(uint8_t *tx_buffer,
                            uint8_t *rx_buffer,
                            uint16_t length)
{
    if ((g_spi_handle == 0)
        || (tx_buffer == 0)
        || (rx_buffer == 0)
        || (length == 0U)
        || (g_spi_busy != 0U)
        || (g_spi_complete_pending != 0U)
        || (g_spi_error_pending != 0U)) {
        return 0U;
    }

    g_spi_busy = 1U;

    if (HAL_SPI_TransmitReceive_DMA(g_spi_handle,
                                    tx_buffer,
                                    rx_buffer,
                                    length) != HAL_OK) {
        g_spi_busy = 0U;
        return 0U;
    }

    return 1U;
}

uint8_t SpiIf_TakeComplete(void)
{
    if (g_spi_complete_pending == 0U) {
        return 0U;
    }

    __DMB();
    g_spi_complete_pending = 0U;
    return 1U;
}

uint8_t SpiIf_TakeError(void)
{
    if (g_spi_error_pending == 0U) {
        return 0U;
    }

    g_spi_error_pending = 0U;
    return 1U;
}

uint8_t SpiIf_IsBusy(void)
{
    return g_spi_busy;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_spi_handle) {
        g_spi_busy = 0U;
        __DMB();
        g_spi_complete_pending = 1U;
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_spi_handle) {
        g_spi_busy = 0U;
        g_spi_error_pending = 1U;
    }
}
