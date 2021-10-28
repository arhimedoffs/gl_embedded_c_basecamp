/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TIM4_PERIOD 100
#define SAMPLING_RATE 100
#define TIM4_PRESCALER 1600
#define TIM3_PRESCALER 160
#define TIM3_PERIOD (TIM34_CLK / (TIM3_PRESCALER * SAMPLING_RATE))
#define TIM34_CLK 16000000
#define ADC1_POT_Pin GPIO_PIN_3
#define ADC1_POT_GPIO_Port GPIOA
#define ADC1_TEMP_Pin GPIO_PIN_1
#define ADC1_TEMP_GPIO_Port GPIOB
#define LCD_BACKLIGHT_Pin GPIO_PIN_9
#define LCD_BACKLIGHT_GPIO_Port GPIOE
#define LED_GREEN_Pin GPIO_PIN_12
#define LED_GREEN_GPIO_Port GPIOD
#define LED_ORANGE_Pin GPIO_PIN_13
#define LED_ORANGE_GPIO_Port GPIOD
#define LED_RED_Pin GPIO_PIN_14
#define LED_RED_GPIO_Port GPIOD
#define LED_BLUE_Pin GPIO_PIN_15
#define LED_BLUE_GPIO_Port GPIOD
#define SWT4_UP_Pin GPIO_PIN_6
#define SWT4_UP_GPIO_Port GPIOC
#define SWT4_UP_EXTI_IRQn EXTI9_5_IRQn
#define SWT5_DOWN_Pin GPIO_PIN_8
#define SWT5_DOWN_GPIO_Port GPIOC
#define SWT5_DOWN_EXTI_IRQn EXTI9_5_IRQn
#define SWT3_LEFT_Pin GPIO_PIN_9
#define SWT3_LEFT_GPIO_Port GPIOC
#define SWT3_LEFT_EXTI_IRQn EXTI9_5_IRQn
#define SWT2_CENTER_Pin GPIO_PIN_15
#define SWT2_CENTER_GPIO_Port GPIOA
#define SWT2_CENTER_EXTI_IRQn EXTI15_10_IRQn
#define SWT1_RIGHT_Pin GPIO_PIN_11
#define SWT1_RIGHT_GPIO_Port GPIOC
#define SWT1_RIGHT_EXTI_IRQn EXTI15_10_IRQn
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
