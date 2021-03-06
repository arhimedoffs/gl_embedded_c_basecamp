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
#include <string.h>
#include <ctype.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum AdcBufferStatus {
	ADC_BUFFER_EMPTY = 0,
	ADC_BUFFER_FULL
} AdcBufferStatus;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Maximum allowed value at 3V reference is 175. Higher value leads to calculation overflow
#define ADC_FILTER_SIZE 64
#define ADC_BUFFER_SIZE ADC_FILTER_SIZE
#define CALIBRATION_VOLTAGE 3300

#define T_MEASURE_PERIOD 5000 // Temperature measure period in ms

#define UART_TX_SIZE 128 // UART TX buffer size for DMA exchange

#define LEDS_COUNT 4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_tx;

/* USER CODE BEGIN PV */
int32_t cfgReferenceVoltage = 3000;
const uint16_t *const vRefCalibration = (const uint16_t *const)0x1fff7a2a;

volatile AdcBufferStatus adcBufferStatus = ADC_BUFFER_EMPTY;
uint16_t adcValues[ADC_BUFFER_SIZE][ADC_CHANNELS];

int32_t extTemperature = 0; // External temperature, 0.1 C
uint32_t lastTemperatureTick = 0;

char uartTXbuf[UART_TX_SIZE] = {0};
volatile uint8_t uartTXbusy = 0;

char rData;

uint8_t cmdLedToggle[LEDS_COUNT] = {0};
GPIO_TypeDef *const ledsGPIO[LEDS_COUNT] = {LED_BLUE_GPIO_Port, LED_ORANGE_GPIO_Port, LED_RED_GPIO_Port, LED_GREEN_GPIO_Port};
const uint16_t ledsPins[LEDS_COUNT] = {LED_BLUE_Pin, LED_ORANGE_Pin, LED_RED_Pin, LED_GREEN_Pin};
const char *const ledsNames[LEDS_COUNT] = {"Blue", "Orange", "Red", "Green"};
const char *const stateNames[2] = {"off", "on"};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
static int32_t adcBuf2Voltages(const uint16_t *adcBuffer, int32_t refV, int32_t chVoltages[ADC_CHANNELS]);
static int32_t adcProcess(int32_t *vRef);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief System function to standard output with output to UART3
 * @param file stream number
 * @param ptr data buffer for output
 * @param len length of data in buffer
 * @retval count of writed data
 */
int _write(int file, char* ptr, int len) {
	while (uartTXbusy);
	if (len > UART_TX_SIZE)
		len = UART_TX_SIZE;
	strncpy(uartTXbuf, ptr, len);
	HAL_UART_Transmit_DMA(&huart3, (uint8_t*)uartTXbuf, len);
	uartTXbusy = 1;
	return len;
}

/**
  * @brief  Tx Transfer completed callbacks.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart3) {
	  uartTXbusy = 0;
  }
}

/**
  * @brief  Regular conversion complete callback in non blocking mode
  * @param  hadc pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	adcBufferStatus = ADC_BUFFER_FULL;
	HAL_TIM_Base_Stop(&htim3);
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
 * Return external sensor temperature
 * @param vRef pointer to reference voltage
 * @retval temperature from external sensor in 0.1 C
 */
static int32_t adcProcess(int32_t *vRef) {
	uint16_t *ptrBuf = (uint16_t*)adcValues;
	int32_t chVoltages[ADC_CHANNELS] = {0};
	*vRef = adcBuf2Voltages(ptrBuf, *vRef, chVoltages);
	adcBufferStatus = ADC_BUFFER_EMPTY;
	return extTemperature = mv2tExternal(chVoltages[0]);
}

/**
 * External interrupt callback for SWT buttons
 *
 * SWT1 - right - blue
 * SWT3 - left - orange
 * SWT4 - top -red
 * SWT5 - bottom - green
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch(GPIO_Pin) {
	case SWT1_RIGHT_Pin:
		cmdLedToggle[0]++;
		break;
	case SWT3_LEFT_Pin:
		cmdLedToggle[1]++;
		break;
	case SWT4_UP_Pin:
		cmdLedToggle[2]++;
		break;
	case SWT5_DOWN_Pin:
		cmdLedToggle[3]++;
		break;
	}
}

/**
 * Get toggle commands from UART data stream
 * @param haurt pointer to UARD handler
 * @param cmdLedToggle array of toggle commands
 * @retval UART receive status
 */
HAL_StatusTypeDef getUartCmd(UART_HandleTypeDef *huart, uint8_t cmdLedToggle[LEDS_COUNT]) {
	// UART commands recognition
	char rData = '\0';
	HAL_StatusTypeDef receiveStatus = HAL_UART_Receive(&*huart,	(uint8_t*) &rData, 1, 1);
	if (receiveStatus == HAL_OK) {
		switch (rData) {
		case 'd':
		case 'D':
			cmdLedToggle[0]++;
			break;
		case 'a':
		case 'A':
			cmdLedToggle[1]++;
			break;
		case 'w':
		case 'W':
			cmdLedToggle[2]++;
			break;
		case 's':
		case 'S':
			cmdLedToggle[3]++;
			break;
		default:
			if (isprint(rData))
				printf("Unrecognised key \"%c\"\r\n", rData);
			else
				printf("Unrecognised key [0x%02x]\r\n", rData);
		}
	}
	return receiveStatus;
}

/**
 * Toggle LEDs according commands from UART and buttons
 * @param cmdLedToggle toggle commands counter for each led
 * @retval None
 */
void toggleLeds(uint8_t cmdLedToggle[LEDS_COUNT]) {
	// Leds switching according commands
	for (int i = 0; i < LEDS_COUNT; i++) {
		if (cmdLedToggle[i] > 0) {
			if (cmdLedToggle[i] & 0x01)
				HAL_GPIO_TogglePin(ledsGPIO[i], ledsPins[i]);

			uint8_t state =
					(HAL_GPIO_ReadPin(ledsGPIO[i], ledsPins[i]) == GPIO_PIN_SET) ?
							1 : 0;
			printf("%s led is %s\r\n", ledsNames[i], stateNames[state]);
			cmdLedToggle[i] = 0;
		}
	}
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // Temperature measurement start
	  uint32_t nowTick = HAL_GetTick();
	  if ((nowTick - lastTemperatureTick) >= T_MEASURE_PERIOD) {
		  lastTemperatureTick = nowTick;
		  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcValues, ADC_CHANNELS*ADC_BUFFER_SIZE);
		  HAL_TIM_Base_Start(&htim3);
	  }

	  // Measurement conversion and output
	  if (adcBufferStatus == ADC_BUFFER_FULL) {
		  extTemperature = adcProcess(&cfgReferenceVoltage);
		  printf("T = %d.%d C\r\n", (int)(extTemperature / 10), (int)(extTemperature % 10));
	  }

	  // UART commands recognition
      getUartCmd(&huart3, cmdLedToggle);
	  // Leds switching according commands
      toggleLeds(cmdLedToggle);

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
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 2;
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LED_GREEN_Pin|LED_ORANGE_Pin|LED_RED_Pin|LED_BLUE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LCD_BACKLIGHT_Pin */
  GPIO_InitStruct.Pin = LCD_BACKLIGHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BACKLIGHT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_GREEN_Pin LED_ORANGE_Pin LED_RED_Pin LED_BLUE_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin|LED_ORANGE_Pin|LED_RED_Pin|LED_BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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
