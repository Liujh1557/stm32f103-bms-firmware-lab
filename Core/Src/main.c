/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bms_protection.h"
#include "bms_types.h"
#include "can_if.h"
#include "spi_if.h"
#include "uart_protocol.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BMS_ADC_VREF_MV      3300U
#define BMS_ADC_MAX_COUNTS   4095U
#define UART_LOG_BUFFER_SIZE 448U
#define UART_RX_BUFFER_SIZE  64U
#define SPI_LOOPBACK_SIZE     4U
#define CAN_LOOPBACK_ID       0x321U
#define CAN_LOOPBACK_SIZE     8U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t g_system_ms = 0;
uint32_t last_10ms = 0;
uint32_t last_100ms = 0;
uint32_t last_1000ms = 0;
uint32_t count_10ms = 0;
uint32_t count_100ms = 0;
static uint16_t g_adc_dma_buffer[BMS_ADC_CHANNEL_COUNT] = {0};
static uint16_t g_adc_filter_samples[BMS_ADC_FILTER_LENGTH]
                                      [BMS_ADC_CHANNEL_COUNT] = {0};
static uint32_t g_adc_filter_sum[BMS_ADC_CHANNEL_COUNT] = {0};
static uint8_t g_adc_filter_index = 0U;
static uint8_t g_adc_filter_count = 0U;
static volatile uint8_t g_adc_busy = 0U;
static volatile uint8_t g_adc_frame_ready = 0U;
static volatile uint8_t g_adc_error_pending = 0U;
static uint32_t g_adc_frame_count = 0U;
static uint32_t g_adc_error_count = 0U;
static uint8_t g_uart_log_buffer[UART_LOG_BUFFER_SIZE] = {0};
static volatile uint8_t g_uart_tx_busy = 0U;
static uint32_t g_uart_tx_count = 0U;
static uint32_t g_uart_tx_drop_count = 0U;
static uint8_t g_uart_rx_dma_buffer[UART_RX_BUFFER_SIZE] = {0};
static uint8_t g_uart_rx_frame[UART_RX_BUFFER_SIZE] = {0};
static volatile uint16_t g_uart_rx_length = 0U;
static volatile uint8_t g_uart_rx_frame_ready = 0U;
static volatile uint8_t g_uart_rx_restart_pending = 0U;
static uint32_t g_uart_rx_count = 0U;
static volatile uint32_t g_uart_rx_drop_count = 0U;
static volatile uint32_t g_uart_error_count = 0U;
static uint32_t g_uart_crc_error_count = 0U;
static uint32_t g_uart_protocol_error_count = 0U;
static const uint8_t g_spi_tx_buffer[SPI_LOOPBACK_SIZE] = {
  0x12U, 0x34U, 0xA5U, 0x5AU
};
static uint8_t g_spi_rx_buffer[SPI_LOOPBACK_SIZE] = {0};
static uint8_t g_spi_last_rx_buffer[SPI_LOOPBACK_SIZE] = {0};
static uint8_t g_spi_last_ok = 0U;
static uint32_t g_spi_transfer_count = 0U;
static uint32_t g_spi_mismatch_count = 0U;
static uint32_t g_spi_error_count = 0U;
static const uint8_t g_can_tx_data[CAN_LOOPBACK_SIZE] = {
  0x12U, 0x34U, 0xA5U, 0x5AU, 0x01U, 0x02U, 0x03U, 0x04U
};
static uint8_t g_can_last_rx_data[CAN_LOOPBACK_SIZE] = {0};
static uint16_t g_can_last_rx_id = 0U;
static uint32_t g_can_tx_count = 0U;
static uint32_t g_can_rx_count = 0U;
static uint32_t g_can_mismatch_count = 0U;
static uint32_t g_can_send_error_count = 0U;
static BmsData g_bms_data = {0};
static BmsConfig g_bms_config = {0};
static BmsFault g_bms_fault = {0};
static BmsProtection g_bms_protection = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Task_10ms(void);
static void Task_100ms(void);
static void Task_1000ms(void);
static uint16_t AdcRawToMillivolts(uint16_t raw);
static void ProcessAdcFrame(void);
static uint8_t StartAdcFrame(void);
static uint8_t StartUartLog(uint16_t length);
static uint8_t StartUartRx(void);
static void ProcessUartRx(void);
static uint8_t StartSpiLoopbackDma(void);
static void ProcessSpiLoopbackResult(void);
static void StartCanLoopbackTest(void);
static void ProcessCanLoopbackResult(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
    Error_Handler();
  }

  /* Teaching thresholds for a safe 0-3.3 V virtual cell input on PA0.
   * These values are not real lithium-cell protection parameters. */
  g_bms_config.monitored_cell_count = 1U;
  g_bms_config.over_voltage_mv = 3000U;
  g_bms_config.under_voltage_mv = 300U;
  g_bms_config.voltage_hysteresis_mv = 100U;
  g_bms_config.fault_confirm_ms = 300U;
  g_bms_config.recovery_ms = 500U;
  BmsProtection_Init(&g_bms_protection, &g_bms_fault);
  SpiIf_Init(&hspi1);

  if (CanIf_Init(&hcan) == 0U) {
    Error_Handler();
  }

  if (StartUartRx() == 0U) {
    Error_Handler();
  }

  if (StartSpiLoopbackDma() == 0U) {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = g_system_ms;

    if ((uint32_t)(now - last_10ms) >= 10U) {
      last_10ms += 10U;
      Task_10ms();
    }

    if ((uint32_t)(now - last_100ms) >= 100U) {
      last_100ms += 100U;
      Task_100ms();
    }

    if ((uint32_t)(now - last_1000ms) >= 1000U) {
      last_1000ms += 1000U;
      Task_1000ms();
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void Task_10ms(void)
{
  uint8_t adc_can_start = 1U;

  count_10ms++;

  if (g_uart_rx_restart_pending != 0U) {
    if (StartUartRx() != 0U) {
      g_uart_rx_restart_pending = 0U;
    }
  }

  ProcessUartRx();
  ProcessSpiLoopbackResult();
  ProcessCanLoopbackResult();

  if (g_adc_error_pending != 0U) {
    g_adc_error_pending = 0U;
    g_bms_data.valid = 0U;

    if (HAL_ADC_Stop_DMA(&hadc1) != HAL_OK) {
      g_adc_error_count++;
      adc_can_start = 0U;
    }
  }

  if (g_adc_frame_ready != 0U) {
    g_adc_frame_ready = 0U;
    __DMB();

    if (HAL_ADC_Stop_DMA(&hadc1) == HAL_OK) {
      ProcessAdcFrame();
      g_adc_frame_count++;
    } else {
      g_adc_error_count++;
      g_bms_data.valid = 0U;
      adc_can_start = 0U;
    }
  }

  if ((g_adc_busy == 0U) && (adc_can_start != 0U)) {
    if (StartAdcFrame() == 0U) {
      g_adc_error_count++;
      g_bms_data.valid = 0U;
    }
  }
}

static void Task_100ms(void)
{
  count_100ms++;
  BmsProtection_Update(&g_bms_protection,
                       &g_bms_data,
                       &g_bms_config,
                       &g_bms_fault,
                       100U);
  StartCanLoopbackTest();
}

static void Task_1000ms(void)
{
  int length;

  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

  if (StartSpiLoopbackDma() == 0U) {
    g_spi_error_count++;
  }

  if (g_uart_tx_busy != 0U) {
    g_uart_tx_drop_count++;
    return;
  }

  length = snprintf((char *)g_uart_log_buffer,
                    sizeof(g_uart_log_buffer),
                    "t=%lu,valid=%u,adc_busy=%u,frames=%lu,adc_err=%lu,fn=%u,cell0=%u,raw0=%u,avg0=%u,fault=0x%08lx,code=%u,latch=%u,pstate=%u,tx=%lu,tx_drop=%lu,rx=%lu,rx_drop=%lu,uart_err=%lu,crc_err=%lu,proto_err=%lu,spi_busy=%u,spi_ok=%u,spi_n=%lu,spi_mis=%lu,spi_err=%lu,spi_rx=%02X%02X%02X%02X,can_tx=%lu,can_rx=%lu,can_mis=%lu,can_err=%lu,can_drop=%lu,can_id=%03X,can_data=%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                    (unsigned long)g_system_ms,
                    (unsigned int)g_bms_data.valid,
                    (unsigned int)g_adc_busy,
                    (unsigned long)g_adc_frame_count,
                    (unsigned long)g_adc_error_count,
                    (unsigned int)g_adc_filter_count,
                    (unsigned int)g_bms_data.cell_voltage_mv[0],
                    (unsigned int)g_bms_data.adc_raw[0],
                    (unsigned int)g_bms_data.adc_filtered[0],
                    (unsigned long)g_bms_fault.flags,
                    (unsigned int)g_bms_fault.active_code,
                    (unsigned int)g_bms_fault.latched,
                    (unsigned int)g_bms_protection.state,
                    (unsigned long)g_uart_tx_count,
                    (unsigned long)g_uart_tx_drop_count,
                    (unsigned long)g_uart_rx_count,
                    (unsigned long)g_uart_rx_drop_count,
                    (unsigned long)g_uart_error_count,
                    (unsigned long)g_uart_crc_error_count,
                    (unsigned long)g_uart_protocol_error_count,
                    (unsigned int)SpiIf_IsBusy(),
                    (unsigned int)g_spi_last_ok,
                    (unsigned long)g_spi_transfer_count,
                    (unsigned long)g_spi_mismatch_count,
                    (unsigned long)g_spi_error_count,
                    (unsigned int)g_spi_last_rx_buffer[0],
                    (unsigned int)g_spi_last_rx_buffer[1],
                    (unsigned int)g_spi_last_rx_buffer[2],
                    (unsigned int)g_spi_last_rx_buffer[3],
                    (unsigned long)g_can_tx_count,
                    (unsigned long)g_can_rx_count,
                    (unsigned long)g_can_mismatch_count,
                    (unsigned long)(g_can_send_error_count
                                    + CanIf_GetErrorCount()),
                    (unsigned long)CanIf_GetRxDropCount(),
                    (unsigned int)g_can_last_rx_id,
                    (unsigned int)g_can_last_rx_data[0],
                    (unsigned int)g_can_last_rx_data[1],
                    (unsigned int)g_can_last_rx_data[2],
                    (unsigned int)g_can_last_rx_data[3],
                    (unsigned int)g_can_last_rx_data[4],
                    (unsigned int)g_can_last_rx_data[5],
                    (unsigned int)g_can_last_rx_data[6],
                    (unsigned int)g_can_last_rx_data[7]);

  if (length > 0) {
    if (length >= (int)sizeof(g_uart_log_buffer)) {
      length = (int)sizeof(g_uart_log_buffer) - 1;
    }

    if (StartUartLog((uint16_t)length) == 0U) {
      g_uart_tx_drop_count++;
    }
  }
}

static uint8_t StartUartLog(uint16_t length)
{
  if ((g_uart_tx_busy != 0U) || (length == 0U)) {
    return 0U;
  }

  g_uart_tx_busy = 1U;
  if (HAL_UART_Transmit_DMA(&huart1, g_uart_log_buffer, length) != HAL_OK) {
    g_uart_tx_busy = 0U;
    return 0U;
  }

  g_uart_tx_count++;
  return 1U;
}

static uint8_t StartSpiLoopbackDma(void)
{
  return SpiIf_StartTransfer((uint8_t *)g_spi_tx_buffer,
                             g_spi_rx_buffer,
                             SPI_LOOPBACK_SIZE);
}

static void ProcessSpiLoopbackResult(void)
{
  if (SpiIf_TakeError() != 0U) {
    g_spi_last_ok = 0U;
    g_spi_error_count++;
  }

  if (SpiIf_TakeComplete() == 0U) {
    return;
  }

  g_spi_transfer_count++;
  memcpy(g_spi_last_rx_buffer,
         g_spi_rx_buffer,
         SPI_LOOPBACK_SIZE);

  if (memcmp(g_spi_tx_buffer,
             g_spi_last_rx_buffer,
             SPI_LOOPBACK_SIZE) != 0) {
    g_spi_last_ok = 0U;
    g_spi_mismatch_count++;
    return;
  }

  g_spi_last_ok = 1U;
}

static void StartCanLoopbackTest(void)
{
  if (CanIf_SendStandard(CAN_LOOPBACK_ID,
                         g_can_tx_data,
                         CAN_LOOPBACK_SIZE) != 0U) {
    g_can_tx_count++;
  } else {
    g_can_send_error_count++;
  }
}

static void ProcessCanLoopbackResult(void)
{
  CanIfFrame frame;

  if (CanIf_TakeRxFrame(&frame) == 0U) {
    return;
  }

  g_can_rx_count++;
  g_can_last_rx_id = frame.standard_id;
  memcpy(g_can_last_rx_data, frame.data, CAN_LOOPBACK_SIZE);

  if ((frame.standard_id != CAN_LOOPBACK_ID)
      || (frame.dlc != CAN_LOOPBACK_SIZE)
      || (memcmp(frame.data,
                 g_can_tx_data,
                 CAN_LOOPBACK_SIZE) != 0)) {
    g_can_mismatch_count++;
  }
}

static uint8_t StartUartRx(void)
{
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1,
                                   g_uart_rx_dma_buffer,
                                   UART_RX_BUFFER_SIZE) != HAL_OK) {
    return 0U;
  }

  /* A half-transfer event does not end a frame in Normal DMA mode. */
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
  return 1U;
}

static void ProcessUartRx(void)
{
  UartProtocolFrame frame;
  UartProtocolResult result;
  int response_length;

  if ((g_uart_rx_frame_ready == 0U) || (g_uart_tx_busy != 0U)) {
    return;
  }

  __DMB();
  result = UartProtocol_Parse(g_uart_rx_frame,
                              g_uart_rx_length,
                              &frame);
  if ((result == UART_PROTOCOL_OK)
      && (frame.command == UART_PROTOCOL_CMD_PING)
      && (frame.payload_length == 0U)) {
    response_length = snprintf((char *)g_uart_log_buffer,
                               sizeof(g_uart_log_buffer),
                               "ACK PING CRC=OK\r\n");
  } else if (result == UART_PROTOCOL_ERROR_CRC) {
    g_uart_crc_error_count++;
    response_length = snprintf((char *)g_uart_log_buffer,
                               sizeof(g_uart_log_buffer),
                               "ERR CRC\r\n");
  } else if (result != UART_PROTOCOL_OK) {
    g_uart_protocol_error_count++;
    response_length = snprintf((char *)g_uart_log_buffer,
                               sizeof(g_uart_log_buffer),
                               "ERR FRAME code=%u\r\n",
                               (unsigned int)result);
  } else {
    response_length = snprintf((char *)g_uart_log_buffer,
                               sizeof(g_uart_log_buffer),
                               "ERR CMD 0x%02X\r\n",
                               (unsigned int)frame.command);
  }

  if ((response_length > 0)
      && (response_length < (int)sizeof(g_uart_log_buffer))
      && (StartUartLog((uint16_t)response_length) != 0U)) {
    g_uart_rx_count++;
    g_uart_rx_frame_ready = 0U;
  }
}

static uint16_t AdcRawToMillivolts(uint16_t raw)
{
  return (uint16_t)(((uint32_t)raw * BMS_ADC_VREF_MV
                     + (BMS_ADC_MAX_COUNTS / 2U))
                    / BMS_ADC_MAX_COUNTS);
}

static void ProcessAdcFrame(void)
{
  uint8_t i;
  uint32_t filter_count;

  if (g_adc_filter_count >= BMS_ADC_FILTER_LENGTH) {
    for (i = 0U; i < BMS_ADC_CHANNEL_COUNT; i++) {
      g_adc_filter_sum[i] -= g_adc_filter_samples[g_adc_filter_index][i];
    }
  } else {
    g_adc_filter_count++;
  }

  for (i = 0U; i < BMS_ADC_CHANNEL_COUNT; i++) {
    g_bms_data.adc_raw[i] = g_adc_dma_buffer[i];
    g_adc_filter_samples[g_adc_filter_index][i] = g_adc_dma_buffer[i];
    g_adc_filter_sum[i] += g_adc_dma_buffer[i];
  }

  g_adc_filter_index++;
  if (g_adc_filter_index >= BMS_ADC_FILTER_LENGTH) {
    g_adc_filter_index = 0U;
  }

  filter_count = g_adc_filter_count;
  for (i = 0U; i < BMS_ADC_CHANNEL_COUNT; i++) {
    g_bms_data.adc_filtered[i] =
        (uint16_t)(g_adc_filter_sum[i] / filter_count);
  }

  for (i = 0U; i < BMS_CELL_COUNT; i++) {
    g_bms_data.cell_voltage_mv[i] =
        AdcRawToMillivolts(g_bms_data.adc_filtered[i]);
  }

  /* Current and temperature transfer functions depend on the actual
   * sensor/front-end circuit and are intentionally not guessed here. */
  g_bms_data.pack_current_ma = 0;
  g_bms_data.temperature_cdeg[0] = 0;
  g_bms_data.temperature_cdeg[1] = 0;
  g_bms_data.timestamp_ms = g_system_ms;
  g_bms_data.valid = 1U;
  g_bms_data.sensor_calibrated = 0U;
}

static uint8_t StartAdcFrame(void)
{
  g_adc_busy = 1U;

  if (HAL_ADC_Start_DMA(&hadc1,
                        (uint32_t *)g_adc_dma_buffer,
                        BMS_ADC_CHANNEL_COUNT) != HAL_OK) {
    g_adc_busy = 0U;
    return 0U;
  }

  return 1U;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1) {
    g_adc_busy = 0U;
    __DMB();
    g_adc_frame_ready = 1U;
  }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1) {
    g_adc_busy = 0U;
    g_adc_error_pending = 1U;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) {
    g_uart_tx_busy = 0U;
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  if (huart->Instance == USART1) {
    if ((size > 0U) && (size <= UART_RX_BUFFER_SIZE)) {
      if (g_uart_rx_frame_ready == 0U) {
        memcpy(g_uart_rx_frame, g_uart_rx_dma_buffer, size);
        g_uart_rx_length = size;
        __DMB();
        g_uart_rx_frame_ready = 1U;
      } else {
        g_uart_rx_drop_count++;
      }
    }

    if (StartUartRx() == 0U) {
      g_uart_rx_restart_pending = 1U;
    }
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) {
    g_uart_error_count++;

    if (huart->gState == HAL_UART_STATE_READY) {
      g_uart_tx_busy = 0U;
    }
    if (huart->RxState == HAL_UART_STATE_READY) {
      g_uart_rx_restart_pending = 1U;
    }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    g_system_ms++;
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
