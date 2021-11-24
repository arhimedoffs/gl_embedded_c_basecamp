/*
 * SST25VF016B.h
 *
 *  Created on: 22 Nov 2021
 *      Author: arhi
 */

#ifndef INC_SST25VF016B_H_
#define INC_SST25VF016B_H_

#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"

typedef struct
{
  SPI_HandleTypeDef *hspi;	// SPI stream handler
  GPIO_TypeDef *cePort;		// CE# GPIO port
  uint16_t cePin;			// CE# GPIO pin
} SPIMEM_HandleDef;

typedef enum
{
	RC_SPI_ERR = -1,
	RC_OK = 0
} SPIMEM_RetCode;

SPIMEM_RetCode spimem_getid(SPIMEM_HandleDef* hmem, uint8_t rBuf[2]);

SPIMEM_RetCode spimem_getstatus(SPIMEM_HandleDef* hmem, uint8_t *status);

SPIMEM_RetCode spimem_read(SPIMEM_HandleDef* hmem, uint32_t addr, uint8_t *rBuf, uint32_t count);

SPIMEM_RetCode spimem_write(SPIMEM_HandleDef* hmem, uint32_t addr, const uint8_t *wBuf, uint32_t count);

SPIMEM_RetCode spimem_erase4k(SPIMEM_HandleDef* hmem, uint32_t addr);

SPIMEM_RetCode spimem_erase_all(SPIMEM_HandleDef* hmem);

SPIMEM_RetCode spimem_lock(SPIMEM_HandleDef* hmem);

SPIMEM_RetCode spimem_unlock(SPIMEM_HandleDef* hmem);

#endif /* INC_SST25VF016B_H_ */
