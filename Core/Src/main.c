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
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bms_types.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BMS_ADC_VREF_MV      3300U
#define BMS_ADC_MAX_COUNTS   4095U
#define UART_LOG_BUFFER_SIZE 256U

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
static volatile uint8_t g_uart_error_pending = 0U;
static uint32_t g_uart_tx_count = 0U;
static uint32_t g_uart_tx_drop_count = 0U;
static BmsData g_bms_data = {0};
static BmsConfig g_bms_config = {0};
static BmsFault g_bms_fault = {0};
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
  /* USER CODE BEGIN 2 */
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
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
}

static void Task_1000ms(void)
{
  int length;

  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

  if (g_uart_error_pending != 0U) {
    g_uart_error_pending = 0U;
    g_uart_tx_drop_count++;
  }

  if (g_uart_tx_busy != 0U) {
    g_uart_tx_drop_count++;
    return;
  }

  length = snprintf((char *)g_uart_log_buffer,
                    sizeof(g_uart_log_buffer),
                    "time=%lu ms, 10ms=%lu, 100ms=%lu, valid=%u, sensor_cal=%u, dma_busy=%u, frames=%lu, adc_err=%lu, filter_n=%u, cell0=%u mV, raw=%u,%u,%u,%u,%u,%u,%u, avg=%u,%u,%u,%u,%u,%u,%u, fault=0x%08lx, cfg_ov=%u, tx=%lu, tx_drop=%lu\r\n",
                    (unsigned long)g_system_ms,
                    (unsigned long)count_10ms,
                    (unsigned long)count_100ms,
                    (unsigned int)g_bms_data.valid,
                    (unsigned int)g_bms_data.sensor_calibrated,
                    (unsigned int)g_adc_busy,
                    (unsigned long)g_adc_frame_count,
                    (unsigned long)g_adc_error_count,
                    (unsigned int)g_adc_filter_count,
                    (unsigned int)g_bms_data.cell_voltage_mv[0],
                    (unsigned int)g_bms_data.adc_raw[0],
                    (unsigned int)g_bms_data.adc_raw[1],
                    (unsigned int)g_bms_data.adc_raw[2],
                    (unsigned int)g_bms_data.adc_raw[3],
                    (unsigned int)g_bms_data.adc_raw[4],
                    (unsigned int)g_bms_data.adc_raw[5],
                    (unsigned int)g_bms_data.adc_raw[6],
                    (unsigned int)g_bms_data.adc_filtered[0],
                    (unsigned int)g_bms_data.adc_filtered[1],
                    (unsigned int)g_bms_data.adc_filtered[2],
                    (unsigned int)g_bms_data.adc_filtered[3],
                    (unsigned int)g_bms_data.adc_filtered[4],
                    (unsigned int)g_bms_data.adc_filtered[5],
                    (unsigned int)g_bms_data.adc_filtered[6],
                    (unsigned long)g_bms_fault.flags,
                    (unsigned int)g_bms_config.over_voltage_mv,
                    (unsigned long)g_uart_tx_count,
                    (unsigned long)g_uart_tx_drop_count);

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

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) {
    g_uart_tx_busy = 0U;
    g_uart_error_pending = 1U;
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
