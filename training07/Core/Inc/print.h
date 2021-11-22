/*
 * print.h
 *
 *  Created on: Nov 22, 2021
 *      Author: arhi
 */

#ifndef INC_PRINT_H_
#define INC_PRINT_H_

/**
 * Simple output string to UART
 * @note If string is longer than internal output buffer, then output is trimmed
 * @param ptr pointer to 0 terminated string
 */
void print(char* ptr);
int _write(int file, char* ptr, int len);

#endif /* INC_PRINT_H_ */
