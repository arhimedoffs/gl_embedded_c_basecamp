/*
 * pca9685.c
 *
 *  Created on: Nov 6, 2021
 *      Author: Dmytro Kovalenko
 */

#include "pca9685.h"

/**
 * Configure PCA9685 working modes
 * @param hled conprolled PWM handle
 */
int LED_Config(LED_HandleDef *hled) {
	uint8_t buf[3] = {0};

	LED_OE_Write(LED_TRUE);

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
	default:
		mode2 |= LED_MODE2_OUTNE1;
	};
	buf[2] = mode2;

	if (LED_I2C_Transmit((hled->address << 1), buf, sizeof(buf)))
		return -1;
	if (LED_PWM_Set(hled, 0, 0, 0))
		return -1;

	LED_OE_Write(LED_FALSE);
}

/**
 * Set PWM parameters for specific LED
 * @param hled conprolled PWM handle
 * @param led led number, 1..16, 0 - to all
 * @param onTime LED output ON state time, 0..4095
 * @param onOffset LED output ON state time offset from cycle start, 0..4095
 * @retval None
 */
int LED_PWM_Set(LED_HandleDef *hled, uint16_t led, uint16_t duty, uint16_t offset) {
	uint8_t buf[5] = {0};

	if (led > 16) return -2;

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

	return LED_I2C_Transmit((hled->address << 1), buf, sizeof(buf));
}
