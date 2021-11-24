/*
 * SST25VF016B.c
 *
 *  Created on: 22 Nov 2021
 *      Author: arhi
 */

#include "stm32f4xx_hal.h"

#include "SST25VF016B.h"

#include <string.h>

#define CMD_READ 0x03
#define CMD_READ_FAST 0x0B
#define CMD_ERASE_4K 0x10
#define CMD_ERASE_32K 0x52
#define CMD_ERASE_64K 0xD8
#define CMD_ERASE_ALL 0x60
#define CMD_PROG_BYTE 0x02
#define CMD_PROG_WORD 0xAD
#define CMD_READ_SR 0x05
#define CMD_ENWR_SR 0x50
#define CMD_WRITE_SR 0x01
#define CMD_WRITE_ENABLE 0x06
#define CMD_WRITE_DISABLE 0x04
#define CMD_READ_ID 0xAB
#define CMD_JEDEC_ID 0x9F
#define CMD_EN_BSY 0x70
#define CMD_DIS_BSY 0x80

#define SPI_BUF_SIZE 32
uint8_t spiBufRx[SPI_BUF_SIZE] = {0};
uint8_t spiBufTx[SPI_BUF_SIZE] = {0};

SPIMEM_RetCode spimem_getid(SPIMEM_HandleDef* hmem, uint8_t rBuf[2]) {
	const unsigned int exLen = 6;
	memset(spiBufTx, 0, exLen);
	memset(spiBufRx, 0, exLen);
	spiBufTx[0] = CMD_READ_ID;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	HAL_StatusTypeDef spi_status = HAL_SPI_TransmitReceive(hmem->hspi, spiBufTx, spiBufRx, exLen, 1000);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	if (spi_status != HAL_OK)
		return RC_SPI_ERR;
	rBuf[0] = spiBufRx[exLen-2];
	rBuf[1] = spiBufRx[exLen-1];
	return RC_OK;
}

SPIMEM_RetCode spimem_getstatus(SPIMEM_HandleDef* hmem, uint8_t *status) {
	const unsigned int exLen = 2;
	memset(spiBufTx, 0, exLen);
	memset(spiBufRx, 0, exLen);
	spiBufTx[0] = CMD_READ_SR;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	HAL_StatusTypeDef spi_status = HAL_SPI_TransmitReceive(hmem->hspi, spiBufTx, spiBufRx, exLen, 1000);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	if (spi_status != HAL_OK)
		return RC_SPI_ERR;
	*status = spiBufRx[exLen-1];
	return RC_OK;
}

SPIMEM_RetCode spimem_read(SPIMEM_HandleDef* hmem, uint32_t addr, uint8_t *rBuf, uint32_t count) {
	const unsigned int exLen = 4+8;
	memset(spiBufTx, 0, exLen);
	spiBufTx[0] = CMD_READ;
	spiBufTx[1] = (addr >> 16) & 0xFF;
	spiBufTx[2] = (addr >>  8) & 0xFF;
	spiBufTx[3] = (addr >>  0) & 0xFF;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	HAL_StatusTypeDef spi_status = HAL_SPI_Transmit(hmem->hspi, spiBufTx, 4, 1000);
	if (spi_status != HAL_OK) {
		HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
		return RC_SPI_ERR;
	}
	spi_status = HAL_SPI_Receive(hmem->hspi, rBuf, count, 1000);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	if (spi_status != HAL_OK)
		return RC_SPI_ERR;
	return RC_OK;
}

SPIMEM_RetCode spimem_writestatus(SPIMEM_HandleDef* hmem, uint8_t status) {
	const unsigned int exLen = 2;
	memset(spiBufTx, 0, exLen);
	spiBufTx[0] = CMD_ENWR_SR;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	HAL_StatusTypeDef spi_status = HAL_SPI_Transmit(hmem->hspi, spiBufTx, 1, 1000);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	if (spi_status != HAL_OK)
		return RC_SPI_ERR;
	memset(spiBufTx, 0, exLen);
	spiBufTx[0] = CMD_WRITE_SR;
	spiBufTx[1] = status;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	spi_status = HAL_SPI_Transmit(hmem->hspi, spiBufTx, 2, 1000);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	if (spi_status != HAL_OK)
		return RC_SPI_ERR;
	return RC_OK;
}

SPIMEM_RetCode spimem_write_set(SPIMEM_HandleDef* hmem, uint8_t enable) {
	HAL_StatusTypeDef spi_status;
	if (enable)
		spiBufTx[0] = CMD_WRITE_ENABLE;
	else
		spiBufTx[0] = CMD_WRITE_DISABLE;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	spi_status = HAL_SPI_Transmit(hmem->hspi, spiBufTx, 1, 1);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	return spi_status == HAL_OK ? RC_OK : RC_SPI_ERR;
}


SPIMEM_RetCode spimem_erase4k(SPIMEM_HandleDef* hmem, uint32_t addr) {
	HAL_StatusTypeDef spi_status;
	if (spimem_write_set(hmem, 1) != RC_OK)
		return RC_SPI_ERR;

	const unsigned int exLen = 4;
	memset(spiBufTx, 0, exLen);
	spiBufTx[0] = CMD_ERASE_4K;
	spiBufTx[1] = (addr >> 16) & 0xFF;
	spiBufTx[2] = (addr >>  8) & 0xFF;
	spiBufTx[3] = (addr >>  0) & 0xFF;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	spi_status = HAL_SPI_TransmitReceive(hmem->hspi, spiBufTx, spiBufRx, exLen, 1000);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	if (spi_status != HAL_OK)
		return RC_SPI_ERR;
	HAL_Delay(25);
	return RC_OK;
}

SPIMEM_RetCode spimem_erase_all(SPIMEM_HandleDef* hmem) {
	HAL_StatusTypeDef spi_status;
	if (spimem_write_set(hmem, 1) != RC_OK)
		return RC_SPI_ERR;

	const unsigned int exLen = 1;
	spiBufTx[0] = CMD_ERASE_ALL;
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
	spi_status = HAL_SPI_TransmitReceive(hmem->hspi, spiBufTx, spiBufRx, exLen, 1);
	HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
	if (spi_status != HAL_OK)
		return RC_SPI_ERR;
	HAL_Delay(50);
	return RC_OK;
}

SPIMEM_RetCode spimem_write(SPIMEM_HandleDef* hmem, uint32_t addr, const uint8_t *wBuf, uint32_t count) {
	HAL_StatusTypeDef spi_status;

	const unsigned int exLen = 5;
	for (int i = 0; i<count; i++) {
		if (spimem_write_set(hmem, 1) != RC_OK)
			return RC_SPI_ERR;
		spiBufTx[0] = CMD_PROG_BYTE;
		uint32_t wrAddr = addr+i;
		spiBufTx[1] = (wrAddr >> 16) & 0xFF;
		spiBufTx[2] = (wrAddr >>  8) & 0xFF;
		spiBufTx[3] = (wrAddr >>  0) & 0xFF;
		spiBufTx[4] = wBuf[i];
		HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_RESET);
		spi_status = HAL_SPI_Transmit(hmem->hspi, spiBufTx, exLen, 1000);
		HAL_GPIO_WritePin(hmem->cePort, hmem->cePin, GPIO_PIN_SET);
		if (spi_status != HAL_OK)
			return RC_SPI_ERR;
		HAL_Delay(1);
	}

	return RC_OK;
}




