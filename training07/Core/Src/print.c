/*
 * print.c
 *
 *  Created on: Nov 22, 2021
 *      Author: arhi
 */

#include "main.h"

#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_tx;

volatile uint8_t uartTXbusy = 0;

#define UART_TX_SIZE 128
char uartTXbuf[UART_TX_SIZE] = {0};

#define UART_RX_SIZE 128
char uartRXbuf[UART_RX_SIZE] = {0};
char *const ptrUartRXbufEnd = uartRXbuf+UART_RX_SIZE;
char *ptrUartRXbuf = uartRXbuf;

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
 * Standard function for stdio output
 * @param file stream id
 * @param ptr pointer to buffer of symbols to output
 * @param len length of ptr buffer
 * @retval count of successfully transmitted bytes
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
 * Simple output string to UART
 * @note If string is longer than internal output buffer, then output is trimmed
 * @param ptr pointer to 0 terminated string
 */
void print(char* ptr) {
	while (uartTXbusy);
	uint8_t len = strlen(ptr);
	if (len > UART_TX_SIZE)
		len = UART_TX_SIZE;
	strncpy(uartTXbuf, ptr, len);
	HAL_UART_Transmit_DMA(&huart3, (uint8_t*)uartTXbuf, len);
	uartTXbusy = 1;
}
