/*
 * SST25VF016B.c
 *
 *  Created on: 22 Nov 2021
 *      Author: arhi
 */

#include "stm32f4xx_hal.h"

#include "SST25VF016B.h"

#include <string.h>

#define SPI_BUF_SIZE 32

uint8_t spiBufRx[SPI_BUF_SIZE] = {0};
uint8_t spiBufTx[SPI_BUF_SIZE] = {0};

SPIMEM_RetCode spimem_getid(SPIMEM_HandleDef* hmem, uint8_t rBuf[2]) {
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	memset(spiBufTx, 0, 6);
	memset(spiBufRx, 0, 6);
	spiBufTx[0] = 0xAB;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hmem->hspi, spiBufTx, spiBufRx, 6, 1000);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	if (status != HAL_OK)
		return RC_SPI_ERR;
	rBuf[0] = spiBufRx[4];
	rBuf[1] = spiBufRx[5];
	return RC_OK;
}


