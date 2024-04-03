#include <stdint.h>

#include "dtb.h"
#include "uart.h"

extern void *_dtb_ptr;
void kernel_main(char* x0) {
    uart_enable_int(RX | TX);
    uart_enable_aux_int();
    dtb_init(_dtb_ptr);


    enable_interrupt();
    shell();
}