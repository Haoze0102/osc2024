#ifndef __UART_BOOT_H__
#define __UART_BOOT_H__

#include "aux.h"
#include "exception.h"
#include "gpio.h"
#include "mmio.h"
#include "task.h"

#define MAX_BUF_SIZE 0x100
#define RX (1 << 0)  // read
#define TX (1 << 1)  // write

void uart_init();
void _putchar(char c);
void _async_putchar(char c);
void uart_read(char* buf, uint32_t size);
void async_uart_read(char* buf, uint32_t size);
void uart_flush();
void uart_write_string(char* str);
void uart_puth(uint32_t d);
void uart_putc(char* buf, uint32_t size);
void delay(uint32_t t);

void uart_enable_aux_int();
void uart_enable_int(uint32_t type);
void uart_disable_int(uint32_t type);

#endif