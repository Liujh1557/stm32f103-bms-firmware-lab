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
#define BMS_ADC_POLL_TIMEOUT 2U

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
static uint16_t g_adc_raw[BMS_ADC_CHANNEL_COUNT] = {0};
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
static uint8_t ReadAdcFrame(void);

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
  count_10ms++;

  if (ReadAdcFrame() != 0U) {
    uint8_t i;

    for (i = 0U; i < BMS_ADC_CHANNEL_COUNT; i++) {
      g_bms_data.adc_raw[i] = g_adc_raw[i];
    }

    for (i = 0U; i < BMS_CELL_COUNT; i++) {
      g_bms_data.cell_voltage_mv[i] = AdcRawToMillivolts(g_adc_raw[i]);
    }

    /* Current and temperature transfer functions depend on the actual
     * sensor/front-end circuit and are intentionally not guessed here. */
    g_bms_data.pack_current_ma = 0;
    g_bms_data.temperature_cdeg[0] = 0;
    g_bms_data.temperature_cdeg[1] = 0;
    g_bms_data.timestamp_ms = g_system_ms;
    g_bms_data.valid = 1U;
    g_bms_data.calibrated = 0U;
  } else {
    g_bms_data.valid = 0U;
  }
}

static void Task_100ms(void)
{
  count_100ms++;
}

static void Task_1000ms(void)
{
  char message[192];
  int length;

  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

  length = snprintf(message,
                    sizeof(message),
                    "time=%lu ms, 10ms=%lu, 100ms=%lu, valid=%u, cal=%u, cell0=%u mV, raw=%u,%u,%u,%u,%u,%u,%u, fault=0x%08lx, code=%u, latched=%u, cfg_ov=%u\r\n",
                    (unsigned long)g_system_ms,
                    (unsigned long)count_10ms,
                    (unsigned long)count_100ms,
                    (unsigned int)g_bms_data.valid,
                    (unsigned int)g_bms_data.calibrated,
                    (unsigned int)g_bms_data.cell_voltage_mv[0],
                    (unsigned int)g_bms_data.adc_raw[0],
                    (unsigned int)g_bms_data.adc_raw[1],
                    (unsigned int)g_bms_data.adc_raw[2],
                    (unsigned int)g_bms_data.adc_raw[3],
                    (unsigned int)g_bms_data.adc_raw[4],
                    (unsigned int)g_bms_data.adc_raw[5],
                    (unsigned int)g_bms_data.adc_raw[6],
                    (unsigned long)g_bms_fault.flags,
                    (unsigned int)g_bms_fault.active_code,
                    (unsigned int)g_bms_fault.latched,
                    (unsigned int)g_bms_config.over_voltage_mv);

  if (length > 0) {
    if (length > (int)sizeof(message)) {
      length = (int)sizeof(message);
    }

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)message,
                      (uint16_t)length,
                      100);
  }
}

static uint16_t AdcRawToMillivolts(uint16_t raw)
{
  return (uint16_t)(((uint32_t)raw * BMS_ADC_VREF_MV
                     + (BMS_ADC_MAX_COUNTS / 2U))
                    / BMS_ADC_MAX_COUNTS);
}

static uint8_t ReadAdcFrame(void)
{
  uint8_t i;

  if (HAL_ADC_Start(&hadc1) != HAL_OK) {
    return 0U;
  }

  for (i = 0U; i < BMS_ADC_CHANNEL_COUNT; i++) {
    if (HAL_ADC_PollForConversion(&hadc1, BMS_ADC_POLL_TIMEOUT) != HAL_OK) {
      (void)HAL_ADC_Stop(&hadc1);
      return 0U;
    }

    g_adc_raw[i] = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }

  if (HAL_ADC_Stop(&hadc1) != HAL_OK) {
    return 0U;
  }

  return 1U;
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
