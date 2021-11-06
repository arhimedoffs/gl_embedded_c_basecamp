/*
 * pca9685.h
 *
 *  Created on: Nov 6, 2021
 *      Author: Dmytro Kovalenko
 */

#ifndef INC_PCA9685_H_
#define INC_PCA9685_H_

#include <stdint.h>

#define LED_OE_SET		1u
#define LED_OE_RESET	0u

#define LED_LED_0                 0x0001u  /* LED 0 selected    */
#define LED_LED_1                 0x0002u  /* LED 1 selected    */
#define LED_LED_2                 0x0004u  /* LED 2 selected    */
#define LED_LED_3                 0x0008u  /* LED 3 selected    */
#define LED_LED_4                 0x0010u  /* LED 4 selected    */
#define LED_LED_5                 0x0020u  /* LED 5 selected    */
#define LED_LED_6                 0x0040u  /* LED 6 selected    */
#define LED_LED_7                 0x0080u  /* LED 7 selected    */
#define LED_LED_8                 0x0100u  /* LED 8 selected    */
#define LED_LED_9                 0x0200u  /* LED 9 selected    */
#define LED_LED_10                0x0400u  /* LED 10 selected   */
#define LED_LED_11                0x0800u  /* LED 11 selected   */
#define LED_LED_12                0x1000u  /* LED 12 selected   */
#define LED_LED_13                0x2000u  /* LED 13 selected   */
#define LED_LED_14                0x4000u  /* LED 14 selected   */
#define LED_LED_15                0x8000u  /* LED 15 selected   */
#define LED_LED_All               0xFFFFu  /* All LEDs selected */

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

/**
 * Transmit data to PCA9685 via I2C
 */
int LED_I2C_Transmit(uint8_t addr, const uint8_t *buf, uint8_t bufSize);
/**
 * Change PCA9685 OEn pin state
 */
void LED_OE_Write(uint8_t state);

/**
 * Configure PCA9685 working modes
 */
void LED_Config(void);


void LED_Init(void);

#endif /* INC_PCA9685_H_ */
