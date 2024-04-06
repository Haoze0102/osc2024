#ifndef __PRINTF_H__
#define __PRINTF_H__

#include "uart.h"

#define MAX_PRINT_BUF_SIZE 0x100

int printf(char *fmt, ...);
int async_printf(char *fmt, ...);
unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);
unsigned int sprintf(char *dst, char *fmt, ...);

#endif