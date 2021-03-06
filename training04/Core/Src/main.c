/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum AdcBufferStatus {
	ADC_BUFFER_NRDY = 0,
	ADC_BUFFER_FIRST_RDY,
	ADC_BUFFER_SECOND_RDY
} AdcBufferStatus;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Maximum allowed value at 3V reference is 175. Higher value leads to calculation overflow
#define ADC_FILTER_SIZE 25
#define ADC_BUFFER_SIZE (2 * ADC_FILTER_SIZE)

#define LED_CCR_BLUE (htim4.Instance->CCR4)
#define LED_CCR_GREEN (htim4.Instance->CCR1)
#define LED_CCR_ORANGE (htim4.Instance->CCR2)

#define CALIBRATION_VOLTAGE 3300

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */
int32_t cfgReferenceVoltage = 3000;

volatile AdcBufferStatus adcBufferStatus = ADC_BUFFER_NRDY;
uint16_t adcValues[ADC_BUFFER_SIZE][ADC_CHANNELS];

int32_t potVoltage = 0; // Potentiometer voltage, mV
int32_t extTemperature = 0; // External temperature, 0.1 C
int32_t intTemperature = 0; // Internal temperature, 0.1 C

uint8_t wrnPotentiometer = 0;
uint8_t wrnExtTemperature = 0;
uint8_t wrnIntTemperature = 0;

const int32_t limPotentiometer = 1500;
const int32_t hystPotentiometer = 100;
const int32_t limTemperature = 40*10;
const int32_t hystTemperature = 1*10;

const int32_t minPotentiometer = 0;
const int32_t maxPotentiometer = 3000;
const int32_t minTemperature = 20*10;
const int32_t maxTemperature = 60*10;

const uint16_t wrnBlinkPeriod[4] = {0, 1000/2, 1000/5, 1000/10};

const uint16_t *const vRefCalibration = (const uint16_t *const)0x1fff7a2a;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
uint8_t hlimCheck(int32_t value, int32_t limit, int32_t hyst, uint8_t prevCheck);
int32_t trimAtLimits(int32_t value, int32_t min, int32_t max);
static void warningBlink(uint8_t warningCount);
static int32_t mv2tInternal(int32_t voltage);
static int32_t mv2tExternal(int32_t voltage);
static int32_t adcBuf2Voltages(const uint16_t *adcBuffer, int32_t refV, int32_t chVoltages[ADC_CHANNELS]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#ifdef DEBUG
/**
 * @brief System function to standard output
 * @param file stream number
 * @param ptr data buffer for output
 * @param len length of data in buffer
 * @retval count of writed data
 */
int _write(int file, char* ptr, int len) {
	int i = 0;
	for (i = 0; i<len; i++)
		ITM_SendChar(*ptr++);
	return len;
}
#endif // DEBUG
/**
  * @brief  Regular conversion half DMA transfer callback in non blocking mode
  * @param  hadc pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	adcBufferStatus = ADC_BUFFER_FIRST_RDY;
}
/**
  * @brief  Regular conversion complete callback in non blocking mode
  * @param  hadc pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	adcBufferStatus = ADC_BUFFER_SECOND_RDY;
}

/**
 * @brief Checking high limit exceed with hysteresis
 * @param value current measurement value
 * @param limit limit setpoint
 * @param hyst hysteresis value
 * @param prevCheck previous checking status
 * @retval boolean result of checking
 */
uint8_t hlimCheck(int32_t value, int32_t limit, int32_t hyst, uint8_t prevCheck)
{
	if (prevCheck)
		return value > (limit-hyst);
	return value >= (limit+hyst);
}

/**
 * @brief Limit value within specified limits [min, max]
 * @param value measured value
 * @param min low limit
 * @param max high limit
 * @retval limited value
 */
int32_t trimAtLimits(int32_t value, int32_t min, int32_t max)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

/**
 * Convert voltage to external board temperature
 * @param voltage measured voltage in 0.1 mV
 * @retval temperature in 0.1 C
 */
static int32_t mv2tExternal(int32_t voltage) {
	return -voltage / 20 + 1010;
}

/**
 * Convert voltage to internal MCU temperature
 * @param voltage measured voltage in 0.1 mV
 * @retval temperature in 0.1 C
 */
static int32_t mv2tInternal(int32_t voltage) {
	return (voltage - 7600) * 10 / 25 + 250;
}

/**
 * Convert ADC DMA buffer values to voltages via averaging
 * @param adcBuffer point to ADC DMA buffer as uint16_t [ADC_FILTER_SIZE][ADC_CHANNELS]
 * @param refV reference voltage
 * @param chVoltages array of resulted values in 0.1 mV
 * @retval updated reference voltage
 */
static int32_t adcBuf2Voltages(const uint16_t *adcBuffer, int32_t refV, int32_t chVoltages[ADC_CHANNELS]) {
	for (int i = 0; i < ADC_FILTER_SIZE; i++)
	  for (int j = 0; j < ADC_CHANNELS; j++)
		  chVoltages[j] += *adcBuffer++;

	#if ENABLE_VREF_CORRECTION == 1
	if (chVoltages[ADC_CHANNELS-1] > 0)
		refV = CALIBRATION_VOLTAGE * ADC_FILTER_SIZE * (*vRefCalibration) / chVoltages[ADC_CHANNELS-1];
	#endif // ENABLE_VREF_CORRECTION == 1
	for (int i = 0; i < ADC_CHANNELS; i++)
	  chVoltages[i] = (chVoltages[i] * refV / ADC_FILTER_SIZE * 10) >> 12; // average values is measured and averaged ADC_ch * 10
	return refV;
}

/**
 * @brief Toggle warning LED based on actual warnings
 * @param warningCount current warnings counter
 * @retval None
 */
void warningBlink(uint8_t warningCount)
{
	static uint32_t tickStart = 0;
	static uint8_t warningCountPrev = 0;

	warningCount = warningCount >= sizeof(wrnBlinkPeriod) ? (sizeof(wrnBlinkPeriod)-1) : warningCount;

	if (warningCount == 0) {
		HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
	} else if (warningCountPrev == 0) {
		tickStart = HAL_GetTick();
		HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
	} else {
		uint32_t tickNow = HAL_GetTick();
		if ((tickNow - tickStart) >= wrnBlinkPeriod[warningCount]) {
			tickStart = tickNow;
			HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
		}
	}
	warningCountPrev = warningCount;
	return;
}
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
  MX_TIM4_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  __attribute__ ((unused)) HAL_StatusTypeDef status = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcValues, ADC_CHANNELS*ADC_BUFFER_SIZE);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  HAL_TIM_Base_Start(&htim3);

  while (1)
  {
	  // Wait data
	  while(adcBufferStatus == ADC_BUFFER_NRDY) {
		  warningBlink(wrnPotentiometer + wrnExtTemperature + wrnIntTemperature);
#ifndef DEBUG
		  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
#endif // DEBUG
	  }

	  // Captured data averaging
	  uint16_t *ptrBuf = (uint16_t*)adcValues;
	  if (adcBufferStatus == ADC_BUFFER_SECOND_RDY)
		  ptrBuf += ADC_CHANNELS * ADC_FILTER_SIZE;
	  int32_t chVoltages[ADC_CHANNELS] = {0};
	  cfgReferenceVoltage = adcBuf2Voltages(ptrBuf, cfgReferenceVoltage, chVoltages);
	  adcBufferStatus = ADC_BUFFER_NRDY;

	  potVoltage = chVoltages[0] / 10;
	  extTemperature = mv2tExternal(chVoltages[1]);
	  intTemperature = mv2tInternal(chVoltages[2]);

	  wrnPotentiometer = hlimCheck(potVoltage, limPotentiometer, hystPotentiometer, wrnPotentiometer);
	  wrnExtTemperature = hlimCheck(extTemperature, limTemperature, hystTemperature, wrnExtTemperature);
	  wrnIntTemperature = hlimCheck(intTemperature, limTemperature, hystTemperature, wrnIntTemperature);

#ifdef DEBUG
	  printf("%ld %ld %ld %d\n", potVoltage, extTemperature, intTemperature, wrnPotentiometer + wrnExtTemperature + wrnIntTemperature);
#endif // DEBUG

	  int32_t scaledToPwm;
	  scaledToPwm = (potVoltage - minPotentiometer) * TIM4_PERIOD / (maxPotentiometer - minPotentiometer);
	  LED_CCR_BLUE = (uint32_t)trimAtLimits(scaledToPwm, 0, TIM4_PERIOD);

	  scaledToPwm = (extTemperature - minTemperature) * TIM4_PERIOD / (maxTemperature - minTemperature);
	  LED_CCR_GREEN = (uint32_t)trimAtLimits(scaledToPwm, 0, TIM4_PERIOD);

	  scaledToPwm = (intTemperature - minTemperature) * TIM4_PERIOD / (maxTemperature - minTemperature);
	  LED_CCR_ORANGE = (uint32_t)trimAtLimits(scaledToPwm, 0, TIM4_PERIOD);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
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
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = TIM3_PRESCALER-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = TIM3_PERIOD-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = TIM4_PRESCALER-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = TIM4_PERIOD-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LCD_BACKLIGHT_Pin */
  GPIO_InitStruct.Pin = LCD_BACKLIGHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BACKLIGHT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_RED_Pin */
  GPIO_InitStruct.Pin = LED_RED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_RED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SWT4_UP_Pin SWT5_DOWN_Pin SWT3_LEFT_Pin SWT1_RIGHT_Pin */
  GPIO_InitStruct.Pin = SWT4_UP_Pin|SWT5_DOWN_Pin|SWT3_LEFT_Pin|SWT1_RIGHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SWT2_CENTER_Pin */
  GPIO_InitStruct.Pin = SWT2_CENTER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SWT2_CENTER_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
