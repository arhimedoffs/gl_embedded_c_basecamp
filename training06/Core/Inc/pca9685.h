/*
 * pca9685.h
 *
 *  Created on: Nov 6, 2021
 *      Author: Dmytro Kovalenko
 */

#ifndef INC_PCA9685_H_
#define INC_PCA9685_H_

#include <stdint.h>

#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_i2c.h"

#define LED_TRUE				1u
#define LED_FALSE				0u

#define LED_OPEN_COLLECTOR		0u
#define LED_PUSH_PULL			1u

#define LED_OFF_OUT_0			0u
#define LED_OFF_OUT_1			1u
#define LED_OFF_OUT_Z			2u

#define LED_OUT_NORMAL			0u
#define LED_OUT_INVERT			1u

typedef struct
{
	uint8_t invert;			// invert output
	uint8_t outPushPull;	// push-pull or open-collector
	uint8_t offState;		// output state when OEn is high
} LED_InitDef;

typedef struct
{
  uint8_t address;			// I2C address in 7 bit format without RW bit
  I2C_HandleTypeDef *hi2c;	// I2C stream handler
  GPIO_TypeDef *oePort;		// OE GPIO port
  uint16_t oePin;			// OE GPIO pin
  LED_InitDef init;			// initial configuration
} LED_HandleDef;

/**
 * Configure PCA9685 working modes
 * @param hled conprolled PWM handle
 */
int LED_Config(LED_HandleDef *hled);

/**
 * Set PWM parameters for specific LED
 * @param hled conprolled PWM handle
 * @param led led number, 1..16, 0 - to all
 * @param onTime LED output ON state time, 0..4095
 * @param onOffset LED output ON state time offset from cycle start, 0..4095
 * @retval None
 */
int LED_PWM_Set(LED_HandleDef *hled, uint16_t led, uint16_t onTime, uint16_t onOffset);

#endif /* INC_PCA9685_H_ */
