/*
 * pca9685.h
 *
 *  Created on: Nov 6, 2021
 *      Author: Dmytro Kovalenko
 */

#ifndef INC_PCA9685_H_
#define INC_PCA9685_H_

#include <stdint.h>

#define LED_TRUE				1u
#define LED_FALSE				0u

#define LED_OPEN_COLLECTOR		0u
#define LED_PUSH_PULL			1u

#define LED_OFF_OUT_0			0u
#define LED_OFF_OUT_1			1u
#define LED_OFF_OUT_Z			2u

#define LED_OUT_NORMAL			0u
#define LED_OUT_INVERT			1u

#define LED_REG_MODE1			0u
#define LED_REG_MODE2			1u
#define LED_REG_SUBADDR1		2u
#define LED_REG_SUBADDR2		3u
#define LED_REG_SUBADDR3		4u
#define LED_REG_ALLCALLADDR		5u

#define LED_REG_CH_BASE_ON 		6u
#define LED_REG_CH_BASE_OFF		8u

#define LED_REG_ALL_ON 			250u
#define LED_REG_ALL_OFF			252u

#define LED_REG_PRESCALE		254u

#define LED_MODE1_RESTART		(1u << 7)
#define LED_MODE1_EXTCLK		(1u << 6)
#define LED_MODE1_AI			(1u << 5)
#define LED_MODE1_SLEEP			(1u << 4)
#define LED_MODE1_SUB1			(1u << 3)
#define LED_MODE1_SUB2			(1u << 2)
#define LED_MODE1_SUB3			(1u << 1)
#define LED_MODE1_ALLCALL		(1u << 0)

#define LED_MODE2_INVRT			(1u << 4)
#define LED_MODE2_OCH			(1u << 3)
#define LED_MODE2_OUTDRV		(1u << 2)
#define LED_MODE2_OUTNE1		(1u << 1)
#define LED_MODE2_OUTNE0		(1u << 0)

typedef struct
{
	uint8_t invert;			// invert output
	uint8_t outPushPull;	// push-pull or open-collector
	uint8_t offState;		// output state when OEn is high
} LED_InitDef;

typedef struct
{
  uint8_t address;	// I2C address in 7 bit format without RW bit
  LED_InitDef init;	// Initial configuration
} LED_HandleDef;

/**
 * Transmit data to PCA9685 via I2C
 * @param addr PWM chip I2C address
 * @param buf pointer to output data buffer
 * @param bufSize length of output data buffer
 * @retval result of transmission, 0 - OK
 */
int LED_I2C_Transmit(uint8_t addr, const uint8_t *buf, uint8_t bufSize);

/**
 * Change PCA9685 OEn pin state
 * @param state new OEn pin state
 */
void LED_OE_Write(uint8_t state);

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
