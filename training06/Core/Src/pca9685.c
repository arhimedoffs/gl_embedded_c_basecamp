/*
 * pca9685.c
 *
 *  Created on: Nov 6, 2021
 *      Author: Dmytro Kovalenko
 */

#include "stm32f4xx_hal.h"

#include "pca9685.h"


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

#define I2C_MAX_DELAY	1000

/**
 * Transmit data to PCA9685 via I2C
 * @param hLed pointer to PWM handle
 * @param buf pointer to output data buffer
 * @param bufSize length of output data buffer
 * @retval result of transmission, 0 - OK
 */
int LED_I2C_Transmit(LED_HandleDef *hLed, const uint8_t *buf, uint8_t bufSize) {
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hLed->hi2c, hLed->address, (uint8_t*)buf, bufSize, I2C_MAX_DELAY);
	return (status == HAL_OK) ? 0 : -1;
}

/**
 * Change PCA9685 OEn pin state
 * @param hLed pointer to PWM handle
 * @param state new OEn pin state
 */
void LED_OE_Write(LED_HandleDef *hLed, GPIO_PinState state) {
	HAL_GPIO_WritePin(hLed->oePort, hLed->oePin, state);
}

/**
 * Configure PCA9685 working modes
 * @param hled conprolled PWM handle
 */
int LED_Config(LED_HandleDef *hled) {
	uint8_t buf[3] = {0};

	LED_OE_Write(hled, GPIO_PIN_SET);

	buf[0] = LED_REG_MODE1;
	buf[1] = LED_MODE1_AI;
	uint8_t mode2 = 0;
	if (hled->init.invert)
		mode2 |= LED_MODE2_INVRT;
	if (hled->init.outPushPull)
		mode2 |= LED_MODE2_OUTDRV;
	switch (hled->init.offState) {
	case LED_OFF_OUT_0:
		break;
	case LED_OFF_OUT_1:
		mode2 |= LED_MODE2_OUTNE0;
		break;
	case LED_OFF_OUT_Z:
	default:
		mode2 |= LED_MODE2_OUTNE1;
	};
	buf[2] = mode2;

	if (LED_I2C_Transmit(hled, buf, sizeof(buf)))
		return -1;
	if (LED_PWM_Set(hled, 0, 0, 0))
		return -1;

	LED_OE_Write(hled, GPIO_PIN_RESET);
	return 0;
}

/**
 * Set PWM parameters for specific LED
 * @param hled conprolled PWM handle
 * @param led led number, 1..16, 0 - to all
 * @param onTime LED output ON state time, 0..4095
 * @param onOffset LED output ON state time offset from cycle start, 0..4095
 * @retval None
 */
int LED_PWM_Set(LED_HandleDef *hled, LED_Pin led, uint16_t duty, uint16_t offset) {
	uint8_t buf[5] = {0};

	if ((led < LED_PIN_ALL) || (led >= LED_PIN_LAST)) return -2;

	if (led == 0)
		buf[0] = LED_REG_ALL_ON;
	else
		buf[0] = LED_REG_CH_BASE_ON + 4*(led-1);

	if (duty > 0x0fff)
		duty = 0x0fff;
	uint16_t onTime = offset & 0x0fff;
	uint16_t offTime = (onTime + duty) & 0x0fff;

	*((uint16_t*)(buf+1)) = onTime;
	*((uint16_t*)(buf+3)) = offTime;

	return LED_I2C_Transmit(hled, buf, sizeof(buf));
}
